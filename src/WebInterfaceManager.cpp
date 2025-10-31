#include "WebInterfaceManager.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "TNCManager.h"

namespace
{
constexpr uint16_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint16_t WIFI_CHECK_INTERVAL_MS = 1000;
constexpr const char *DEFAULT_AP_PASSWORD = "LoRaTNCX";
} // namespace

WebInterfaceManager::WebInterfaceManager()
    : spiffsMounted(false),
      wifiStarted(false),
      serverRunning(false),
      captivePortalActive(false),
      staConnected(false),
      lastWiFiReconnectAttempt(0),
      server(80),
      webSocket("/ws"),
      apIP(192, 168, 4, 1),
      tncManager(nullptr)
{
}

bool WebInterfaceManager::begin(TNCManager &tnc)
{
    tncManager = &tnc;

    if (!mountSPIFFS())
    {
        Serial.println("[WebInterface] Failed to mount SPIFFS; continuing without filesystem content.");
    }

    setupWiFi();
    setupCaptivePortal();
    setupWebSocket();
    setupWebServer();

    server.begin();
    serverRunning = true;

    Serial.println("[WebInterface] Async web server started.");
    return wifiStarted;
}

void WebInterfaceManager::loop()
{
    if (captivePortalActive)
    {
        dnsServer.processNextRequest();
    }

    if (wifiStarted)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            staConnected = true;
        }
        else if (staConnected)
        {
            staConnected = false;
            lastWiFiReconnectAttempt = millis();
        }
        else if (millis() - lastWiFiReconnectAttempt > WIFI_CHECK_INTERVAL_MS)
        {
            lastWiFiReconnectAttempt = millis();
            WiFi.reconnect();
        }
    }

    webSocket.cleanupClients();
}

void WebInterfaceManager::stop()
{
    if (serverRunning)
    {
        server.end();
        serverRunning = false;
    }

    if (captivePortalActive)
    {
        dnsServer.stop();
        captivePortalActive = false;
    }

    if (wifiStarted)
    {
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        wifiStarted = false;
    }

    if (spiffsMounted)
    {
        SPIFFS.end();
        spiffsMounted = false;
    }

    tncManager = nullptr;
}

bool WebInterfaceManager::mountSPIFFS()
{
    if (spiffsMounted)
    {
        return true;
    }

    if (!SPIFFS.begin(true))
    {
        return false;
    }

    spiffsMounted = true;
    Serial.println("[WebInterface] SPIFFS mounted successfully.");
    return true;
}

void WebInterfaceManager::setupWiFi()
{
    if (wifiStarted)
    {
        return;
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        staConnected = true;
        Serial.print("[WebInterface] Connected to Wi-Fi network, IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        staConnected = false;
        Serial.println("[WebInterface] Unable to join saved Wi-Fi network; operating in AP-only mode.");
    }

    IPAddress netMask(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMask);

    uint32_t shortId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
    apSSID = String("LoRaTNCX-") + String(shortId, HEX);
    apSSID.toUpperCase();

    if (WiFi.softAP(apSSID.c_str(), DEFAULT_AP_PASSWORD))
    {
        Serial.print("[WebInterface] Access point started: ");
        Serial.print(apSSID);
        Serial.print(" password: ");
        Serial.println(DEFAULT_AP_PASSWORD);
    }
    else
    {
        Serial.println("[WebInterface] Failed to start access point.");
    }

    wifiStarted = true;
}

void WebInterfaceManager::setupCaptivePortal()
{
    if (!wifiStarted || captivePortalActive)
    {
        return;
    }

    dnsServer.setTTL(300);
    dnsServer.start(53, "*", apIP);
    captivePortalActive = true;
    Serial.println("[WebInterface] Captive portal DNS server started.");
}

void WebInterfaceManager::setupWebServer()
{
    if (!spiffsMounted)
    {
        server.on(
            "/", HTTP_GET, [this](AsyncWebServerRequest *request) {
                String response = F("LoRaTNCX Web Interface\n");
                if (tncManager)
                {
                    response += tncManager->getStatus();
                }
                request->send(200, "text/plain", response);
            });
    }
    else
    {
        server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    }

    server.on(
        "/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
            JsonDocument doc;
            doc["wifi"]["sta_connected"] = staConnected;
            doc["wifi"]["ap_ssid"] = apSSID;
            if (tncManager)
            {
                doc["tnc"]["status"] = tncManager->getStatus();
            }
            else
            {
                doc["tnc"]["status"] = "unavailable";
            }

            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (request->host() != WiFi.softAPIP().toString())
        {
            String redirectUrl = String("http://") + WiFi.softAPIP().toString();
            request->redirect(redirectUrl);
        }
        else if (spiffsMounted)
        {
            request->send(SPIFFS, "/index.html", String(), false);
        }
        else
        {
            request->send(404, "text/plain", "Resource not found.");
        }
    });
}

void WebInterfaceManager::setupWebSocket()
{
    webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                              uint8_t *data, size_t len) {
        onWebSocketEvent(server, client, type, arg, data, len);
    });

    server.addHandler(&webSocket);
}

void WebInterfaceManager::onWebSocketEvent(AsyncWebSocket *serverInstance, AsyncWebSocketClient *client,
                                           AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
    {
        JsonDocument doc;
        doc["type"] = "hello";
        doc["ap_ssid"] = apSSID;
        doc["sta_connected"] = staConnected;
        if (tncManager)
        {
            doc["tnc_status"] = tncManager->getStatus();
        }

        String payload;
        serializeJson(doc, payload);
        client->text(payload);
        break;
    }
    case WS_EVT_DATA:
    {
        AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
        {
            String message;
            message.reserve(len);
            for (size_t i = 0; i < len; ++i)
            {
                message += static_cast<char>(data[i]);
            }

            if (message.equalsIgnoreCase("status") && tncManager)
            {
                JsonDocument doc;
                doc["type"] = "status";
                doc["tnc_status"] = tncManager->getStatus();
                String payload;
                serializeJson(doc, payload);
                client->text(payload);
            }
            else
            {
                JsonDocument doc;
                doc["type"] = "echo";
                doc["message"] = message;
                String payload;
                serializeJson(doc, payload);
                client->text(payload);
            }
        }
        break;
    }
    case WS_EVT_DISCONNECT:
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    default:
        break;
    }
}
