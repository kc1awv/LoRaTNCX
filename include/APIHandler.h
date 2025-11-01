/**
 * @file APIHandler.h
 * @brief Base class for REST API handlers
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

/**
 * @brief Base class for API endpoint handlers
 * 
 * This provides a common interface for handling REST API endpoints
 * with proper JSON response formatting and error handling.
 */
class APIHandler
{
public:
    /**
     * @brief Constructor
     * @param basePath The base path for this handler's endpoints (e.g., "/api/system")
     */
    explicit APIHandler(const String& basePath);

    /**
     * @brief Virtual destructor
     */
    virtual ~APIHandler() = default;

    /**
     * @brief Register all endpoints for this handler with the web server
     * @param server Reference to the AsyncWebServer instance
     */
    virtual void registerRoutes(AsyncWebServer& server) = 0;

    /**
     * @brief Get the base path for this handler
     */
    const String& getBasePath() const { return basePath; }

protected:
    String basePath;

    /**
     * @brief Send a successful JSON response
     * @param request The web request
     * @param data JSON data to send (optional)
     * @param message Success message (optional)
     */
    void sendSuccess(AsyncWebServerRequest* request, const JsonDocument& data = JsonDocument(), const String& message = "");

    /**
     * @brief Send an error JSON response
     * @param request The web request
     * @param message Error message
     * @param httpCode HTTP status code (default 400)
     */
    void sendError(AsyncWebServerRequest* request, const String& message, int httpCode = 400);

    /**
     * @brief Send a JSON response with custom structure
     * @param request The web request
     * @param doc JsonDocument to send
     * @param httpCode HTTP status code (default 200)
     */
    void sendJsonResponse(AsyncWebServerRequest* request, const JsonDocument& doc, int httpCode = 200);

    /**
     * @brief Set CORS headers on a response
     * @param response The web response to modify
     */
    void setCORSHeaders(AsyncWebServerResponse* response);

    /**
     * @brief Create a standardized error response document
     * @param message Error message
     * @return JsonDocument with error structure
     */
    JsonDocument createErrorResponse(const String& message);

    /**
     * @brief Create a standardized success response document
     * @param data Optional data payload
     * @param message Optional success message
     * @return JsonDocument with success structure
     */
    JsonDocument createSuccessResponse(const JsonDocument& data = JsonDocument(), const String& message = "");

    /**
     * @brief Validate required parameters in POST request
     * @param request The web request
     * @param requiredParams List of required parameter names
     * @return true if all required parameters are present
     */
    bool validateRequiredParams(AsyncWebServerRequest* request, const std::vector<String>& requiredParams);

    /**
     * @brief Get parameter value from POST request
     * @param request The web request
     * @param paramName Parameter name
     * @param defaultValue Default value if parameter not found
     * @return Parameter value or default
     */
    String getParam(AsyncWebServerRequest* request, const String& paramName, const String& defaultValue = "");
};