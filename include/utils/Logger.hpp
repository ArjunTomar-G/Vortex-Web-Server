#pragma once

#include <string>
#include <mutex>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
private:
    static std::mutex log_mutex;
    static std::string get_current_time();
    static std::string level_to_string(LogLevel level);
    static std::string level_to_color(LogLevel level);
    static const std::string RESET_COLOR;

public:
    // Core logging function
    static void log(LogLevel level, const std::string& message);
    
    // Quick-access wrappers
    static void info(const std::string& message) { log(LogLevel::INFO, message); }
    static void warn(const std::string& message) { log(LogLevel::WARNING, message); }
    static void error(const std::string& message) { log(LogLevel::ERROR, message); }
    static void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
};