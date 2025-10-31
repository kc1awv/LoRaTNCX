#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <DisplayManager.h>
#include <TNCCommands.h>

class TNCManager;

/**
 * @brief Manages the web interface, including Wi-Fi, SPIFFS, and the async server stack.
 */
class WebInterfaceManager
{
public:
    WebInterfaceManager();

    /**
     * @brief Initialize networking and web resources.
     *
     * @param tnc Reference to the TNC manager for status and control hooks.
     * @return true if networking was started successfully, false otherwise.
     */
    bool begin(TNCManager &tnc);

    /**
     * @brief Handle periodic maintenance tasks for networking components.
     */
    void loop();

    /**
     * @brief Stop networking services and release resources.
     */
    void stop();

    /**
     * @brief Broadcast incremental status updates to connected WebSocket clients.
     */
    void broadcastStatus(const DisplayManager::StatusData &status);

    /**
     * @brief Emit a structured command execution result to clients.
     */
    void notifyCommandResult(const String &command, TNCCommandResult result, const String &source,
                             AsyncWebSocketClient *client = nullptr, const String &requestId = String());

    /**
     * @brief Emit a structured configuration update result to clients.
     */
    void notifyConfigurationResult(const String &command, bool success, const String &source,
                                   AsyncWebSocketClient *client = nullptr, const String &requestId = String());

    /**
     * @brief Broadcast an alert (e.g., GNSS state change) to connected clients.
     */
    void broadcastAlert(const String &category, const String &message, bool state);

    /**
     * @brief Broadcast a packet reception notification.
     */
    void broadcastPacketNotification(size_t length, float rssi, float snr, const String &preview);

private:
    bool mountSPIFFS();
    void setupWiFi();
    void setupCaptivePortal();
    void setupWebServer();
    void setupWebSocket();
    void handleWebSocketConnect(AsyncWebSocketClient *client);
    void handleWebSocketDisconnect(AsyncWebSocketClient *client);
    void handleWebSocketMessage(AsyncWebSocketClient *client, const char *data, size_t len);
    void sendStatusSnapshot(AsyncWebSocketClient *client, const String &requestId = String());
    void sendErrorMessage(AsyncWebSocketClient *client, const String &message, const String &requestId = String());
    void sendErrorMessage(AsyncWebSocketClient *client, const __FlashStringHelper *message,
                          const String &requestId = String());
    void handleThemePost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    String normaliseThemePreference(const String &theme) const;
    void sendJsonToClient(AsyncWebSocketClient *client, JsonDocument &doc);
    bool ensureAuthenticated(AsyncWebServerRequest *request);
    bool ensureCsrfToken(AsyncWebServerRequest *request);
    String generateRandomToken(size_t length) const;
    void regenerateCsrfToken();
    void refreshCredentialsFromConfig();
    void configureWebSocketAuthentication();
    void startAccessPoint(const String &password);
    void restartStationInterface();
    bool stationModeRequested() const;

    bool spiffsMounted;
    bool wifiStarted;
    bool serverRunning;
    bool captivePortalActive;
    bool staConnected;
    unsigned long lastWiFiReconnectAttempt;

    String uiThemePreference;
    bool uiThemeOverride;
    bool pairingRequired;
    String temporaryApPassword;
    String currentApPassword;
    String basicAuthUsername;
    String basicAuthPassword;
    String csrfToken;

    AsyncWebServer server;
    AsyncWebSocket webSocket;
    DNSServer dnsServer;
    IPAddress apIP;
    String apSSID;
    TNCManager *tncManager;

    unsigned long lastStatusBroadcast;
    bool hasBroadcastStatus;
    DisplayManager::StatusData lastBroadcastStatus;
};
