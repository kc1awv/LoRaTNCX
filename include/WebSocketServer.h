#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Forward declarations
class RadioHAL;
class GNSSDriver;
class KISS;
class ConfigManager;
class OTAManager;

/**
 * @brief WebSocket server for LoRaTNCX web interface
 * 
 * Provides real-time communication between the web interface and ESP32,
 * handling configuration updates, status monitoring, and commands.
 */
class WebSocketServer {
public:
    /**
     * @brief Initialize the WebSocket server
     * @param server Reference to AsyncWebServer instance
     */
    static void begin(AsyncWebServer& server);
    
    /**
     * @brief Handle periodic updates and cleanup
     * Must be called regularly from main loop
     */
    static void handle();
    
    /**
     * @brief Set references to system components
     */
    static void setRadio(RadioHAL* radio);
    static void setGNSS(GNSSDriver* gnss);
    static void setKISS(KISS* kiss);
    static void setConfig(ConfigManager* config);
    
    /**
     * @brief Broadcast status updates to all connected clients
     */
    static void broadcastSystemStatus();
    static void broadcastRadioStatus();
    static void broadcastAPRSMessage(const String& from, const String& to, const String& message, int16_t rssi);
    static void broadcastGNSSStatus();
    static void broadcastBatteryStatus();
    static void broadcastWiFiStatus();
    static void broadcastLogMessage(const String& level, const String& message);
    
    /**
     * @brief Get number of connected clients
     */
    static uint32_t getClientCount();

private:
    static AsyncWebSocket* ws;
    static AsyncWebServer* webServer;
    
    // System component references
    static RadioHAL* radioRef;
    static GNSSDriver* gnssRef;
    static KISS* kissRef;
    static ConfigManager* configRef;
    
    // Status tracking
    static unsigned long lastStatusBroadcast;
    static unsigned long lastSystemInfo;
    static const unsigned long STATUS_INTERVAL = 5000; // 5 seconds
    static const unsigned long SYSTEM_INFO_INTERVAL = 10000; // 10 seconds
    
    // Message handling
    static void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                                AwsEventType type, void* arg, uint8_t* data, size_t len);
    static void handleWebSocketMessage(AsyncWebSocketClient* client, const String& message);
    static void processMessage(AsyncWebSocketClient* client, JsonDocument& doc);
    
    // Request handlers
    static void handleStatusRequest(AsyncWebSocketClient* client, const String& dataType);
    static void handleConfigUpdate(AsyncWebSocketClient* client, const String& category, JsonObject data);
    static void handleCommand(AsyncWebSocketClient* client, const String& action, JsonObject data);
    
    // Configuration handlers
    static void updateRadioConfig(JsonObject data);
    static void updateAPRSConfig(JsonObject data);
    static void updateWiFiConfig(JsonObject data);
    static void updateSystemConfig(JsonObject data);
    
    // Command handlers
    static void handleRestart();
    static void handleFactoryReset();
    static void handleSendAPRSMessage(JsonObject data);
    static void handleBackupConfig(AsyncWebSocketClient* client);
    
    // Response helpers
    static void sendResponse(AsyncWebSocketClient* client, const String& type, JsonObject& payload);
    static void sendError(AsyncWebSocketClient* client, const String& message);
    static void broadcastMessage(const String& type, JsonObject& payload);
    
    // Status data generators
    static JsonObject createSystemStatus(JsonDocument& doc);
    static JsonObject createRadioStatus(JsonDocument& doc);
    static JsonObject createGNSSStatus(JsonDocument& doc);
    static JsonObject createBatteryStatus(JsonDocument& doc);
    static JsonObject createWiFiStatus(JsonDocument& doc);
    
    // Utility functions
    static String formatUptime(unsigned long seconds);
    static size_t getHeapSize();
    static size_t getFreeHeap();
    static size_t getFlashSize();
    static size_t getFlashUsed();
    static size_t getSPIFFSSize();
    static size_t getSPIFFSUsed();
    static float getCPUTemperature();
    
    // SPIFFS web file serving
    static void setupWebRoutes();
    static void serveFile(AsyncWebServerRequest* request, const String& path);
    static String getContentType(const String& filename);
    static bool handleFileRead(AsyncWebServerRequest* request, String path);
};