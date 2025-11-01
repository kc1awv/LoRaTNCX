/**
 * @file SystemAPIHandler.cpp
 * @brief Implementation of system API handler
 */

#include "SystemAPIHandler.h"
#include "SystemLogger.h"
#include <WiFi.h>
#include <esp_system.h>

SystemAPIHandler::SystemAPIHandler() : APIHandler("/api/system")
{
}

void SystemAPIHandler::registerRoutes(AsyncWebServer& server)
{
    // System status endpoint
    server.on((basePath + "/status").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleSystemStatus(request);
        });

    // System information endpoint
    server.on((basePath + "/info").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleSystemInfo(request);
        });

    // Performance metrics endpoint
    server.on((basePath + "/performance").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handlePerformanceInfo(request);
        });

    // System logs endpoint
    server.on((basePath + "/logs").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleSystemLogs(request);
        });

    // OPTIONS handler for CORS
    server.on((basePath + "/*").c_str(), HTTP_OPTIONS, 
        [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            this->setCORSHeaders(response);
            request->send(response);
        });
}

void SystemAPIHandler::setSystemStatusCallback(std::function<String()> callback)
{
    getSystemStatusCallback = callback;
}

void SystemAPIHandler::handleSystemStatus(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    if (getSystemStatusCallback) {
        String statusStr = getSystemStatusCallback();
        
        // Try to parse the callback result as JSON
        JsonDocument callbackDoc;
        DeserializationError error = deserializeJson(callbackDoc, statusStr);
        
        if (error == DeserializationError::Ok) {
            dataDoc = callbackDoc;
        } else {
            // If it's not JSON, treat it as a plain string
            dataDoc["status_text"] = statusStr;
        }
    } else {
        dataDoc["status_text"] = "System operational";
    }

    // Add basic system metrics
    dataDoc["uptime_ms"] = millis();
    dataDoc["free_heap"] = ESP.getFreeHeap();
    dataDoc["total_heap"] = ESP.getHeapSize();
    dataDoc["min_free_heap"] = ESP.getMinFreeHeap();

    sendSuccess(request, dataDoc, "System status retrieved successfully");
}

void SystemAPIHandler::handleSystemInfo(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    // Hardware information
    dataDoc["chip_model"] = ESP.getChipModel();
    dataDoc["chip_revision"] = ESP.getChipRevision();
    dataDoc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    dataDoc["flash_size"] = ESP.getFlashChipSize();
    dataDoc["flash_speed"] = ESP.getFlashChipSpeed();
    
    // SDK and firmware info
    dataDoc["sdk_version"] = ESP.getSdkVersion();
    dataDoc["core_version"] = ESP.getCoreVersion();
    
    // MAC addresses
    dataDoc["wifi_mac"] = WiFi.macAddress();
    dataDoc["bt_mac"] = WiFi.macAddress(); // ESP32 uses same for both typically
    
    sendSuccess(request, dataDoc, "System information retrieved successfully");
}

void SystemAPIHandler::handlePerformanceInfo(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    // Memory information
    JsonObject memory = dataDoc["memory"].to<JsonObject>();
    memory["free_heap"] = ESP.getFreeHeap();
    memory["total_heap"] = ESP.getHeapSize();
    memory["min_free_heap"] = ESP.getMinFreeHeap();
    memory["max_alloc_heap"] = ESP.getMaxAllocHeap();
    
    // Timing information
    JsonObject timing = dataDoc["timing"].to<JsonObject>();
    timing["uptime_ms"] = millis();
    timing["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    
    // Reset information
    JsonObject reset = dataDoc["reset"].to<JsonObject>();
    reset["reason"] = esp_reset_reason();
    reset["wakeup_cause"] = esp_sleep_get_wakeup_cause();
    
    sendSuccess(request, dataDoc, "Performance metrics retrieved successfully");
}

void SystemAPIHandler::handleSystemLogs(AsyncWebServerRequest* request)
{
    SystemLogger* logger = SystemLogger::getInstance();
    if (!logger) {
        sendError(request, "Logging system not available", 503);
        return;
    }
    
    // Parse query parameters
    size_t count = 100;  // Default count
    LogLevel minLevel = LogLevel::DEBUG;  // Default level
    
    if (request->hasParam("count")) {
        String countStr = request->getParam("count")->value();
        int parsedCount = countStr.toInt();
        if (parsedCount > 0 && parsedCount <= 1000) {
            count = parsedCount;
        }
    }
    
    if (request->hasParam("level")) {
        String levelStr = request->getParam("level")->value();
        minLevel = SystemLogger::stringToLevel(levelStr);
    }
    
    // Handle special flags
    if (request->hasParam("all") && request->getParam("all")->value() == "true") {
        count = 0;  // Get all entries
    }
    
    // Get log entries
    auto entries = logger->getRecentEntries(count, minLevel);
    
    JsonDocument dataDoc;
    JsonArray logsArray = dataDoc["logs"].to<JsonArray>();
    
    for (const auto& entry : entries) {
        JsonObject logEntry = logsArray.add<JsonObject>();
        logEntry["timestamp"] = entry.timestamp;
        logEntry["level"] = SystemLogger::levelToString(entry.level);
        logEntry["component"] = entry.component;
        logEntry["message"] = entry.message;
        
        // Add human-readable timestamp
        uint32_t seconds = entry.timestamp / 1000;
        uint32_t millis_part = entry.timestamp % 1000;
        uint32_t hours = seconds / 3600;
        seconds %= 3600;
        uint32_t minutes = seconds / 60;
        seconds %= 60;
        
        char timeStr[32];
        if (hours > 0) {
            snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu.%03lu", 
                    hours, minutes, seconds, millis_part);
        } else {
            snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu.%03lu", 
                    minutes, seconds, millis_part);
        }
        logEntry["time_formatted"] = String(timeStr);
    }
    
    // Add statistics
    auto stats = logger->getStats();
    JsonObject statsObj = dataDoc["stats"].to<JsonObject>();
    statsObj["total_messages"] = stats.totalMessages;
    statsObj["dropped_messages"] = stats.droppedMessages;
    statsObj["current_entries"] = stats.currentEntries;
    statsObj["max_capacity"] = stats.maxEntries;
    statsObj["uptime_ms"] = stats.uptimeMs;
    
    // Add metadata
    dataDoc["requested_count"] = count;
    dataDoc["requested_level"] = SystemLogger::levelToString(minLevel);
    dataDoc["returned_count"] = entries.size();
    
    sendSuccess(request, dataDoc, "Log entries retrieved successfully");
}