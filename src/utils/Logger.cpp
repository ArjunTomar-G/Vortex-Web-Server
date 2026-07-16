#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Initialize the static mutex
std::mutex Logger::log_mutex;
const std::string Logger::RESET_COLOR = "\033[0m";

std::string Logger::get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::DEBUG:   return "DEBUG";
        default:                return "UNKNOWN";
    }
}

std::string Logger::level_to_color(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "\033[32m"; // Green
        case LogLevel::WARNING: return "\033[33m"; // Yellow
        case LogLevel::ERROR:   return "\033[31m"; // Red
        case LogLevel::DEBUG:   return "\033[36m"; // Cyan
        default:                return RESET_COLOR;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    // 1. Lock the terminal so no other thread can print right now
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // 2. Safely output the color-coded, timestamped string
    std::cout << level_to_color(level) 
              << "[" << get_current_time() << "] "
              << "[" << level_to_string(level) << "] "
              << message 
              << RESET_COLOR << "\n";
} // 3. Mutex is automatically released here when lock goes out of scope