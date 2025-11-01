/**
 * @file LoRaAPIHandler.cpp
 * @brief Implementation of LoRa API handler
 */

#include "LoRaAPIHandler.h"

LoRaAPIHandler::LoRaAPIHandler() : APIHandler("/api/lora")
{
}

void LoRaAPIHandler::registerRoutes(AsyncWebServer& server)
{
    // LoRa status endpoint
    server.on((basePath + "/status").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleLoRaStatus(request);
        });

    // LoRa configuration GET endpoint
    server.on((basePath + "/config").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleLoRaConfig(request);
        });

    // LoRa configuration SET endpoint
    server.on((basePath + "/config").c_str(), HTTP_POST, 
        [this](AsyncWebServerRequest* request) {
            this->handleSetLoRaConfig(request);
        });

    // LoRa statistics endpoint
    server.on((basePath + "/stats").c_str(), HTTP_GET, 
        [this](AsyncWebServerRequest* request) {
            this->handleLoRaStats(request);
        });

    // OPTIONS handler for CORS
    server.on((basePath + "/*").c_str(), HTTP_OPTIONS, 
        [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            this->setCORSHeaders(response);
            request->send(response);
        });
}

void LoRaAPIHandler::setLoRaStatusCallback(std::function<String()> callback)
{
    getLoRaStatusCallback = callback;
}

void LoRaAPIHandler::setLoRaConfigCallback(std::function<bool(const String&, const String&, String&)> callback)
{
    loraConfigCallback = callback;
}

void LoRaAPIHandler::handleLoRaStatus(AsyncWebServerRequest* request)
{
    JsonDocument dataDoc;
    
    if (getLoRaStatusCallback) {
        String statusStr = getLoRaStatusCallback();
        
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
        sendError(request, "LoRa status callback not available", 503);
        return;
    }

    sendSuccess(request, dataDoc, "LoRa status retrieved successfully");
}

void LoRaAPIHandler::handleLoRaConfig(AsyncWebServerRequest* request)
{
    // This would typically get current LoRa configuration
    // For now, return basic info indicating config endpoint is available
    JsonDocument dataDoc;
    dataDoc["message"] = "LoRa configuration endpoint available";
    dataDoc["supported_params"] = JsonArray();
    dataDoc["supported_params"].add("frequency");
    dataDoc["supported_params"].add("bandwidth");
    dataDoc["supported_params"].add("spreading_factor");
    dataDoc["supported_params"].add("coding_rate");
    dataDoc["supported_params"].add("tx_power");
    
    sendSuccess(request, dataDoc, "LoRa configuration info retrieved");
}

void LoRaAPIHandler::handleSetLoRaConfig(AsyncWebServerRequest* request)
{
    std::vector<String> requiredParams = {"parameter", "value"};
    
    if (!validateRequiredParams(request, requiredParams)) {
        sendError(request, "Missing required parameters: parameter and value");
        return;
    }
    
    if (!loraConfigCallback) {
        sendError(request, "LoRa configuration callback not available", 503);
        return;
    }
    
    String parameter = getParam(request, "parameter");
    String value = getParam(request, "value");
    String message;
    
    if (loraConfigCallback(parameter, value, message)) {
        JsonDocument dataDoc;
        dataDoc["parameter"] = parameter;
        dataDoc["value"] = value;
        sendSuccess(request, dataDoc, message);
    } else {
        sendError(request, message);
    }
}

void LoRaAPIHandler::handleLoRaStats(AsyncWebServerRequest* request)
{
    // Basic LoRa statistics - this would be enhanced with real data
    JsonDocument dataDoc;
    
    JsonObject packets = dataDoc["packets"].to<JsonObject>();
    packets["transmitted"] = 0;
    packets["received"] = 0;
    packets["errors"] = 0;
    
    JsonObject signal = dataDoc["signal"].to<JsonObject>();
    signal["rssi"] = 0;
    signal["snr"] = 0.0;
    
    JsonObject radio = dataDoc["radio"].to<JsonObject>();
    radio["frequency"] = 915000000;  // Example frequency
    radio["bandwidth"] = 125000;     // Example bandwidth
    radio["spreading_factor"] = 7;   // Example SF
    
    sendSuccess(request, dataDoc, "LoRa statistics retrieved");
}