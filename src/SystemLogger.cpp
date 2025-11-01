/**
 * @file SystemLogger.cpp
 * @brief SystemLogger implementation
 * @author LoRaTNCX Project
 * @date November 1, 2025
 */

#include "SystemLogger.h"
#include <cstdarg>

// Static instance
SystemLogger* SystemLogger::instance = nullptr;
SystemLogger* g_logger = nullptr;

SystemLogger::SystemLogger(size_t maxEntries, LogLevel minLevel)
    : maxEntries(maxEntries), minLevel(minLevel), initialized(false),
      totalMessages(0), droppedMessages(0) {
    logEntries.clear();
}

SystemLogger* SystemLogger::getInstance() {
    if (!instance) {
        instance = new SystemLogger();
        g_logger = instance;
    }
    return instance;
}

void SystemLogger::begin() {
    if (initialized) {
        return;
    }
    
    initialized = true;
    
    // Log the start of the logging system
    log(LogLevel::INFO, "SYSTEM", "SystemLogger initialized");
    logf(LogLevel::INFO, "SYSTEM", "Log capacity: %u entries, Min level: %s", 
         maxEntries, levelToString(minLevel));
}

void SystemLogger::log(LogLevel level, const String& component, const String& message) {
    if (!initialized || level < minLevel) {
        return;
    }
    
    addEntry(level, component, message);
}

void SystemLogger::logf(LogLevel level, const String& component, const char* format, ...) {
    if (!initialized || level < minLevel) {
        return;
    }
    
    char buffer[512];  // Reasonable size for log messages
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    addEntry(level, component, String(buffer));
}

void SystemLogger::addEntry(LogLevel level, const String& component, const String& message) {
    totalMessages++;
    
    // Create new entry
    LogEntry entry(millis(), level, component, message);
    
    // Add to deque
    logEntries.push_back(entry);
    
    // Maintain size limit
    if (logEntries.size() > maxEntries) {
        logEntries.pop_front();
        droppedMessages++;
    }
}

std::vector<LogEntry> SystemLogger::getRecentEntries(size_t count, LogLevel minLevel) const {
    std::vector<LogEntry> result;
    
    if (!initialized || logEntries.empty()) {
        return result;
    }
    
    // If count is 0, return all entries
    if (count == 0) {
        count = logEntries.size();
    }
    
    // Reserve space for efficiency
    result.reserve(std::min(count, logEntries.size()));
    
    // Get entries from the end (most recent first)
    auto startIt = logEntries.end();
    size_t available = logEntries.size();
    size_t toSkip = (count < available) ? (available - count) : 0;
    
    if (toSkip > 0) {
        startIt -= (available - toSkip);
    } else {
        startIt = logEntries.begin();
    }
    
    // Filter by minimum level and add to result
    for (auto it = startIt; it != logEntries.end(); ++it) {
        if (it->level >= minLevel) {
            result.push_back(*it);
        }
    }
    
    return result;
}

String SystemLogger::getFormattedLog(size_t count, LogLevel minLevel) const {
    auto entries = getRecentEntries(count, minLevel);
    
    if (entries.empty()) {
        return "No log entries available.\n";
    }
    
    String result;
    result.reserve(entries.size() * 80);  // Rough estimate
    
    result += "System Log (last " + String(entries.size()) + " entries):\n";
    result += "============================================\n";
    
    for (const auto& entry : entries) {
        result += formatTimestamp(entry.timestamp);
        result += " [" + String(levelToString(entry.level)) + "] ";
        result += entry.component + ": " + entry.message + "\n";
    }
    
    return result;
}

String SystemLogger::getJSONLog(size_t count, LogLevel minLevel) const {
    auto entries = getRecentEntries(count, minLevel);
    
    String json = "{\"status\":\"ok\",\"entries\":[";
    
    for (size_t i = 0; i < entries.size(); i++) {
        if (i > 0) json += ",";
        
        const auto& entry = entries[i];
        json += "{";
        json += "\"timestamp\":" + String(entry.timestamp) + ",";
        json += "\"level\":\"" + String(levelToString(entry.level)) + "\",";
        json += "\"component\":\"" + entry.component + "\",";
        json += "\"message\":\"" + entry.message + "\"";
        json += "}";
    }
    
    json += "],\"stats\":{";
    auto stats = getStats();
    json += "\"total\":" + String(stats.totalMessages) + ",";
    json += "\"dropped\":" + String(stats.droppedMessages) + ",";
    json += "\"current\":" + String(stats.currentEntries) + ",";
    json += "\"uptime\":" + String(stats.uptimeMs);
    json += "}}";
    
    return json;
}

SystemLogger::Stats SystemLogger::getStats() const {
    Stats stats;
    stats.totalMessages = totalMessages;
    stats.droppedMessages = droppedMessages;
    stats.currentEntries = logEntries.size();
    stats.maxEntries = maxEntries;
    stats.uptimeMs = millis();
    return stats;
}

void SystemLogger::clear() {
    logEntries.clear();
    // Don't reset totalMessages and droppedMessages - they are cumulative
    
    log(LogLevel::INFO, "SYSTEM", "Log entries cleared");
}

const char* SystemLogger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

LogLevel SystemLogger::stringToLevel(const String& str) {
    String upper = str;
    upper.toUpperCase();
    
    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO") return LogLevel::INFO;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::WARNING;
    if (upper == "ERROR") return LogLevel::ERROR;
    if (upper == "CRIT" || upper == "CRITICAL") return LogLevel::CRITICAL;
    
    return LogLevel::INFO;  // Default
}

String SystemLogger::formatTimestamp(uint32_t timestamp) const {
    uint32_t seconds = timestamp / 1000;
    uint32_t millis_part = timestamp % 1000;
    
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    seconds %= 60;
    
    char buffer[16];
    if (hours > 0) {
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu.%03lu", 
                hours, minutes, seconds, millis_part);
    } else {
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu.%03lu", 
                minutes, seconds, millis_part);
    }
    
    return String(buffer);
}