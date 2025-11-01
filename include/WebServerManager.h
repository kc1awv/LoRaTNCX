/**
 * @file WebServerManager.h
 * @brief Asynchronous web server management for LoRaTNCX
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <functional>
#include <memory>
#include "APIManager.h"

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

    /**
     * @brief Get the API manager for advanced configuration
     * @return Pointer to APIManager instance
     */
    APIManager* getAPIManager() { return apiManager.get(); }

private:
    AsyncWebServer server;
    uint16_t serverPort;
    bool serverRunning;
    bool filesystemMounted;
    
    // API management system
    std::unique_ptr<APIManager> apiManager;

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
     * @brief Handle 404 not found responses
     */
    void handleNotFound(AsyncWebServerRequest *request);

    /**
     * @brief Get MIME type for file extension
     */
    String getMimeType(const String& path);

    /**
     * @brief Check if a file exists in SPIFFS
     */
    bool fileExists(const String& path);
};