#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "radio.h"
#include "config_manager.h"
#include "board_config.h"
#include "gnss.h"
#include "nmea_server.h"
#include "validation_utils.h"
#include "battery_monitor.h"

// Web Server Constants
#define HTTP_STATUS_OK              200     ///< HTTP OK status code
#define HTTP_BAD_REQUEST            400     ///< HTTP Bad Request status code

/**
 * @brief Web Server for LoRa TNC configuration and monitoring
 *
 * Provides a RESTful HTTP API for remote configuration and status monitoring
 * of the LoRa TNC. Supports both web interface and programmatic access.
 *
 * Features:
 * - RESTful API endpoints for all configuration options
 * - JSON responses for easy programmatic access
 * - CORS support for web applications
 * - Real-time status information
 * - Firmware upload capability
 * - Input validation and error handling
 */
class TNCWebServer {
public:
    /**
     * @brief Construct a new TNC Web Server
     *
     * @param wifiMgr Pointer to WiFiManager instance
     * @param radio Pointer to LoRaRadio instance
     * @param configMgr Pointer to ConfigManager instance
     */
    TNCWebServer(WiFiManager* wifiMgr, LoRaRadio* radio, ConfigManager* configMgr);
    
    /**
     * @brief Set GNSS and NMEA server instances (optional)
     *
     * GNSS functionality is optional and can be enabled by providing
     * pointers to GNSS and NMEA server instances.
     *
     * @param gnss Pointer to GNSSModule instance
     * @param nmea Pointer to NMEAServer instance
     */
    void setGNSS(GNSSModule* gnss, NMEAServer* nmea) {
        gnssModule = gnss;
        nmeaServer = nmea;
    }
    
    /**
     * @brief Set BatteryMonitor instance
     *
     * @param battery Pointer to BatteryMonitor instance
     */
    void setBatteryMonitor(BatteryMonitor* battery) {
        batteryMonitor = battery;
    }
    
    /**
     * @brief Initialize and start the web server
     *
     * Sets up HTTP routes, starts the server, and prepares for client connections.
     *
     * @return Result<void> indicating success or specific error
     */
    Result<void> begin();
    
    /**
     * @brief Stop the web server
     *
     * Shuts down the HTTP server and frees resources.
     */
    void stop();
    
    /**
     * @brief Update web server state
     *
     * Main update function that should be called regularly in the main loop.
     * Handles pending operations like delayed WiFi configuration changes.
     */
    void update();
    
    /**
     * @brief Check if the web server is running
     *
     * @return true if server is active and accepting connections
     * @return false if server is stopped or failed
     */
    bool isRunning();
    
private:
    AsyncWebServer* server;
    WiFiManager* wifiManager;
    LoRaRadio* loraRadio;
    ConfigManager* configManager;
    GNSSModule* gnssModule;
    NMEAServer* nmeaServer;
    BatteryMonitor* batteryMonitor;
    bool running;
    
    // Pending WiFi config change
    bool pendingWiFiChange;
    WiFiConfig pendingWiFiConfig;
    unsigned long wifiChangeTime;
    
    // Setup routes
    void setupRoutes();
    
    // API handlers - System status
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetSystemInfo(AsyncWebServerRequest* request);
    
    // API handlers - LoRa configuration
    void handleGetLoRaConfig(AsyncWebServerRequest* request);
    void handleSaveLoRaConfig(AsyncWebServerRequest* request);
    void handleResetLoRaConfig(AsyncWebServerRequest* request);
    
    // API handlers - WiFi configuration
    void handleGetWiFiConfig(AsyncWebServerRequest* request);
    void handleSaveWiFiConfig(AsyncWebServerRequest* request);
    void handleScanWiFi(AsyncWebServerRequest* request);
    void handleScanStatus(AsyncWebServerRequest* request);
    
    // API handlers - GNSS configuration
    void handleGetGNSSConfig(AsyncWebServerRequest* request);
    void handleSetGNSSConfig(AsyncWebServerRequest* request, const char* jsonData, size_t len);
    void handleGetGNSSStatus(AsyncWebServerRequest* request);
    
    // API handlers - Control
    void handleReboot(AsyncWebServerRequest* request);
    
    // Helper methods
    String getJSONStatus();
    String getJSONSystemInfo();
    String getJSONLoRaConfig();
    String getJSONWiFiConfig();
    String getJSONGNSSConfig();
    String getJSONGNSSStatus();
    
    // File serving with compression
    void serveCompressedFile(AsyncWebServerRequest* request, const char* path, const char* contentType);
    
    // CORS headers
    void addCORSHeaders(AsyncWebServerResponse* response);
    
    // Input validation and security
    bool validateStringInput(const char* input, size_t maxLen, bool allowSpecialChars = false);
    bool validateWiFiPassword(const char* password);
    bool validateSSID(const char* ssid);
    void sendErrorResponse(AsyncWebServerRequest* request, int code, const char* message);
};

#endif // WEB_SERVER_H
