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

// Web Server Constants
#define WEB_SERVER_PORT             80      ///< HTTP server port
#define WIFI_CHANGE_DELAY_MS        500     ///< Delay before applying WiFi changes
#define HTTP_STATUS_OK              200     ///< HTTP OK status code
#define HTTP_BAD_REQUEST            400     ///< HTTP Bad Request status code
#define WEB_CACHE_MAX_AGE           3600    ///< Cache max age in seconds (1 hour)

// Radio Parameter Validation Ranges
#define RADIO_FREQ_MIN              150.0f  ///< Minimum frequency in MHz
#define RADIO_FREQ_MAX              960.0f  ///< Maximum frequency in MHz
#define RADIO_SF_MIN                7       ///< Minimum spreading factor
#define RADIO_SF_MAX                12      ///< Maximum spreading factor
#define RADIO_CR_MIN                5       ///< Minimum coding rate
#define RADIO_CR_MAX                8       ///< Maximum coding rate
#define RADIO_POWER_MIN             -9      ///< Minimum power in dBm
// RADIO_POWER_MAX is now defined in config.h based on board type

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
     * @brief Initialize the web server
     *
     * Sets up HTTP routes, starts the server, and prepares for client connections.
     *
     * @return true if server started successfully
     * @return false if initialization failed
     */
    bool begin();
    
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
};

#endif // WEB_SERVER_H
