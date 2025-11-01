/**
 * @file SystemAPIHandler.cpp
 * @brief Implementation of system API handler
 */

#include "SystemAPIHandler.h"
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