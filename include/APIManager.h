/**
 * @file APIManager.h
 * @brief Central API management system
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <memory>
#include <vector>
#include "APIHandler.h"
#include "SystemAPIHandler.h"
#include "LoRaAPIHandler.h"
#include "WiFiAPIHandler.h"

/**
 * @brief Central API management system
 * 
 * Manages all API handlers and provides a unified interface for
 * registering callbacks and handling API requests.
 */
class APIManager
{
public:
    /**
     * @brief Constructor
     */
    APIManager();

    /**
     * @brief Destructor
     */
    ~APIManager() = default;

    /**
     * @brief Initialize the API manager
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Register all API routes with the web server
     * @param server Reference to the AsyncWebServer instance
     */
    void registerRoutes(AsyncWebServer& server);

    /**
     * @brief Get the system API handler
     * @return Pointer to SystemAPIHandler
     */
    SystemAPIHandler* getSystemHandler() { return systemHandler.get(); }

    /**
     * @brief Get the LoRa API handler
     * @return Pointer to LoRaAPIHandler
     */
    LoRaAPIHandler* getLoRaHandler() { return loraHandler.get(); }

    /**
     * @brief Get the WiFi API handler
     * @return Pointer to WiFiAPIHandler
     */
    WiFiAPIHandler* getWiFiHandler() { return wifiHandler.get(); }

    /**
     * @brief Set system status callback for all relevant handlers
     * @param callback Function that returns system status as JSON string
     */
    void setSystemStatusCallback(std::function<String()> callback);

    /**
     * @brief Set LoRa status callback
     * @param callback Function that returns LoRa status as JSON string
     */
    void setLoRaStatusCallback(std::function<String()> callback);

    /**
     * @brief Set LoRa configuration callback
     * @param callback Function that accepts parameter, value, and returns success with message
     */
    void setLoRaConfigCallback(std::function<bool(const String&, const String&, String&)> callback);

    /**
     * @brief Set WiFi networks callback
     * @param callback Function that returns WiFi networks as JSON string
     */
    void setWiFiNetworksCallback(std::function<String()> callback);

    /**
     * @brief Set WiFi status callback
     * @param callback Function that returns WiFi status as JSON string
     */
    void setWiFiStatusCallback(std::function<String()> callback);

    /**
     * @brief Set add WiFi network callback
     * @param callback Function that accepts SSID, password, and returns success with message
     */
    void setAddWiFiNetworkCallback(std::function<bool(const String&, const String&, String&)> callback);

    /**
     * @brief Set remove WiFi network callback
     * @param callback Function that accepts SSID and returns success with message
     */
    void setRemoveWiFiNetworkCallback(std::function<bool(const String&, String&)> callback);

private:
    std::unique_ptr<SystemAPIHandler> systemHandler;
    std::unique_ptr<LoRaAPIHandler> loraHandler;
    std::unique_ptr<WiFiAPIHandler> wifiHandler;
    
    std::vector<std::unique_ptr<APIHandler>> handlers;
    bool initialized;

    /**
     * @brief Setup debug API endpoints
     * @param server Reference to the AsyncWebServer instance
     */
    void setupDebugRoutes(AsyncWebServer& server);

    /**
     * @brief Handle debug files list endpoint
     */
    void handleListFiles(AsyncWebServerRequest* request);

    /**
     * @brief Handle API root endpoint
     */
    void handleAPIRoot(AsyncWebServerRequest* request);
};