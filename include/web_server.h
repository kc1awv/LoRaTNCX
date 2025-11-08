#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "radio.h"
#include "config_manager.h"
#include "board_config.h"

class TNCWebServer {
public:
    TNCWebServer(WiFiManager* wifiMgr, LoRaRadio* radio, ConfigManager* configMgr);
    
    // Initialize web server
    bool begin();
    
    // Stop web server
    void stop();
    
    // Update web server - call in loop()
    void update();
    
    // Check if server is running
    bool isRunning();
    
private:
    AsyncWebServer* server;
    WiFiManager* wifiManager;
    LoRaRadio* loraRadio;
    ConfigManager* configManager;
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
    
    // API handlers - Control
    void handleReboot(AsyncWebServerRequest* request);
    
    // Helper methods
    String getJSONStatus();
    String getJSONSystemInfo();
    String getJSONLoRaConfig();
    String getJSONWiFiConfig();
    
    // CORS headers
    void addCORSHeaders(AsyncWebServerResponse* response);
};

#endif // WEB_SERVER_H
