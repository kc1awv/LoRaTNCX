/**
 * @file SystemAPIHandler.h
 * @brief System status and information API handler
 */

#pragma once

#include "APIHandler.h"
#include <functional>

/**
 * @brief Handles system-related API endpoints
 * 
 * Provides endpoints for:
 * - System status information
 * - Hardware information
 * - Uptime and performance metrics
 * - Debug information
 */
class SystemAPIHandler : public APIHandler
{
public:
    /**
     * @brief Constructor
     */
    SystemAPIHandler();

    /**
     * @brief Register system API routes
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Set callback for getting system status
     * @param callback Function that returns system status as JSON string
     */
    void setSystemStatusCallback(std::function<String()> callback);

private:
    std::function<String()> getSystemStatusCallback;

    /**
     * @brief Handle GET /api/system/status
     */
    void handleSystemStatus(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/system/info
     */
    void handleSystemInfo(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/system/performance
     */
    void handlePerformanceInfo(AsyncWebServerRequest* request);

    /**
     * @brief Handle GET /api/system/logs
     */
    void handleSystemLogs(AsyncWebServerRequest* request);
};