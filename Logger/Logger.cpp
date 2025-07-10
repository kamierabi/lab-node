#include "Logger.hpp"


Logger::Logger(const std::string& log_file_prefix) {
    std::filesystem::create_directory("logs");
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);
    std::ostringstream oss;
    oss << "logs/" << log_file_prefix << "_"
            << std::put_time(&local_time, "%Y%m%d_%H%M%S")
            << ".log";
    log_file_.open(oss.str(), std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
            throw std::runtime_error("Cannot open log file");
        }
}


Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}


void Logger::log(LogLevel level, const std::string& message) {
    if (log_file_.is_open()) {
        log_file_ << "[" << level_to_string(level) << "] - "
                  << current_time() << " - "
                  << message << std::endl;
        }
}


std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
        }
}


std::string Logger::current_time() {
    std::time_t now = std::time(nullptr);
    std::tm local_time = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y/%m/%d - %H:%M:%S");
    return oss.str();
}
