#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads) : stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                    if (stop_ && tasks_.empty()) return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}
