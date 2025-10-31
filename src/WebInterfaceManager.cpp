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
constexpr unsigned long STATUS_BROADCAST_INTERVAL_MS = 300;

bool floatChanged(float current, float previous, float epsilon = 0.05f)
{
    return fabsf(current - previous) > epsilon;
}

bool doubleChanged(double current, double previous, double epsilon = 0.000001)
{
    if (isnan(current) && isnan(previous))
    {
        return false;
    }
    if (isnan(current) != isnan(previous))
    {
        return true;
    }
    return fabs(current - previous) > epsilon;
}

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
      tncManager(nullptr),
      lastStatusBroadcast(0),
      hasBroadcastStatus(false)
{
}

bool WebInterfaceManager::begin(TNCManager &tnc)
{
    tncManager = &tnc;
    tncManager->setWebInterface(this);

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

    if (tncManager)
    {
        tncManager->setWebInterface(nullptr);
        tncManager = nullptr;
    }

    hasBroadcastStatus = false;
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
                String commandText(command);
                bool success = tncManager->processConfigurationCommand(command);

                DynamicJsonDocument response(1024);
                response["success"] = success;
                response["status_text"] = tncManager->getConfigurationStatus();
                response["message"] = success ? "Configuration command accepted." : "Configuration command rejected.";

                String payload;
                serializeJson(response, payload);
                request->send(success ? 200 : 400, "application/json", payload);

                notifyConfigurationResult(commandText, success, "http");
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
                String commandText(command);
                TNCCommandResult result = tncManager->executeCommand(command);
                CommandHttpMapping mapping = mapCommandResultToHttp(result);

                DynamicJsonDocument response(512);
                response["success"] = mapping.success;
                response["result"] = commandResultToString(result);
                response["message"] = mapping.message;

                String payload;
                serializeJson(response, payload);
                request->send(mapping.statusCode, "application/json", payload);

                notifyCommandResult(commandText, result, "http");
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

    DynamicJsonDocument message(224);
    message["type"] = "ui_theme";
    message["timestamp"] = millis();
    message["theme"] = uiThemeOverride ? uiThemePreference : "system";
    message["override"] = uiThemeOverride;
    message["source"] = "http";
    sendJsonToClient(nullptr, message);
}

void WebInterfaceManager::sendStatusSnapshot(AsyncWebSocketClient *client, const String &requestId)
{
    if (!client)
    {
        return;
    }

    DynamicJsonDocument doc(4096);
    doc["type"] = "status_snapshot";
    doc["timestamp"] = millis();
    doc["client_id"] = client->id();
    if (!requestId.isEmpty())
    {
        doc["id"] = requestId;
    }

    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["sta_connected"] = staConnected;
    wifi["ap_ssid"] = apSSID;

    JsonObject tnc = doc.createNestedObject("tnc");
    if (tncManager)
    {
        tnc["available"] = true;
        TNCManager::StatusSnapshot snapshot = tncManager->getStatusSnapshot();
        tnc["status_text"] = snapshot.statusText;

        const DisplayManager::StatusData &displayStatus = snapshot.displayStatus;
        JsonObject display = tnc.createNestedObject("display");
        display["mode"] = modeToString(displayStatus.mode);
        display["tx_count"] = displayStatus.txCount;
        display["rx_count"] = displayStatus.rxCount;
        display["last_packet_millis"] = displayStatus.lastPacketMillis;
        display["has_recent_packet"] = displayStatus.hasRecentPacket;
        display["last_rssi"] = displayStatus.lastRSSI;
        display["last_snr"] = displayStatus.lastSNR;
        display["frequency_mhz"] = displayStatus.frequency;
        display["bandwidth_khz"] = displayStatus.bandwidth;
        display["spreading_factor"] = displayStatus.spreadingFactor;
        display["coding_rate"] = displayStatus.codingRate;
        display["tx_power_dbm"] = displayStatus.txPower;
        display["uptime_ms"] = displayStatus.uptimeMillis;

        JsonObject battery = display.createNestedObject("battery");
        battery["voltage"] = displayStatus.batteryVoltage;
        battery["percent"] = displayStatus.batteryPercent;

        JsonObject powerOff = display.createNestedObject("power_off");
        powerOff["active"] = displayStatus.powerOffActive;
        powerOff["progress"] = displayStatus.powerOffProgress;
        powerOff["complete"] = displayStatus.powerOffComplete;

        JsonObject gnss = display.createNestedObject("gnss");
        gnss["enabled"] = displayStatus.gnssEnabled;
        gnss["has_fix"] = displayStatus.gnssHasFix;
        gnss["is_3d_fix"] = displayStatus.gnssIs3DFix;
        if (!isnan(displayStatus.gnssLatitude))
        {
            gnss["latitude"] = displayStatus.gnssLatitude;
        }
        else
        {
            gnss["latitude"] = nullptr;
        }
        if (!isnan(displayStatus.gnssLongitude))
        {
            gnss["longitude"] = displayStatus.gnssLongitude;
        }
        else
        {
            gnss["longitude"] = nullptr;
        }
        if (!isnan(displayStatus.gnssAltitude))
        {
            gnss["altitude_m"] = displayStatus.gnssAltitude;
        }
        else
        {
            gnss["altitude_m"] = nullptr;
        }
        gnss["speed_knots"] = displayStatus.gnssSpeed;
        gnss["course_degrees"] = displayStatus.gnssCourse;
        gnss["hdop"] = displayStatus.gnssHdop;
        gnss["satellites"] = displayStatus.gnssSatellites;
        gnss["time_valid"] = displayStatus.gnssTimeValid;
        gnss["time_synced"] = displayStatus.gnssTimeSynced;
        gnss["year"] = displayStatus.gnssYear;
        gnss["month"] = displayStatus.gnssMonth;
        gnss["day"] = displayStatus.gnssDay;
        gnss["hour"] = displayStatus.gnssHour;
        gnss["minute"] = displayStatus.gnssMinute;
        gnss["second"] = displayStatus.gnssSecond;
        gnss["pps_available"] = displayStatus.gnssPpsAvailable;
        gnss["pps_last_millis"] = displayStatus.gnssPpsLastMillis;
        gnss["pps_count"] = displayStatus.gnssPpsCount;
    }
    else
    {
        tnc["available"] = false;
        tnc["status_text"] = "TNC manager unavailable";
    }

    JsonObject ui = doc.createNestedObject("ui");
    ui["theme"] = uiThemeOverride ? uiThemePreference : "system";
    ui["override"] = uiThemeOverride;

    sendJsonToClient(client, doc);
}

void WebInterfaceManager::sendErrorMessage(AsyncWebSocketClient *client, const String &message, const String &requestId)
{
    if (!client)
    {
        return;
    }

    DynamicJsonDocument doc(192);
    doc["type"] = "error";
    doc["timestamp"] = millis();
    doc["message"] = message;
    doc["client_id"] = client->id();
    if (!requestId.isEmpty())
    {
        doc["id"] = requestId;
    }

    sendJsonToClient(client, doc);
}

void WebInterfaceManager::sendErrorMessage(AsyncWebSocketClient *client, const __FlashStringHelper *message,
                                           const String &requestId)
{
    sendErrorMessage(client, String(message), requestId);
}

void WebInterfaceManager::sendJsonToClient(AsyncWebSocketClient *client, DynamicJsonDocument &doc)
{
    String payload;
    serializeJson(doc, payload);

    if (client)
    {
        client->text(payload);
    }
    else
    {
        webSocket.textAll(payload);
    }
}

void WebInterfaceManager::broadcastStatus(const DisplayManager::StatusData &status)
{
    const unsigned long now = millis();
    bool firstBroadcast = !hasBroadcastStatus;

    if (webSocket.count() == 0)
    {
        lastBroadcastStatus = status;
        lastStatusBroadcast = now;
        hasBroadcastStatus = true;
        return;
    }

    if (!firstBroadcast && (now - lastStatusBroadcast) < STATUS_BROADCAST_INTERVAL_MS)
    {
        return;
    }

    DynamicJsonDocument doc(1536);
    doc["type"] = "status";
    doc["timestamp"] = now;
    doc["client_count"] = webSocket.count();

    bool anyChange = firstBroadcast;
    JsonObject display;
    bool displayCreated = false;
    JsonObject battery;
    bool batteryCreated = false;
    JsonObject powerOff;
    bool powerOffCreated = false;
    JsonObject gnss;
    bool gnssCreated = false;

    auto ensureDisplay = [&]() -> JsonObject {
        if (!displayCreated)
        {
            display = doc.createNestedObject("display");
            displayCreated = true;
        }
        return display;
    };

    auto ensureBattery = [&]() -> JsonObject {
        if (!batteryCreated)
        {
            battery = ensureDisplay().createNestedObject("battery");
            batteryCreated = true;
        }
        return battery;
    };

    auto ensurePowerOff = [&]() -> JsonObject {
        if (!powerOffCreated)
        {
            powerOff = ensureDisplay().createNestedObject("power_off");
            powerOffCreated = true;
        }
        return powerOff;
    };

    auto ensureGnss = [&]() -> JsonObject {
        if (!gnssCreated)
        {
            gnss = ensureDisplay().createNestedObject("gnss");
            gnssCreated = true;
        }
        return gnss;
    };

    if (firstBroadcast || status.mode != lastBroadcastStatus.mode)
    {
        ensureDisplay()["mode"] = modeToString(status.mode);
        anyChange = true;
    }

    if (firstBroadcast || status.txCount != lastBroadcastStatus.txCount)
    {
        ensureDisplay()["tx_count"] = status.txCount;
        anyChange = true;
    }

    if (firstBroadcast || status.rxCount != lastBroadcastStatus.rxCount)
    {
        ensureDisplay()["rx_count"] = status.rxCount;
        anyChange = true;
    }

    if (firstBroadcast || status.lastPacketMillis != lastBroadcastStatus.lastPacketMillis)
    {
        ensureDisplay()["last_packet_millis"] = status.lastPacketMillis;
        anyChange = true;
    }

    if (firstBroadcast || status.hasRecentPacket != lastBroadcastStatus.hasRecentPacket)
    {
        ensureDisplay()["has_recent_packet"] = status.hasRecentPacket;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.lastRSSI, lastBroadcastStatus.lastRSSI, 0.1f))
    {
        ensureDisplay()["last_rssi"] = status.lastRSSI;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.lastSNR, lastBroadcastStatus.lastSNR, 0.1f))
    {
        ensureDisplay()["last_snr"] = status.lastSNR;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.frequency, lastBroadcastStatus.frequency, 0.001f))
    {
        ensureDisplay()["frequency_mhz"] = status.frequency;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.bandwidth, lastBroadcastStatus.bandwidth, 0.1f))
    {
        ensureDisplay()["bandwidth_khz"] = status.bandwidth;
        anyChange = true;
    }

    if (firstBroadcast || status.spreadingFactor != lastBroadcastStatus.spreadingFactor)
    {
        ensureDisplay()["spreading_factor"] = status.spreadingFactor;
        anyChange = true;
    }

    if (firstBroadcast || status.codingRate != lastBroadcastStatus.codingRate)
    {
        ensureDisplay()["coding_rate"] = status.codingRate;
        anyChange = true;
    }

    if (firstBroadcast || status.txPower != lastBroadcastStatus.txPower)
    {
        ensureDisplay()["tx_power_dbm"] = status.txPower;
        anyChange = true;
    }

    if (firstBroadcast || status.uptimeMillis != lastBroadcastStatus.uptimeMillis)
    {
        ensureDisplay()["uptime_ms"] = status.uptimeMillis;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.batteryVoltage, lastBroadcastStatus.batteryVoltage, 0.01f))
    {
        ensureBattery()["voltage"] = status.batteryVoltage;
        anyChange = true;
    }

    if (firstBroadcast || status.batteryPercent != lastBroadcastStatus.batteryPercent)
    {
        ensureBattery()["percent"] = status.batteryPercent;
        anyChange = true;
    }

    if (firstBroadcast || status.powerOffActive != lastBroadcastStatus.powerOffActive)
    {
        ensurePowerOff()["active"] = status.powerOffActive;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.powerOffProgress, lastBroadcastStatus.powerOffProgress, 0.01f))
    {
        ensurePowerOff()["progress"] = status.powerOffProgress;
        anyChange = true;
    }

    if (firstBroadcast || status.powerOffComplete != lastBroadcastStatus.powerOffComplete)
    {
        ensurePowerOff()["complete"] = status.powerOffComplete;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssEnabled != lastBroadcastStatus.gnssEnabled)
    {
        ensureGnss()["enabled"] = status.gnssEnabled;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssHasFix != lastBroadcastStatus.gnssHasFix)
    {
        ensureGnss()["has_fix"] = status.gnssHasFix;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssIs3DFix != lastBroadcastStatus.gnssIs3DFix)
    {
        ensureGnss()["is_3d_fix"] = status.gnssIs3DFix;
        anyChange = true;
    }

    if (firstBroadcast || doubleChanged(status.gnssLatitude, lastBroadcastStatus.gnssLatitude, 0.000001))
    {
        if (isnan(status.gnssLatitude))
        {
            ensureGnss()["latitude"] = nullptr;
        }
        else
        {
            ensureGnss()["latitude"] = status.gnssLatitude;
        }
        anyChange = true;
    }

    if (firstBroadcast || doubleChanged(status.gnssLongitude, lastBroadcastStatus.gnssLongitude, 0.000001))
    {
        if (isnan(status.gnssLongitude))
        {
            ensureGnss()["longitude"] = nullptr;
        }
        else
        {
            ensureGnss()["longitude"] = status.gnssLongitude;
        }
        anyChange = true;
    }

    if (firstBroadcast || doubleChanged(status.gnssAltitude, lastBroadcastStatus.gnssAltitude, 0.01))
    {
        if (isnan(status.gnssAltitude))
        {
            ensureGnss()["altitude_m"] = nullptr;
        }
        else
        {
            ensureGnss()["altitude_m"] = status.gnssAltitude;
        }
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.gnssSpeed, lastBroadcastStatus.gnssSpeed, 0.01f))
    {
        ensureGnss()["speed_knots"] = status.gnssSpeed;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.gnssCourse, lastBroadcastStatus.gnssCourse, 0.1f))
    {
        ensureGnss()["course_degrees"] = status.gnssCourse;
        anyChange = true;
    }

    if (firstBroadcast || floatChanged(status.gnssHdop, lastBroadcastStatus.gnssHdop, 0.01f))
    {
        ensureGnss()["hdop"] = status.gnssHdop;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssSatellites != lastBroadcastStatus.gnssSatellites)
    {
        ensureGnss()["satellites"] = status.gnssSatellites;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssTimeValid != lastBroadcastStatus.gnssTimeValid)
    {
        ensureGnss()["time_valid"] = status.gnssTimeValid;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssTimeSynced != lastBroadcastStatus.gnssTimeSynced)
    {
        ensureGnss()["time_synced"] = status.gnssTimeSynced;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssYear != lastBroadcastStatus.gnssYear)
    {
        ensureGnss()["year"] = status.gnssYear;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssMonth != lastBroadcastStatus.gnssMonth)
    {
        ensureGnss()["month"] = status.gnssMonth;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssDay != lastBroadcastStatus.gnssDay)
    {
        ensureGnss()["day"] = status.gnssDay;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssHour != lastBroadcastStatus.gnssHour)
    {
        ensureGnss()["hour"] = status.gnssHour;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssMinute != lastBroadcastStatus.gnssMinute)
    {
        ensureGnss()["minute"] = status.gnssMinute;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssSecond != lastBroadcastStatus.gnssSecond)
    {
        ensureGnss()["second"] = status.gnssSecond;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssPpsAvailable != lastBroadcastStatus.gnssPpsAvailable)
    {
        ensureGnss()["pps_available"] = status.gnssPpsAvailable;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssPpsLastMillis != lastBroadcastStatus.gnssPpsLastMillis)
    {
        ensureGnss()["pps_last_millis"] = status.gnssPpsLastMillis;
        anyChange = true;
    }

    if (firstBroadcast || status.gnssPpsCount != lastBroadcastStatus.gnssPpsCount)
    {
        ensureGnss()["pps_count"] = status.gnssPpsCount;
        anyChange = true;
    }

    if (!anyChange)
    {
        lastStatusBroadcast = now;
        hasBroadcastStatus = true;
        return;
    }

    lastBroadcastStatus = status;
    lastStatusBroadcast = now;
    hasBroadcastStatus = true;

    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::notifyCommandResult(const String &command, TNCCommandResult result, const String &source,
                                              AsyncWebSocketClient *client, const String &requestId)
{
    if (!client && webSocket.count() == 0)
    {
        return;
    }

    CommandHttpMapping mapping = mapCommandResultToHttp(result);

    DynamicJsonDocument doc(320);
    doc["type"] = "command_result";
    doc["timestamp"] = millis();
    doc["command"] = command;
    doc["source"] = source;
    doc["success"] = mapping.success;
    doc["result"] = commandResultToString(result);
    doc["message"] = mapping.message;
    if (!requestId.isEmpty())
    {
        doc["id"] = requestId;
    }
    if (client)
    {
        doc["client_id"] = client->id();
    }

    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::notifyConfigurationResult(const String &command, bool success, const String &source,
                                                    AsyncWebSocketClient *client, const String &requestId)
{
    if (!client && webSocket.count() == 0)
    {
        return;
    }

    DynamicJsonDocument doc(512);
    doc["type"] = "config_result";
    doc["timestamp"] = millis();
    doc["command"] = command;
    doc["source"] = source;
    doc["success"] = success;
    doc["message"] = success ? "Configuration command accepted." : "Configuration command rejected.";
    if (!requestId.isEmpty())
    {
        doc["id"] = requestId;
    }
    if (client)
    {
        doc["client_id"] = client->id();
    }
    if (tncManager)
    {
        doc["status_text"] = tncManager->getConfigurationStatus();
    }

    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::broadcastAlert(const String &category, const String &message, bool state)
{
    if (webSocket.count() == 0)
    {
        return;
    }

    DynamicJsonDocument doc(256);
    doc["type"] = "alert";
    doc["timestamp"] = millis();
    doc["category"] = category;
    doc["message"] = message;
    doc["state"] = state;

    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::broadcastPacketNotification(size_t length, float rssi, float snr, const String &preview)
{
    if (webSocket.count() == 0)
    {
        return;
    }

    DynamicJsonDocument doc(320);
    doc["type"] = "packet";
    doc["timestamp"] = millis();
    doc["length"] = static_cast<uint32_t>(length);
    doc["rssi"] = rssi;
    doc["snr"] = snr;
    if (!preview.isEmpty())
    {
        doc["preview"] = preview;
    }

    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::setupWebSocket()
{
    webSocket.onEvent([this](AsyncWebSocket *, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                              uint8_t *data, size_t len) {
        switch (type)
        {
        case WS_EVT_CONNECT:
            handleWebSocketConnect(client);
            break;
        case WS_EVT_DISCONNECT:
            handleWebSocketDisconnect(client);
            break;
        case WS_EVT_DATA:
        {
            if (!client)
            {
                break;
            }

            AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);
            if (info && info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
            {
                handleWebSocketMessage(client, reinterpret_cast<const char *>(data), len);
            }
            else if (info && info->opcode == WS_BINARY)
            {
                sendErrorMessage(client, F("Binary messages are not supported."));
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
        default:
            break;
        }
    });

    server.addHandler(&webSocket);
}

void WebInterfaceManager::handleWebSocketConnect(AsyncWebSocketClient *client)
{
    if (!client)
    {
        return;
    }

    DynamicJsonDocument doc(512);
    doc["type"] = "hello";
    doc["timestamp"] = millis();
    doc["client_id"] = client->id();
    doc["ap_ssid"] = apSSID;
    doc["sta_connected"] = staConnected;
    doc["ui_theme"] = uiThemeOverride ? uiThemePreference : "system";
    doc["ui_theme_override"] = uiThemeOverride;
    if (tncManager)
    {
        doc["tnc_available"] = true;
        doc["tnc_status"] = tncManager->getStatus();
    }
    else
    {
        doc["tnc_available"] = false;
        doc["tnc_status"] = "TNC manager unavailable";
    }

    sendJsonToClient(client, doc);

    sendStatusSnapshot(client);
}

void WebInterfaceManager::handleWebSocketDisconnect(AsyncWebSocketClient *client)
{
    if (!client)
    {
        return;
    }

    DynamicJsonDocument doc(160);
    doc["type"] = "client_disconnected";
    doc["timestamp"] = millis();
    doc["client_id"] = client->id();
    sendJsonToClient(nullptr, doc);
}

void WebInterfaceManager::handleWebSocketMessage(AsyncWebSocketClient *client, const char *data, size_t len)
{
    if (!client)
    {
        return;
    }

    DynamicJsonDocument doc(768);
    DeserializationError error = deserializeJson(doc, data, len);
    if (error)
    {
        sendErrorMessage(client, F("Invalid JSON payload."));
        return;
    }

    if (!doc.containsKey("type") || !doc["type"].is<const char *>())
    {
        sendErrorMessage(client, F("Missing type field."));
        return;
    }

    String type = doc["type"].as<String>();
    type.toLowerCase();

    String requestId;
    if (doc.containsKey("id"))
    {
        requestId = doc["id"].as<String>();
    }

    if (type == "command")
    {
        if (!doc.containsKey("command") || !doc["command"].is<const char *>())
        {
            sendErrorMessage(client, F("Missing command field."), requestId);
            return;
        }

        if (!tncManager)
        {
            sendErrorMessage(client, F("TNC manager unavailable."), requestId);
            return;
        }

        String command = doc["command"].as<const char *>();
        TNCCommandResult result = tncManager->executeCommand(command.c_str());
        notifyCommandResult(command, result, "websocket", client, requestId);
        return;
    }

    if (type == "config")
    {
        if (!doc.containsKey("command") || !doc["command"].is<const char *>())
        {
            sendErrorMessage(client, F("Missing command field."), requestId);
            return;
        }

        if (!tncManager)
        {
            sendErrorMessage(client, F("TNC manager unavailable."), requestId);
            return;
        }

        String command = doc["command"].as<const char *>();
        bool success = tncManager->processConfigurationCommand(command.c_str());
        notifyConfigurationResult(command, success, "websocket", client, requestId);
        return;
    }

    if (type == "status_request" || type == "status")
    {
        sendStatusSnapshot(client, requestId);
        return;
    }

    if (type == "ping")
    {
        DynamicJsonDocument response(160);
        response["type"] = "pong";
        if (!requestId.isEmpty())
        {
            response["id"] = requestId;
        }
        response["timestamp"] = millis();
        response["client_id"] = client->id();
        sendJsonToClient(client, response);
        return;
    }

    if (type == "ui_theme" || type == "theme")
    {
        if (!doc.containsKey("theme"))
        {
            sendErrorMessage(client, F("Missing theme field."), requestId);
            return;
        }

        String incoming = doc["theme"].as<String>();
        String normalised = normaliseThemePreference(incoming);
        if (normalised.isEmpty())
        {
            sendErrorMessage(client, F("Unsupported theme value."), requestId);
            return;
        }

        uiThemeOverride = normalised != "system";
        uiThemePreference = uiThemeOverride ? normalised : "system";

        DynamicJsonDocument response(224);
        response["type"] = "ui_theme";
        if (!requestId.isEmpty())
        {
            response["id"] = requestId;
        }
        response["timestamp"] = millis();
        response["theme"] = uiThemeOverride ? uiThemePreference : "system";
        response["override"] = uiThemeOverride;
        response["source"] = "websocket";
        response["client_id"] = client->id();
        sendJsonToClient(nullptr, response);
        return;
    }

    sendErrorMessage(client, F("Unsupported message type."), requestId);
}
