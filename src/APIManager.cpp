/**
 * @file APIManager.cpp
 * @brief Implementation of central API management system
 */

#include "APIManager.h"
#include "SystemLogger.h"
#include <SPIFFS.h>
#include <WiFi.h>

APIManager::APIManager() : initialized(false)
{
}

bool APIManager::begin()
{
    if (initialized) {
        return true;
    }

    LOG_BOOT_INFO("Initializing API management system...");

    // Create API handlers
    systemHandler = std::make_unique<SystemAPIHandler>();
    loraHandler = std::make_unique<LoRaAPIHandler>();
    wifiHandler = std::make_unique<WiFiAPIHandler>();

    // Store handlers in vector for easy iteration
    handlers.clear();
    // Note: We can't add unique_ptrs directly to the vector since they're already stored
    // in specific member variables. The handlers vector would be for additional handlers.

    initialized = true;
    LOG_BOOT_SUCCESS("API management system initialized");
    
    return true;
}

void APIManager::registerRoutes(AsyncWebServer& server)
{
    if (!initialized) {
        LOG_ERROR("Cannot register routes - API manager not initialized");
        return;
    }

    LOG_BOOT_INFO("Registering API routes...");

    // Register all handler routes
    systemHandler->registerRoutes(server);
    loraHandler->registerRoutes(server);
    wifiHandler->registerRoutes(server);

    // Register additional handlers from vector
    for (auto& handler : handlers) {
        handler->registerRoutes(server);
    }

    // Setup debug and utility routes
    setupDebugRoutes(server);

    LOG_BOOT_SUCCESS("API routes registered successfully");
}

void APIManager::setSystemStatusCallback(std::function<String()> callback)
{
    if (systemHandler) {
        systemHandler->setSystemStatusCallback(callback);
    }
}

void APIManager::setLoRaStatusCallback(std::function<String()> callback)
{
    if (loraHandler) {
        loraHandler->setLoRaStatusCallback(callback);
    }
}

void APIManager::setLoRaConfigCallback(std::function<bool(const String&, const String&, String&)> callback)
{
    if (loraHandler) {
        loraHandler->setLoRaConfigCallback(callback);
    }
}

void APIManager::setWiFiNetworksCallback(std::function<String()> callback)
{
    if (wifiHandler) {
        wifiHandler->setWiFiNetworksCallback(callback);
    }
}

void APIManager::setWiFiStatusCallback(std::function<String()> callback)
{
    if (wifiHandler) {
        wifiHandler->setWiFiStatusCallback(callback);
    }
}

void APIManager::setAddWiFiNetworkCallback(std::function<bool(const String&, const String&, String&)> callback)
{
    if (wifiHandler) {
        wifiHandler->setAddWiFiNetworkCallback(callback);
    }
}

void APIManager::setRemoveWiFiNetworkCallback(std::function<bool(const String&, String&)> callback)
{
    if (wifiHandler) {
        wifiHandler->setRemoveWiFiNetworkCallback(callback);
    }
}

void APIManager::setupDebugRoutes(AsyncWebServer& server)
{
    // API root endpoint - provides API overview
    server.on("/api", HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleAPIRoot(request);
        });

    // Debug files list endpoint
    server.on("/api/debug/files", HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleListFiles(request);
        });

    // Global CORS OPTIONS handler for any /api/* endpoint
    server.on("/api/*", HTTP_OPTIONS, 
        [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            response->addHeader("Access-Control-Max-Age", "86400");
            request->send(response);
        });
}

void APIManager::handleAPIRoot(AsyncWebServerRequest* request)
{
    JsonDocument doc;
    doc["status"] = "success";
    doc["message"] = "LoRaTNCX API Server";
    doc["version"] = "1.0";
    doc["timestamp"] = millis();
    
    // API endpoints documentation
    JsonObject endpoints = doc["endpoints"].to<JsonObject>();
    
    JsonObject systemEndpoints = endpoints["system"].to<JsonObject>();
    systemEndpoints["status"] = "GET /api/system/status";
    systemEndpoints["info"] = "GET /api/system/info";
    systemEndpoints["performance"] = "GET /api/system/performance";
    systemEndpoints["logs"] = "GET /api/system/logs?count=N&level=LEVEL&all=true";
    
    JsonObject loraEndpoints = endpoints["lora"].to<JsonObject>();
    loraEndpoints["status"] = "GET /api/lora/status";
    loraEndpoints["config"] = "GET/POST /api/lora/config";
    loraEndpoints["stats"] = "GET /api/lora/stats";
    
    JsonObject wifiEndpoints = endpoints["wifi"].to<JsonObject>();
    wifiEndpoints["networks"] = "GET /api/wifi/networks";
    wifiEndpoints["status"] = "GET /api/wifi/status";
    wifiEndpoints["scan"] = "POST /api/wifi/scan";
    wifiEndpoints["add"] = "POST /api/wifi/add";
    wifiEndpoints["remove"] = "POST /api/wifi/remove";
    wifiEndpoints["connect"] = "POST /api/wifi/connect";
    wifiEndpoints["disconnect"] = "POST /api/wifi/disconnect";
    
    JsonObject debugEndpoints = endpoints["debug"].to<JsonObject>();
    debugEndpoints["files"] = "GET /api/debug/files";

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", jsonString);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void APIManager::handleListFiles(AsyncWebServerRequest* request)
{
    JsonDocument doc;
    doc["status"] = "success";
    doc["timestamp"] = millis();
    
    JsonArray files = doc["files"].to<JsonArray>();
    
    if (SPIFFS.begin()) {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file) {
            JsonObject fileObj = files.add<JsonObject>();
            fileObj["name"] = file.name();
            fileObj["size"] = file.size();
            file = root.openNextFile();
        }
        
        doc["message"] = "Files listed successfully";
    } else {
        doc["status"] = "error";
        doc["message"] = "SPIFFS not available";
    }

    String jsonString;
    serializeJson(doc, jsonString);
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", jsonString);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}