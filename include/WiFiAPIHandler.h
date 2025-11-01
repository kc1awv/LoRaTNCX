/**
 * @file WiFiAPIHandler.h
 * @brief WiFi management API handler
 */

#pragma once

#include "APIHandler.h"
#include <functional>

/**
 * @brief Handles WiFi-related API endpoints
 * 
 * Provides endpoints for:
 * - WiFi network scanning and listing
 * - Network connection management
 * - WiFi status and configuration
 * - Access point management
 */
class WiFiAPIHandler : public APIHandler
{
public:
    /**
     * @brief Constructor
     */
    WiFiAPIHandler();

    /**
     * @brief Register WiFi API routes
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Set callback for getting available WiFi networks
     * @param callback Function that returns WiFi networks as JSON string
     */
    void setWiFiNetworksCallback(std::function<String()> callback);

    /**
     * @brief Set callback for adding a WiFi network
     * @param callback Function that accepts SSID, password, and returns success with message
     */
    void setAddWiFiNetworkCallback(std::function<bool(const String&, const String&, String&)> callback);

    /**
     * @brief Set callback for removing a WiFi network
     * @param callback Function that accepts SSID and returns success with message
     */
    void setRemoveWiFiNetworkCallback(std::function<bool(const String&, String&)> callback);

    /**
     * @brief Set callback for getting WiFi status
     * @param callback Function that returns WiFi status as JSON string
     */
    void setWiFiStatusCallback(std::function<String()> callback);

private:
    std::function<String()> getWiFiNetworksCallback;
    std::function<bool(const String&, const String&, String&)> addWiFiNetworkCallback;
    std::function<bool(const String&, String&)> removeWiFiNetworkCallback;
    std::function<String()> getWiFiStatusCallback;

    /**
     * @brief Handle GET /api/wifi/networks
     */
    void handleWiFiNetworks(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/wifi/status
     */
    void handleWiFiStatus(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/wifi/add
     */
    void handleAddWiFiNetwork(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/wifi/remove
     */
    void handleRemoveWiFiNetwork(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/wifi/scan
     */
    void handleWiFiScan(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/wifi/connect
     */
    void handleWiFiConnect(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/wifi/disconnect
     */
    void handleWiFiDisconnect(AsyncWebServerRequest* request);
};