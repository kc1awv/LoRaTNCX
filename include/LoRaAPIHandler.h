/**
 * @file LoRaAPIHandler.h
 * @brief LoRa radio API handler
 */

#pragma once

#include "APIHandler.h"
#include <functional>

/**
 * @brief Handles LoRa-related API endpoints
 * 
 * Provides endpoints for:
 * - LoRa status and configuration
 * - Radio parameters (frequency, bandwidth, etc.)
 * - Packet statistics
 * - Configuration changes
 */
class LoRaAPIHandler : public APIHandler
{
public:
    /**
     * @brief Constructor
     */
    LoRaAPIHandler();

    /**
     * @brief Register LoRa API routes
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Set callback for getting LoRa status
     * @param callback Function that returns LoRa status as JSON string
     */
    void setLoRaStatusCallback(std::function<String()> callback);

    /**
     * @brief Set callback for configuring LoRa parameters
     * @param callback Function that accepts parameter name, value, and returns success with message
     */
    void setLoRaConfigCallback(std::function<bool(const String&, const String&, String&)> callback);

private:
    std::function<String()> getLoRaStatusCallback;
    std::function<bool(const String&, const String&, String&)> loraConfigCallback;

    /**
     * @brief Handle GET /api/lora/status
     */
    void handleLoRaStatus(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/lora/config
     */
    void handleLoRaConfig(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/lora/config
     */
    void handleSetLoRaConfig(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/lora/stats
     */
    void handleLoRaStats(AsyncWebServerRequest* request);
};