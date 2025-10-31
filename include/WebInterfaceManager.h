#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

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

private:
    bool mountSPIFFS();
    void setupWiFi();
    void setupCaptivePortal();
    void setupWebServer();
    void setupWebSocket();
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                          uint8_t *data, size_t len);
    void handleThemePost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    String normaliseThemePreference(const String &theme) const;

    bool spiffsMounted;
    bool wifiStarted;
    bool serverRunning;
    bool captivePortalActive;
    bool staConnected;
    unsigned long lastWiFiReconnectAttempt;

    String uiThemePreference;
    bool uiThemeOverride;

    AsyncWebServer server;
    AsyncWebSocket webSocket;
    DNSServer dnsServer;
    IPAddress apIP;
    String apSSID;
    TNCManager *tncManager;
};
