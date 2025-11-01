/**
 * @file SystemLogger.h
 * @brief Centralized logging system for LoRaTNCX
 * @author LoRaTNCX Project
 * @date November 1, 2025
 *
 * Provides a comprehensive logging system that captures all system messages
 * and boot information while keeping the console clean for TNC operations.
 */

#pragma once

#include <Arduino.h>
#include <deque>
#include <vector>
#include <functional>

/**
 * @brief Log levels for filtering messages
 */
enum class LogLevel : uint8_t {
    DEBUG = 0,    // Debug information
    INFO = 1,     // General information
    WARNING = 2,  // Warning messages
    ERROR = 3,    // Error messages
    CRITICAL = 4  // Critical system errors
};

/**
 * @brief Individual log entry
 */
struct LogEntry {
    uint32_t timestamp;     // Milliseconds since boot
    LogLevel level;         // Log level
    String component;       // Component name (e.g., "WiFi", "LoRa", "TNC")
    String message;         // Log message
    
    LogEntry(uint32_t ts, LogLevel lvl, const String& comp, const String& msg)
        : timestamp(ts), level(lvl), component(comp), message(msg) {}
};

/**
 * @brief Centralized system logger
 * 
 * This class captures all system messages that would normally be printed
 * to Serial, stores them in memory, and provides access via API calls
 * and TNC commands.
 */
class SystemLogger {
public:
    static const size_t DEFAULT_MAX_ENTRIES = 500;  // Maximum log entries to keep
    static const size_t WEB_LOG_LIMIT = 200;        // Max entries for web API

private:
    std::deque<LogEntry> logEntries;
    size_t maxEntries;
    LogLevel minLevel;
    bool initialized;
    
    // Statistics
    uint32_t totalMessages;
    uint32_t droppedMessages;
    
    // Singleton instance
    static SystemLogger* instance;

public:
    /**
     * @brief Constructor
     */
    SystemLogger(size_t maxEntries = DEFAULT_MAX_ENTRIES, LogLevel minLevel = LogLevel::DEBUG);
    
    /**
     * @brief Get singleton instance
     */
    static SystemLogger* getInstance();
    
    /**
     * @brief Initialize the logging system
     */
    void begin();
    
    /**
     * @brief Log a message
     */
    void log(LogLevel level, const String& component, const String& message);
    
    /**
     * @brief Log formatted message (printf style)
     */
    void logf(LogLevel level, const String& component, const char* format, ...);
    
    /**
     * @brief Get recent log entries
     * @param count Number of entries to retrieve (0 = all)
     * @param minLevel Minimum log level to include
     * @return Vector of log entries
     */
    std::vector<LogEntry> getRecentEntries(size_t count = 0, LogLevel minLevel = LogLevel::DEBUG) const;
    
    /**
     * @brief Get log entries as formatted string for console display
     */
    String getFormattedLog(size_t count = 50, LogLevel minLevel = LogLevel::DEBUG) const;
    
    /**
     * @brief Get log entries as JSON for web API
     */
    String getJSONLog(size_t count = WEB_LOG_LIMIT, LogLevel minLevel = LogLevel::DEBUG) const;
    
    /**
     * @brief Get logging statistics
     */
    struct Stats {
        uint32_t totalMessages;
        uint32_t droppedMessages;
        uint32_t currentEntries;
        uint32_t maxEntries;
        uint32_t uptimeMs;
    };
    Stats getStats() const;
    
    /**
     * @brief Set minimum log level
     */
    void setMinLevel(LogLevel level) { minLevel = level; }
    
    /**
     * @brief Get minimum log level
     */
    LogLevel getMinLevel() const { return minLevel; }
    
    /**
     * @brief Clear all log entries
     */
    void clear();
    
    /**
     * @brief Convert log level to string
     */
    static const char* levelToString(LogLevel level);
    
    /**
     * @brief Convert log level from string
     */
    static LogLevel stringToLevel(const String& str);

private:
    void addEntry(LogLevel level, const String& component, const String& message);
    String formatTimestamp(uint32_t timestamp) const;
};

// Global logger instance access
extern SystemLogger* g_logger;

// Convenience macros for logging
#define LOG_DEBUG(component, message) \
    do { if (g_logger) g_logger->log(LogLevel::DEBUG, component, message); } while(0)

#define LOG_INFO(component, message) \
    do { if (g_logger) g_logger->log(LogLevel::INFO, component, message); } while(0)

#define LOG_WARNING(component, message) \
    do { if (g_logger) g_logger->log(LogLevel::WARNING, component, message); } while(0)

#define LOG_ERROR(component, message) \
    do { if (g_logger) g_logger->log(LogLevel::ERROR, component, message); } while(0)

#define LOG_CRITICAL(component, message) \
    do { if (g_logger) g_logger->log(LogLevel::CRITICAL, component, message); } while(0)

// Printf-style logging macros
#define LOG_DEBUGF(component, format, ...) \
    do { if (g_logger) g_logger->logf(LogLevel::DEBUG, component, format, ##__VA_ARGS__); } while(0)

#define LOG_INFOF(component, format, ...) \
    do { if (g_logger) g_logger->logf(LogLevel::INFO, component, format, ##__VA_ARGS__); } while(0)

#define LOG_WARNINGF(component, format, ...) \
    do { if (g_logger) g_logger->logf(LogLevel::WARNING, component, format, ##__VA_ARGS__); } while(0)

#define LOG_ERRORF(component, format, ...) \
    do { if (g_logger) g_logger->logf(LogLevel::ERROR, component, format, ##__VA_ARGS__); } while(0)

#define LOG_CRITICALF(component, format, ...) \
    do { if (g_logger) g_logger->logf(LogLevel::CRITICAL, component, format, ##__VA_ARGS__); } while(0)

// Boot message helpers
#define LOG_BOOT_INFO(message) LOG_INFO("BOOT", message)
#define LOG_BOOT_ERROR(message) LOG_ERROR("BOOT", message)
#define LOG_BOOT_SUCCESS(message) LOG_INFO("BOOT", "✓ " + String(message))
#define LOG_BOOT_FAILURE(message) LOG_ERROR("BOOT", "✗ " + String(message))

// Component-specific helpers
#define LOG_WIFI_INFO(message) LOG_INFO("WiFi", message)
#define LOG_WIFI_ERROR(message) LOG_ERROR("WiFi", message)
#define LOG_LORA_INFO(message) LOG_INFO("LoRa", message)
#define LOG_LORA_ERROR(message) LOG_ERROR("LoRa", message)
#define LOG_WEB_INFO(message) LOG_INFO("Web", message)
#define LOG_WEB_ERROR(message) LOG_ERROR("Web", message)
#define LOG_GNSS_INFO(message) LOG_INFO("GNSS", message)
#define LOG_GNSS_ERROR(message) LOG_ERROR("GNSS", message)
#define LOG_KISS_INFO(message) LOG_INFO("KISS", message)
#define LOG_KISS_ERROR(message) LOG_ERROR("KISS", message)