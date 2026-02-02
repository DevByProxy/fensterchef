#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fensterchef {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::initialize(const std::string& log_file, Level min_level) {
    min_level_ = min_level;
    
    if (!log_file.empty()) {
        log_file_.open(log_file, std::ios::out | std::ios::app);
        if (log_file_.is_open()) {
            use_file_ = true;
        } else {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
        }
    }
}

void Logger::log(Level level, std::string_view message) {
    if (level < min_level_) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::ostream& out = use_file_ ? log_file_ : std::cerr;
    
    out << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
        << levelToString(level) << ": " << message << std::endl;
}

void Logger::flush() {
    if (use_file_) {
        log_file_.flush();
    } else {
        std::cerr.flush();
    }
}

const char* Logger::levelToString(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

} // namespace fensterchef
