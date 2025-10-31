#include "WebInterfaceManager.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <math.h>

#include "TNCManager.h"

namespace
{
constexpr uint16_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint16_t WIFI_CHECK_INTERVAL_MS = 1000;
constexpr const char *DEFAULT_AP_PASSWORD = "LoRaTNCX";
constexpr size_t MAX_JSON_BODY_SIZE = 512;

const char *modeToString(TNCMode mode)
{
    switch (mode)
    {
    case TNCMode::KISS_MODE:
        return "KISS";
    case TNCMode::COMMAND_MODE:
        return "COMMAND";
    case TNCMode::TERMINAL_MODE:
        return "TERMINAL";
    case TNCMode::TRANSPARENT_MODE:
        return "TRANSPARENT";
    default:
        return "UNKNOWN";
    }
}

struct CommandHttpMapping
{
    int statusCode;
    bool success;
    const char *message;
};

CommandHttpMapping mapCommandResultToHttp(TNCCommandResult result)
{
    switch (result)
    {
    case TNCCommandResult::SUCCESS:
        return {200, true, "Command executed successfully."};
    case TNCCommandResult::SUCCESS_SILENT:
        return {200, true, "Command executed successfully (no output)."};
    case TNCCommandResult::ERROR_UNKNOWN_COMMAND:
        return {404, false, "Unknown command."};
    case TNCCommandResult::ERROR_INVALID_PARAMETER:
        return {400, false, "Invalid parameter."};
    case TNCCommandResult::ERROR_SYSTEM_ERROR:
        return {500, false, "System error encountered."};
    case TNCCommandResult::ERROR_NOT_IMPLEMENTED:
        return {501, false, "Command not implemented."};
    case TNCCommandResult::ERROR_INSUFFICIENT_ARGS:
        return {400, false, "Insufficient arguments provided."};
    case TNCCommandResult::ERROR_TOO_MANY_ARGS:
        return {400, false, "Too many arguments provided."};
    case TNCCommandResult::ERROR_INVALID_VALUE:
        return {400, false, "Invalid value provided."};
    case TNCCommandResult::ERROR_HARDWARE_ERROR:
        return {503, false, "Hardware error encountered."};
    default:
        return {500, false, "Unhandled command result."};
    }
}

const char *commandResultToString(TNCCommandResult result)
{
    switch (result)
    {
    case TNCCommandResult::SUCCESS:
        return "SUCCESS";
    case TNCCommandResult::SUCCESS_SILENT:
        return "SUCCESS_SILENT";
    case TNCCommandResult::ERROR_UNKNOWN_COMMAND:
        return "ERROR_UNKNOWN_COMMAND";
    case TNCCommandResult::ERROR_INVALID_PARAMETER:
        return "ERROR_INVALID_PARAMETER";
    case TNCCommandResult::ERROR_SYSTEM_ERROR:
        return "ERROR_SYSTEM_ERROR";
    case TNCCommandResult::ERROR_NOT_IMPLEMENTED:
        return "ERROR_NOT_IMPLEMENTED";
    case TNCCommandResult::ERROR_INSUFFICIENT_ARGS:
        return "ERROR_INSUFFICIENT_ARGS";
    case TNCCommandResult::ERROR_TOO_MANY_ARGS:
        return "ERROR_TOO_MANY_ARGS";
    case TNCCommandResult::ERROR_INVALID_VALUE:
        return "ERROR_INVALID_VALUE";
    case TNCCommandResult::ERROR_HARDWARE_ERROR:
        return "ERROR_HARDWARE_ERROR";
    default:
        return "UNKNOWN";
    }
}
} // namespace

String WebInterfaceManager::normaliseThemePreference(const String &theme) const
{
    String value = theme;
    value.trim();
    value.toLowerCase();

    if (value == "auto")
    {
        value = "system";
    }

    if (value == "light" || value == "dark" || value == "system")
    {
        return value;
    }

    return String();
}

WebInterfaceManager::WebInterfaceManager()
    : spiffsMounted(false),
      wifiStarted(false),
      serverRunning(false),
      captivePortalActive(false),
      staConnected(false),
      lastWiFiReconnectAttempt(0),
      uiThemePreference("system"),
      uiThemeOverride(false),
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
        AsyncStaticWebHandler &staticHandler = server.serveStatic("/", SPIFFS, "/");
        staticHandler.setDefaultFile("index.html");
        staticHandler.setCacheControl("public, max-age=86400, immutable");
        
        // Explicitly handle CSS files with correct MIME type
        server.on("/css/bootstrap.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/css/bootstrap.css", "text/css");
        });
        
        // Explicitly handle JS files with correct MIME type
        server.on("/js/bootstrap.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/bootstrap.js", "application/javascript");
        });
        
        // Handle other common JS files
        server.on("/js/api.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/api.js", "application/javascript");
        });
        
        server.on("/js/common.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/common.js", "application/javascript");
        });
        
        server.on("/js/config.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/config.js", "application/javascript");
        });
        
        server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/index.js", "application/javascript");
        });
        
        server.on("/js/status.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/status.js", "application/javascript");
        });
        
        server.on("/js/theme.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/js/theme.js", "application/javascript");
        });
    }

    server.on(
        "/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(4096);
            JsonObject wifi = doc.createNestedObject("wifi");
            wifi["sta_connected"] = staConnected;
            wifi["ap_ssid"] = apSSID;

            JsonObject tnc = doc.createNestedObject("tnc");
            if (tncManager)
            {
                TNCManager::StatusSnapshot snapshot = tncManager->getStatusSnapshot();
                tnc["available"] = true;
                tnc["status_text"] = snapshot.statusText;

                JsonObject display = tnc.createNestedObject("display");
                display["mode"] = modeToString(snapshot.displayStatus.mode);
                display["tx_count"] = snapshot.displayStatus.txCount;
                display["rx_count"] = snapshot.displayStatus.rxCount;
                display["last_packet_millis"] = snapshot.displayStatus.lastPacketMillis;
                display["has_recent_packet"] = snapshot.displayStatus.hasRecentPacket;
                display["last_rssi"] = snapshot.displayStatus.lastRSSI;
                display["last_snr"] = snapshot.displayStatus.lastSNR;
                display["frequency_mhz"] = snapshot.displayStatus.frequency;
                display["bandwidth_khz"] = snapshot.displayStatus.bandwidth;
                display["spreading_factor"] = snapshot.displayStatus.spreadingFactor;
                display["coding_rate"] = snapshot.displayStatus.codingRate;
                display["tx_power_dbm"] = snapshot.displayStatus.txPower;
                display["uptime_ms"] = snapshot.displayStatus.uptimeMillis;

                JsonObject battery = display.createNestedObject("battery");
                battery["voltage"] = snapshot.displayStatus.batteryVoltage;
                battery["percent"] = snapshot.displayStatus.batteryPercent;

                JsonObject powerOff = display.createNestedObject("power_off");
                powerOff["active"] = snapshot.displayStatus.powerOffActive;
                powerOff["progress"] = snapshot.displayStatus.powerOffProgress;
                powerOff["complete"] = snapshot.displayStatus.powerOffComplete;

                JsonObject gnss = display.createNestedObject("gnss");
                gnss["enabled"] = snapshot.displayStatus.gnssEnabled;
                gnss["has_fix"] = snapshot.displayStatus.gnssHasFix;
                gnss["is_3d_fix"] = snapshot.displayStatus.gnssIs3DFix;
                if (!isnan(snapshot.displayStatus.gnssLatitude))
                {
                    gnss["latitude"] = snapshot.displayStatus.gnssLatitude;
                }
                else
                {
                    gnss["latitude"] = nullptr;
                }
                if (!isnan(snapshot.displayStatus.gnssLongitude))
                {
                    gnss["longitude"] = snapshot.displayStatus.gnssLongitude;
                }
                else
                {
                    gnss["longitude"] = nullptr;
                }
                if (!isnan(snapshot.displayStatus.gnssAltitude))
                {
                    gnss["altitude_m"] = snapshot.displayStatus.gnssAltitude;
                }
                else
                {
                    gnss["altitude_m"] = nullptr;
                }
                gnss["speed_knots"] = snapshot.displayStatus.gnssSpeed;
                gnss["course_degrees"] = snapshot.displayStatus.gnssCourse;
                gnss["hdop"] = snapshot.displayStatus.gnssHdop;
                gnss["satellites"] = snapshot.displayStatus.gnssSatellites;
                gnss["time_valid"] = snapshot.displayStatus.gnssTimeValid;
                gnss["time_synced"] = snapshot.displayStatus.gnssTimeSynced;
                gnss["year"] = snapshot.displayStatus.gnssYear;
                gnss["month"] = snapshot.displayStatus.gnssMonth;
                gnss["day"] = snapshot.displayStatus.gnssDay;
                gnss["hour"] = snapshot.displayStatus.gnssHour;
                gnss["minute"] = snapshot.displayStatus.gnssMinute;
                gnss["second"] = snapshot.displayStatus.gnssSecond;
                gnss["pps_available"] = snapshot.displayStatus.gnssPpsAvailable;
                gnss["pps_last_millis"] = snapshot.displayStatus.gnssPpsLastMillis;
                gnss["pps_count"] = snapshot.displayStatus.gnssPpsCount;
            }
            else
            {
                tnc["available"] = false;
                tnc["status_text"] = "TNC manager unavailable";
            }

            JsonObject ui = doc.createNestedObject("ui");
            ui["theme"] = uiThemeOverride ? uiThemePreference : "system";
            ui["override"] = uiThemeOverride;

            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        });

    server.on(
        "/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(1024);
            JsonObject config = doc.createNestedObject("config");
            if (tncManager)
            {
                config["available"] = true;
                config["status_text"] = tncManager->getConfigurationStatus();
            }
            else
            {
                config["available"] = false;
                config["status_text"] = "TNC manager unavailable";
            }

            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        });

    server.on(
        "/api/config", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!request->_tempObject)
            {
                request->send(400, "application/json", "{\"error\":\"Missing JSON body\"}");
            }
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0)
            {
                request->_tempObject = new String();
                size_t reserveSize = total;
                if (reserveSize > MAX_JSON_BODY_SIZE)
                {
                    reserveSize = MAX_JSON_BODY_SIZE;
                }
                static_cast<String *>(request->_tempObject)->reserve(reserveSize);
            }

            String *body = static_cast<String *>(request->_tempObject);
            if (body->length() + len > MAX_JSON_BODY_SIZE)
            {
                delete body;
                request->_tempObject = nullptr;
                request->send(413, "application/json", "{\"error\":\"JSON payload too large\"}");
                return;
            }

            body->concat(reinterpret_cast<const char *>(data), len);

            if (index + len == total)
            {
                DynamicJsonDocument doc(512);
                DeserializationError error = deserializeJson(doc, *body);
                delete body;
                request->_tempObject = nullptr;

                if (error)
                {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON payload\"}");
                    return;
                }

                if (!doc.containsKey("command") || !doc["command"].is<const char *>())
                {
                    request->send(400, "application/json", "{\"error\":\"Missing command field\"}");
                    return;
                }

                if (!tncManager)
                {
                    request->send(503, "application/json", "{\"error\":\"TNC manager unavailable\"}");
                    return;
                }

                const char *command = doc["command"];
                bool success = tncManager->processConfigurationCommand(command);

                DynamicJsonDocument response(1024);
                response["success"] = success;
                response["status_text"] = tncManager->getConfigurationStatus();
                response["message"] = success ? "Configuration command accepted." : "Configuration command rejected.";

                String payload;
                serializeJson(response, payload);
                request->send(success ? 200 : 400, "application/json", payload);
            }
        });

    server.on(
        "/api/command", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!request->_tempObject)
            {
                request->send(400, "application/json", "{\"error\":\"Missing JSON body\"}");
            }
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0)
            {
                request->_tempObject = new String();
                size_t reserveSize = total;
                if (reserveSize > MAX_JSON_BODY_SIZE)
                {
                    reserveSize = MAX_JSON_BODY_SIZE;
                }
                static_cast<String *>(request->_tempObject)->reserve(reserveSize);
            }

            String *body = static_cast<String *>(request->_tempObject);
            if (body->length() + len > MAX_JSON_BODY_SIZE)
            {
                delete body;
                request->_tempObject = nullptr;
                request->send(413, "application/json", "{\"error\":\"JSON payload too large\"}");
                return;
            }

            body->concat(reinterpret_cast<const char *>(data), len);

            if (index + len == total)
            {
                DynamicJsonDocument doc(512);
                DeserializationError error = deserializeJson(doc, *body);
                delete body;
                request->_tempObject = nullptr;

                if (error)
                {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON payload\"}");
                    return;
                }

                if (!doc.containsKey("command") || !doc["command"].is<const char *>())
                {
                    request->send(400, "application/json", "{\"error\":\"Missing command field\"}");
                    return;
                }

                if (!tncManager)
                {
                    request->send(503, "application/json", "{\"error\":\"TNC manager unavailable\"}");
                    return;
                }

                const char *command = doc["command"];
                TNCCommandResult result = tncManager->executeCommand(command);
                CommandHttpMapping mapping = mapCommandResultToHttp(result);

                DynamicJsonDocument response(512);
                response["success"] = mapping.success;
                response["result"] = commandResultToString(result);
                response["message"] = mapping.message;

                String payload;
                serializeJson(response, payload);
                request->send(mapping.statusCode, "application/json", payload);
            }
        });

    server.on(
        "/api/ui/theme", HTTP_GET, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(128);
            doc["theme"] = uiThemeOverride ? uiThemePreference : "system";
            doc["override"] = uiThemeOverride;

            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        });

    server.on(
        "/api/ui/theme", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (!request->_tempObject)
            {
                request->_tempObject = new String();
            }
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleThemePost(request, data, len, index, total);
        });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (captivePortalActive && request->host() != WiFi.softAPIP().toString())
        {
            String redirectUrl = String("http://") + WiFi.softAPIP().toString();
            request->redirect(redirectUrl);
        }
        else if (spiffsMounted)
        {
            String path = request->url();
            if (!path.startsWith("/"))
            {
                path = "/" + path;
            }

            if (path.endsWith("/"))
            {
                path += "index.html";
            }

            if (SPIFFS.exists(path))
            {
                AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, String(), false);
                response->addHeader("Cache-Control", "public, max-age=86400, immutable");
                request->send(response);
                return;
            }

            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", String(), false);
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
        }
        else
        {
            request->send(404, "text/plain", "Resource not found.");
        }
        });
}

void WebInterfaceManager::handleThemePost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    String *body = static_cast<String *>(request->_tempObject);
    if (index == 0 && !body)
    {
        body = new String();
        size_t reserveSize = total > 256 ? 256 : total;
        body->reserve(reserveSize);
        request->_tempObject = body;
    }

    if (!body)
    {
        request->send(500, "application/json", "{\"error\":\"Internal buffer error\"}");
        return;
    }

    body->concat(reinterpret_cast<const char *>(data), len);

    if (index + len != total)
    {
        return;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, *body);
    delete body;
    request->_tempObject = nullptr;

    if (error)
    {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON payload\"}");
        return;
    }

    if (!doc.containsKey("theme"))
    {
        request->send(400, "application/json", "{\"error\":\"Missing theme field\"}");
        return;
    }

    String incoming = doc["theme"].as<String>();
    String normalised = normaliseThemePreference(incoming);
    if (normalised.isEmpty())
    {
        request->send(400, "application/json", "{\"error\":\"Unsupported theme value\"}");
        return;
    }

    uiThemeOverride = normalised != "system";
    uiThemePreference = uiThemeOverride ? normalised : "system";

    DynamicJsonDocument response(192);
    response["theme"] = uiThemeOverride ? uiThemePreference : "system";
    response["override"] = uiThemeOverride;
    if (doc.containsKey("source") && doc["source"].is<const char *>())
    {
        response["source"] = doc["source"].as<const char *>();
    }
    else
    {
        response["source"] = "user";
    }

    String payload;
    serializeJson(response, payload);
    request->send(200, "application/json", payload);
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
        doc["ui_theme"] = uiThemeOverride ? uiThemePreference : "system";
        doc["ui_theme_override"] = uiThemeOverride;

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
