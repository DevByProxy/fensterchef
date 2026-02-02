#ifndef FENSTERCHEF_UTILS_LOGGER_HPP
#define FENSTERCHEF_UTILS_LOGGER_HPP

#include <string>
#include <string_view>
#include <fstream>
#include <memory>
#include <sstream>

namespace fensterchef {

/**
 * @brief Logging facility for Fensterchef
 * 
 * Provides a modern logging interface with multiple log levels and
 * formatted output. This replaces the old LOG macro system.
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };
    
    /**
     * @brief Get the singleton logger instance
     */
    static Logger& instance();
    
    /**
     * @brief Initialize the logger
     * @param log_file Path to log file (empty for stderr)
     * @param min_level Minimum level to log
     */
    void initialize(const std::string& log_file = "", Level min_level = Level::Info);
    
    /**
     * @brief Set minimum log level
     */
    void setLevel(Level level) { min_level_ = level; }
    
    /**
     * @brief Get current log level
     */
    Level getLevel() const { return min_level_; }
    
    /**
     * @brief Log a message
     * @param level Log level
     * @param message The message to log
     */
    void log(Level level, std::string_view message);
    
    /**
     * @brief Log a formatted message
     * @param level Log level
     * @param format Format string (printf-style)
     * @param args Format arguments
     */
    template<typename... Args>
    void logf(Level level, const char* format, Args&&... args);
    
    /**
     * @brief Log debug message
     */
    void debug(std::string_view message) {
        log(Level::Debug, message);
    }
    
    /**
     * @brief Log info message
     */
    void info(std::string_view message) {
        log(Level::Info, message);
    }
    
    /**
     * @brief Log warning message
     */
    void warn(std::string_view message) {
        log(Level::Warning, message);
    }
    
    /**
     * @brief Log error message
     */
    void error(std::string_view message) {
        log(Level::Error, message);
    }
    
    /**
     * @brief Flush log output
     */
    void flush();
    
private:
    Logger() = default;
    
    std::ofstream log_file_;
    Level min_level_{Level::Info};
    bool use_file_{false};
    
    static const char* levelToString(Level level);
};

// Convenience macros for logging
#define LOG_DEBUG(msg) ::fensterchef::Logger::instance().debug(msg)
#define LOG_INFO(msg) ::fensterchef::Logger::instance().info(msg)
#define LOG_WARN(msg) ::fensterchef::Logger::instance().warn(msg)
#define LOG_ERROR(msg) ::fensterchef::Logger::instance().error(msg)

} // namespace fensterchef

#endif // FENSTERCHEF_UTILS_LOGGER_HPP
