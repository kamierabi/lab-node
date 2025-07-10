#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <filesystem>


class Logger {
public:
    enum class LogLevel {DEBUG, WARNING, ERR, FATAL};
    explicit Logger(const std::string& log_file_prefix = "log");
    ~Logger();
    void log(LogLevel level, const std::string& message);
private:
    std::ofstream log_file_;
    std::string level_to_string(LogLevel level);
    std::string current_time();
};