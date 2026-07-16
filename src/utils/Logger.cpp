#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::mutex Logger::log_mutex;
std::ofstream Logger::log_file;
const std::string Logger::RESET_COLOR = "\033[0m";
// open the file safely in append mode
void Logger::init(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(log_mutex);
    log_file.open(filepath, std::ios::app); 
    if (!log_file.is_open()) {
        std::cerr << "\033[31m[FATAL] Could not open log file: " << filepath << RESET_COLOR << "\n";
    }
}

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
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::string time_str = get_current_time();
    std::string level_str = level_to_string(level);

    // Terminal Output
    std::cout << level_to_color(level) 
              << "[" << time_str << "] "
              << "[" << level_str << "] "
              << message 
              << RESET_COLOR << "\n";

    // File Output
    if (log_file.is_open()) {
        log_file << "[" << time_str << "] [" << level_str << "] " << message << "\n";
        log_file.flush(); 
    }
}