/**
 * @file WebServerManager.h
 * @brief Asynchronous web server management for LoRaTNCX
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <functional>

// Forward declarations
class TNCWiFiManager;
class TNCManager;

/**
 * @brief Manages the asynchronous web server with proper WiFi state integration
 */
class WebServerManager
{
public:
    /**
     * @brief Constructor
     * @param port HTTP server port (default 80)
     */
    explicit WebServerManager(uint16_t port = 80);

    /**
     * @brief Initialize the web server subsystem
     * @note This does NOT start the server - that happens when WiFi is ready
     * @return true on successful initialization
     */
    bool begin();

    /**
     * @brief Update function to be called regularly
     * Handles server lifecycle based on WiFi state
     */
    void update();

    /**
     * @brief Start the web server (called when WiFi is ready)
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the web server (called when WiFi disconnects)
     */
    void stop();

    /**
     * @brief Check if the web server is currently running
     * @return true if server is active
     */
    bool isRunning() const { return serverRunning; }

    /**
     * @brief Set callback functions for getting system data
     */
    void setCallbacks(
        std::function<String()> getSystemStatus,
        std::function<String()> getLoRaStatus,
        std::function<String()> getWiFiNetworks,
        std::function<bool(const String&, const String&, String&)> addWiFiNetwork,
        std::function<bool(const String&, String&)> removeWiFiNetwork
    );

private:
    AsyncWebServer server;
    uint16_t serverPort;
    bool serverRunning;
    bool filesystemMounted;
    
    // Callback functions for data access
    std::function<String()> getSystemStatusCallback;
    std::function<String()> getLoRaStatusCallback;
    std::function<String()> getWiFiNetworksCallback;
    std::function<bool(const String&, const String&, String&)> addWiFiNetworkCallback;
    std::function<bool(const String&, String&)> removeWiFiNetworkCallback;

    /**
     * @brief Initialize SPIFFS for serving web files
     * @return true if successful
     */
    bool initializeFilesystem();

    /**
     * @brief Setup all web server routes
     */
    void setupRoutes();

    /**
     * @brief Setup static file serving for web interface
     */
    void setupStaticFiles();

    /**
     * @brief Setup REST API endpoints
     */
    void setupAPIRoutes();

    /**
     * @brief Handle system status API endpoint
     */
    void handleSystemStatus(AsyncWebServerRequest *request);

    /**
     * @brief Handle LoRa status API endpoint
     */
    void handleLoRaStatus(AsyncWebServerRequest *request);

    /**
     * @brief Handle WiFi networks list API endpoint
     */
    void handleWiFiNetworks(AsyncWebServerRequest *request);

    /**
     * @brief Handle add WiFi network API endpoint
     */
    void handleAddWiFiNetwork(AsyncWebServerRequest *request);

    /**
     * @brief Handle remove WiFi network API endpoint
     */
    void handleRemoveWiFiNetwork(AsyncWebServerRequest *request);

    /**
     * @brief Handle debug files list API endpoint
     */
    void handleListFiles(AsyncWebServerRequest *request);

    /**
     * @brief Handle 404 not found responses
     */
    void handleNotFound(AsyncWebServerRequest *request);

    /**
     * @brief Set CORS headers for API responses
     */
    void setCORSHeaders(AsyncWebServerResponse *response);

    /**
     * @brief Get MIME type for file extension
     */
    String getMimeType(const String& path);

    /**
     * @brief Check if a file exists in SPIFFS
     */
    bool fileExists(const String& path);
};