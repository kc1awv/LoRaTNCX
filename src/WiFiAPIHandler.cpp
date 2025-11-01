/**
 * @file WiFiAPIHandler.cpp
 * @brief Implementation of WiFi API handler
 */

#include "WiFiAPIHandler.h"
#include <WiFi.h>

WiFiAPIHandler::WiFiAPIHandler() : APIHandler("/api/wifi")
{
}

void WiFiAPIHandler::registerRoutes(AsyncWebServer& server)
{
    // WiFi networks list endpoint
    server.on((basePath + "/networks").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleWiFiNetworks(request);
        });

    // WiFi status endpoint
    server.on((basePath + "/status").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleWiFiStatus(request);
        });

    // Add WiFi network endpoint
    server.on((basePath + "/add").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleAddWiFiNetwork(request);
        });

    // Remove WiFi network endpoint
    server.on((basePath + "/remove").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleRemoveWiFiNetwork(request);
        });

    // WiFi scan endpoint
    server.on((basePath + "/scan").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleWiFiScan(request);
        });

    // WiFi connect endpoint
    server.on((basePath + "/connect").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleWiFiConnect(request);
        });

    // WiFi disconnect endpoint
    server.on((basePath + "/disconnect").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleWiFiDisconnect(request);
        });

    // OPTIONS handler for CORS
    server.on((basePath + "/*").c_str(), HTTP_OPTIONS, 
        [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            this->setCORSHeaders(response);
            request->send(response);
        });
}

void WiFiAPIHandler::setWiFiNetworksCallback(std::function<String()> callback)
{
    getWiFiNetworksCallback = callback;
}

void WiFiAPIHandler::setAddWiFiNetworkCallback(std::function<bool(const String&, const String&, String&)> callback)
{
    addWiFiNetworkCallback = callback;
}

void WiFiAPIHandler::setRemoveWiFiNetworkCallback(std::function<bool(const String&, String&)> callback)
{
    removeWiFiNetworkCallback = callback;
}

void WiFiAPIHandler::setWiFiStatusCallback(std::function<String()> callback)
{
    getWiFiStatusCallback = callback;
}

void WiFiAPIHandler::handleWiFiNetworks(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    if (getWiFiNetworksCallback) {
        String networksStr = getWiFiNetworksCallback();
        
        // Try to parse the callback result as JSON
        JsonDocument callbackDoc;
        DeserializationError error = deserializeJson(callbackDoc, networksStr);
        
        if (error == DeserializationError::Ok) {
            dataDoc = callbackDoc;
        } else {
            // If it's not JSON, treat it as a plain string
            dataDoc["networks_text"] = networksStr;
        }
    } else {
        sendError(request, "WiFi networks callback not available", 503);
        return;
    }

    sendSuccess(request, dataDoc, "WiFi networks retrieved successfully");
}

void WiFiAPIHandler::handleWiFiStatus(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    if (getWiFiStatusCallback) {
        String statusStr = getWiFiStatusCallback();
        
        // Try to parse the callback result as JSON
        JsonDocument callbackDoc;
        DeserializationError error = deserializeJson(callbackDoc, statusStr);
        
        if (error == DeserializationError::Ok) {
            dataDoc = callbackDoc;
        } else {
            // If it's not JSON, create basic status structure
            dataDoc["status_text"] = statusStr;
        }
    } else {
        // Provide basic WiFi status without callback
        dataDoc["connected"] = WiFi.isConnected();
        dataDoc["ssid"] = WiFi.SSID();
        dataDoc["ip_address"] = WiFi.localIP().toString();
        dataDoc["mac_address"] = WiFi.macAddress();
        dataDoc["rssi"] = WiFi.RSSI();
    }

    sendSuccess(request, dataDoc, "WiFi status retrieved successfully");
}

void WiFiAPIHandler::handleAddWiFiNetwork(AsyncWebServerRequest* request)
{
    std::vector<String> requiredParams = {"ssid", "password"};
    
    if (!validateRequiredParams(request, requiredParams)) {
        sendError(request, "Missing required parameters: ssid and password");
        return;
    }
    
    if (!addWiFiNetworkCallback) {
        sendError(request, "Add WiFi network callback not available", 503);
        return;
    }
    
    String ssid = getParam(request, "ssid");
    String password = getParam(request, "password");
    String message;
    
    if (addWiFiNetworkCallback(ssid, password, message)) {
        JsonDocument dataDoc;
        dataDoc["ssid"] = ssid;
        sendSuccess(request, dataDoc, message);
    } else {
        sendError(request, message);
    }
}

void WiFiAPIHandler::handleRemoveWiFiNetwork(AsyncWebServerRequest* request)
{
    std::vector<String> requiredParams = {"ssid"};
    
    if (!validateRequiredParams(request, requiredParams)) {
        sendError(request, "Missing required parameter: ssid");
        return;
    }
    
    if (!removeWiFiNetworkCallback) {
        sendError(request, "Remove WiFi network callback not available", 503);
        return;
    }
    
    String ssid = getParam(request, "ssid");
    String message;
    
    if (removeWiFiNetworkCallback(ssid, message)) {
        JsonDocument dataDoc;
        dataDoc["ssid"] = ssid;
        sendSuccess(request, dataDoc, message);
    } else {
        sendError(request, message);
    }
}

void WiFiAPIHandler::handleWiFiScan(AsyncWebServerRequest* request)
{
    // Trigger a WiFi scan and return results
    JsonDocument dataDoc;
    
    // Start scan
    int networkCount = WiFi.scanNetworks();
    
    if (networkCount > 0) {
        JsonArray networks = dataDoc["scan_results"].to<JsonArray>();
        
        for (int i = 0; i < networkCount; i++) {
            JsonObject network = networks.add<JsonObject>();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
            network["channel"] = WiFi.channel(i);
        }
        
        dataDoc["count"] = networkCount;
        sendSuccess(request, dataDoc, "WiFi scan completed successfully");
    } else {
        sendError(request, "No networks found or scan failed");
    }
    
    // Clean up scan results
    WiFi.scanDelete();
}

void WiFiAPIHandler::handleWiFiConnect(AsyncWebServerRequest* request)
{
    std::vector<String> requiredParams = {"ssid"};
    
    if (!validateRequiredParams(request, requiredParams)) {
        sendError(request, "Missing required parameter: ssid");
        return;
    }
    
    String ssid = getParam(request, "ssid");
    String password = getParam(request, "password");
    
    JsonDocument dataDoc;
    dataDoc["ssid"] = ssid;
    dataDoc["action"] = "connect_requested";
    
    sendSuccess(request, dataDoc, "WiFi connection request submitted");
}

void WiFiAPIHandler::handleWiFiDisconnect(AsyncWebServerRequest* request)
{
    WiFi.disconnect();
    
    JsonDocument dataDoc;
    dataDoc["action"] = "disconnect_requested";
    
    sendSuccess(request, dataDoc, "WiFi disconnect requested");
}