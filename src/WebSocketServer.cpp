#include "WebSocketServer.h"
#include "Radio.h"
#include "GNSS.h"
#include "KISS.h"
#include "Config.h"
#include "Battery.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

// Static member initialization
AsyncWebSocket* WebSocketServer::ws = nullptr;
AsyncWebServer* WebSocketServer::webServer = nullptr;
RadioHAL* WebSocketServer::radioRef = nullptr;
GNSSDriver* WebSocketServer::gnssRef = nullptr;
KISS* WebSocketServer::kissRef = nullptr;
ConfigManager* WebSocketServer::configRef = nullptr;
unsigned long WebSocketServer::lastStatusBroadcast = 0;
unsigned long WebSocketServer::lastSystemInfo = 0;

void WebSocketServer::begin(AsyncWebServer& server) {
    webServer = &server;
    
    // Initialize SPIFFS for web file serving
    if (!SPIFFS.begin(true)) {
        Serial.println("[WS] Failed to mount SPIFFS");
        return;
    }
    
    // Create WebSocket endpoint
    ws = new AsyncWebSocket("/ws");
    ws->onEvent(onWebSocketEvent);
    server.addHandler(ws);
    
    // Set up web file routes
    setupWebRoutes();
    
    Serial.println("[WS] WebSocket server initialized");
}

void WebSocketServer::handle() {
    if (!ws) return;
    
    // Clean up dead connections
    ws->cleanupClients();
    
    unsigned long now = millis();
    
    // Broadcast status updates periodically
    if (now - lastStatusBroadcast >= STATUS_INTERVAL) {
        if (ws->count() > 0) {
            broadcastSystemStatus();
            broadcastRadioStatus();
            broadcastGNSSStatus();
            broadcastBatteryStatus();
            broadcastWiFiStatus();
        }
        lastStatusBroadcast = now;
    }
}

void WebSocketServer::setRadio(RadioHAL* radio) {
    radioRef = radio;
}

void WebSocketServer::setGNSS(GNSSDriver* gnss) {
    gnssRef = gnss;
}

void WebSocketServer::setKISS(KISS* kiss) {
    kissRef = kiss;
}

void WebSocketServer::setConfig(ConfigManager* config) {
    configRef = config;
}

uint32_t WebSocketServer::getClientCount() {
    return ws ? ws->count() : 0;
}

void WebSocketServer::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n", client->id(), 
                         client->remoteIP().toString().c_str());
            // Send initial status to new client
            handleStatusRequest(client, "all_status");
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                String message = String((char*)data, len);
                handleWebSocketMessage(client, message);
            }
            break;
        }
        
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebSocketServer::handleWebSocketMessage(AsyncWebSocketClient* client, const String& message) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("[WS] Failed to parse JSON message");
        sendError(client, "Invalid JSON format");
        return;
    }
    
    processMessage(client, doc);
}

void WebSocketServer::processMessage(AsyncWebSocketClient* client, JsonDocument& doc) {
    if (!doc["type"].is<String>()) {
        sendError(client, "Missing message type");
        return;
    }
    
    String type = doc["type"].as<String>();
    
    if (type == "request") {
        if (doc["data"].is<String>()) {
            handleStatusRequest(client, doc["data"].as<String>());
        }
    }
    else if (type == "config") {
        if (doc["category"].is<String>() && doc["data"].is<JsonObject>()) {
            JsonObject data = doc["data"].as<JsonObject>();
            handleConfigUpdate(client, doc["category"].as<String>(), data);
        }
    }
    else if (type == "command") {
        if (doc["action"].is<String>()) {
            JsonObject data = doc["data"].is<JsonObject>() ? doc["data"].as<JsonObject>() : JsonObject();
            handleCommand(client, doc["action"].as<String>(), data);
        }
    }
    else {
        sendError(client, "Unknown message type: " + type);
    }
}

void WebSocketServer::handleStatusRequest(AsyncWebSocketClient* client, const String& dataType) {
    if (dataType == "all_status" || dataType == "status") {
        // Send all status data
        JsonDocument doc;
        JsonObject payload = createSystemStatus(doc);
        sendResponse(client, "status", payload);
        
        doc.clear();
        payload = createRadioStatus(doc);
        sendResponse(client, "radio", payload);
        
        doc.clear();
        payload = createGNSSStatus(doc);
        sendResponse(client, "gnss", payload);
        
        doc.clear();
        payload = createBatteryStatus(doc);
        sendResponse(client, "battery", payload);
        
        doc.clear();
        payload = createWiFiStatus(doc);
        sendResponse(client, "wifi", payload);
    }
    else if (dataType == "radio") {
        JsonDocument doc;
        JsonObject payload = createRadioStatus(doc);
        sendResponse(client, "radio", payload);
    }
    // Add other specific status requests as needed
}

void WebSocketServer::handleConfigUpdate(AsyncWebSocketClient* client, const String& category, JsonObject data) {
    if (category == "radio") {
        updateRadioConfig(data);
    }
    else if (category == "aprs") {
        updateAPRSConfig(data);
    }
    else if (category == "wifi") {
        updateWiFiConfig(data);
    }
    else if (category == "system") {
        updateSystemConfig(data);
    }
    else {
        sendError(client, "Unknown config category: " + category);
        return;
    }
    
    // Send success response
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["success"] = true;
    response["category"] = category;
    sendResponse(client, "config_result", response);
}

void WebSocketServer::handleCommand(AsyncWebSocketClient* client, const String& action, JsonObject data) {
    if (action == "restart") {
        handleRestart();
    }
    else if (action == "factory_reset") {
        handleFactoryReset();
    }
    else if (action == "send_aprs_message") {
        handleSendAPRSMessage(data);
    }
    else if (action == "backup_config") {
        handleBackupConfig(client);
    }
    else {
        sendError(client, "Unknown command: " + action);
        return;
    }
    
    // Send success response for most commands
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["success"] = true;
    response["action"] = action;
    sendResponse(client, "command_result", response);
}

void WebSocketServer::updateRadioConfig(JsonObject data) {
    if (!radioRef) return;
    
    if (data["frequency"].is<float>()) {
        float freq = data["frequency"].as<float>();
        radioRef->setFrequency(freq);
    }
    
    if (data["power"].is<int8_t>()) {
        int8_t power = data["power"].as<int8_t>();
        radioRef->setTxPower(power);
    }
    
    if (data["bandwidth"].is<float>()) {
        float bw = data["bandwidth"].as<float>() / 1000.0; // Convert Hz to kHz
        radioRef->setBandwidth(bw);
    }
    
    if (data["spreading_factor"].is<uint8_t>()) {
        uint8_t sf = data["spreading_factor"].as<uint8_t>();
        radioRef->setSpreadingFactor(sf);
    }
    
    if (data["coding_rate"].is<uint8_t>()) {
        uint8_t cr = data["coding_rate"].as<uint8_t>();
        radioRef->setCodingRate(cr);
    }
    
    // Save configuration
    if (configRef) {
        configRef->saveConfig();
    }
}

void WebSocketServer::updateAPRSConfig(JsonObject data) {
    if (!configRef) return;
    
    // Update APRS configuration in ConfigManager
    if (data["callsign"].is<String>()) {
        String callsign = data["callsign"].as<String>();
        // Set callsign in config
    }
    
    if (data["ssid"].is<uint8_t>()) {
        uint8_t ssid = data["ssid"].as<uint8_t>();
        // Set SSID in config
    }
    
    if (data["beacon_interval"].is<uint16_t>()) {
        uint16_t interval = data["beacon_interval"].as<uint16_t>();
        // Set beacon interval in config
    }
    
    if (data["beacon_text"].is<String>()) {
        String text = data["beacon_text"].as<String>();
        // Set beacon text in config
    }
    
    if (data["auto_beacon"].is<bool>()) {
        bool enabled = data["auto_beacon"].as<bool>();
        // Set auto beacon flag in config
    }
    
    configRef->saveConfig();
}

void WebSocketServer::updateWiFiConfig(JsonObject data) {
    if (!configRef) return;
    
    // Update WiFi configuration - requires restart to take effect
    if (data["ssid"].is<String>()) {
        String ssid = data["ssid"].as<String>();
        // Set WiFi SSID in config
    }
    
    if (data["password"].is<String>()) {
        String password = data["password"].as<String>();
        // Set WiFi password in config
    }
    
    if (data["ap_mode"].is<bool>()) {
        bool apMode = data["ap_mode"].as<bool>();
        // Set AP mode flag in config
    }
    
    configRef->saveConfig();
}

void WebSocketServer::updateSystemConfig(JsonObject data) {
    if (!configRef) return;
    
    // Update system configuration
    if (data["oled_enabled"].is<bool>()) {
        bool enabled = data["oled_enabled"].as<bool>();
        // Set OLED enable flag in config
    }
    
    if (data["gnss_enabled"].is<bool>()) {
        bool enabled = data["gnss_enabled"].as<bool>();
        // Set GNSS enable flag in config
    }
    
    if (data["timezone"].is<String>()) {
        String timezone = data["timezone"].as<String>();
        // Set timezone in config
    }
    
    configRef->saveConfig();
}

void WebSocketServer::handleRestart() {
    Serial.println("[WS] Restart command received");
    delay(1000); // Give time for response to be sent
    ESP.restart();
}

void WebSocketServer::handleFactoryReset() {
    Serial.println("[WS] Factory reset command received");
    if (configRef) {
        configRef->resetToDefaults();
        configRef->saveConfig();
    }
    delay(1000);
    ESP.restart();
}

void WebSocketServer::handleSendAPRSMessage(JsonObject data) {
    if (!data["to"].is<String>() || !data["text"].is<String>()) {
        return;
    }
    
    String to = data["to"].as<String>();
    String text = data["text"].as<String>();
    
    // TODO: Implement APRS message sending via radio
    Serial.printf("[WS] APRS message to %s: %s\n", to.c_str(), text.c_str());
}

void WebSocketServer::handleBackupConfig(AsyncWebSocketClient* client) {
    if (!configRef) return;
    
    // TODO: Implement configuration backup
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["config_data"] = "backup_data_placeholder";
    sendResponse(client, "config_backup", response);
}

void WebSocketServer::sendResponse(AsyncWebSocketClient* client, const String& type, JsonObject& payload) {
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["type"] = type;
    response["payload"] = payload;
    
    String message;
    serializeJson(response, message);
    client->text(message);
}

void WebSocketServer::sendError(AsyncWebSocketClient* client, const String& message) {
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["type"] = "error";
    JsonObject payload = response["payload"].to<JsonObject>();
    payload["message"] = message;
    
    String jsonMessage;
    serializeJson(response, jsonMessage);
    client->text(jsonMessage);
}

void WebSocketServer::broadcastMessage(const String& type, JsonObject& payload) {
    if (!ws || ws->count() == 0) return;
    
    JsonDocument doc;
    JsonObject response = doc.to<JsonObject>();
    response["type"] = type;
    response["payload"] = payload;
    
    String message;
    serializeJson(response, message);
    ws->textAll(message);
}

void WebSocketServer::broadcastSystemStatus() {
    JsonDocument doc;
    JsonObject payload = createSystemStatus(doc);
    broadcastMessage("status", payload);
}

void WebSocketServer::broadcastRadioStatus() {
    JsonDocument doc;
    JsonObject payload = createRadioStatus(doc);
    broadcastMessage("radio", payload);
}

void WebSocketServer::broadcastAPRSMessage(const String& from, const String& to, const String& message, int16_t rssi) {
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    JsonObject aprsData = payload["messages"].to<JsonObject>();
    
    JsonArray messages = aprsData["messages"].to<JsonArray>();
    JsonObject msg = messages.add<JsonObject>();
    msg["timestamp"] = millis();
    msg["from"] = from;
    msg["to"] = to;
    msg["message"] = message;
    msg["rssi"] = rssi;
    
    broadcastMessage("aprs", payload);
}

void WebSocketServer::broadcastGNSSStatus() {
    JsonDocument doc;
    JsonObject payload = createGNSSStatus(doc);
    broadcastMessage("gnss", payload);
}

void WebSocketServer::broadcastBatteryStatus() {
    JsonDocument doc;
    JsonObject payload = createBatteryStatus(doc);
    broadcastMessage("battery", payload);
}

void WebSocketServer::broadcastWiFiStatus() {
    JsonDocument doc;
    JsonObject payload = createWiFiStatus(doc);
    broadcastMessage("wifi", payload);
}

void WebSocketServer::broadcastLogMessage(const String& level, const String& message) {
    JsonDocument doc;
    JsonObject payload = doc.to<JsonObject>();
    payload["level"] = level;
    payload["message"] = message;
    payload["timestamp"] = millis();
    
    broadcastMessage("log", payload);
}

JsonObject WebSocketServer::createSystemStatus(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();
    
    payload["firmware_version"] = "1.0.0"; // TODO: Get from Constants.h
    payload["uptime"] = millis() / 1000;
    payload["free_heap"] = getFreeHeap();
    payload["flash_used"] = getFlashUsed();
    payload["flash_total"] = getFlashSize();
    payload["spiffs_used"] = getSPIFFSUsed();
    payload["spiffs_total"] = getSPIFFSSize();
    payload["cpu_temp"] = getCPUTemperature();
    
    return payload;
}

JsonObject WebSocketServer::createRadioStatus(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();
    
    if (radioRef) {
        // TODO: Get actual values from radio
        payload["frequency"] = 433.775;
        payload["power"] = 20;
        payload["rssi"] = -95;
        payload["snr"] = 8.5;
        payload["packets_tx"] = 0;
        payload["packets_rx"] = 0;
        payload["crc_errors"] = 0;
        payload["last_tx"] = 0;
        payload["last_rx"] = 0;
    }
    
    return payload;
}

JsonObject WebSocketServer::createGNSSStatus(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();
    
    if (gnssRef) {
        // TODO: Get actual values from GNSS
        payload["fix"] = false;
        payload["satellites"] = 0;
        payload["latitude"] = 0.0;
        payload["longitude"] = 0.0;
        payload["altitude"] = 0.0;
        payload["speed"] = 0.0;
        payload["course"] = 0.0;
    }
    
    return payload;
}

JsonObject WebSocketServer::createBatteryStatus(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();
    
    // TODO: Get actual battery status
    payload["voltage"] = 3.7;
    payload["level"] = 75;
    payload["charging"] = false;
    
    return payload;
}

JsonObject WebSocketServer::createWiFiStatus(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();
    
    payload["connected"] = WiFi.status() == WL_CONNECTED;
    payload["ip"] = WiFi.localIP().toString();
    payload["ssid"] = WiFi.SSID();
    payload["rssi"] = WiFi.RSSI();
    payload["ap_mode"] = WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
    
    return payload;
}

size_t WebSocketServer::getHeapSize() {
    return ESP.getHeapSize();
}

size_t WebSocketServer::getFreeHeap() {
    return ESP.getFreeHeap();
}

size_t WebSocketServer::getFlashSize() {
    return ESP.getFlashChipSize();
}

size_t WebSocketServer::getFlashUsed() {
    return ESP.getSketchSize();
}

size_t WebSocketServer::getSPIFFSSize() {
    return SPIFFS.totalBytes();
}

size_t WebSocketServer::getSPIFFSUsed() {
    return SPIFFS.usedBytes();
}

float WebSocketServer::getCPUTemperature() {
    // ESP32-S3 temperature sensor
    return temperatureRead();
}

String WebSocketServer::formatUptime(unsigned long seconds) {
    unsigned long days = seconds / 86400;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    
    return String(days) + "d " + String(hours) + "h " + String(minutes) + "m";
}

void WebSocketServer::setupWebRoutes() {
    if (!webServer) return;
    
    // Serve web interface files from SPIFFS
    webServer->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (handleFileRead(request, "/index.html")) {
            return;
        }
        request->send(404, "text/plain", "File not found");
    });
    
    // Handle all other files
    webServer->onNotFound([](AsyncWebServerRequest* request) {
        if (handleFileRead(request, request->url())) {
            return;
        }
        request->send(404, "text/plain", "File not found");
    });
    
    Serial.println("[WS] Web routes configured");
}

bool WebSocketServer::handleFileRead(AsyncWebServerRequest* request, String path) {
    if (path.endsWith("/")) {
        path += "index.html";
    }
    
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    
    // Try compressed version first
    if (SPIFFS.exists(pathWithGz)) {
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, pathWithGz, contentType);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=86400"); // 1 day cache
        request->send(response);
        return true;
    }
    
    // Try uncompressed version
    if (SPIFFS.exists(path)) {
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, path, contentType);
        response->addHeader("Cache-Control", "max-age=86400"); // 1 day cache
        request->send(response);
        return true;
    }
    
    return false;
}

String WebSocketServer::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    else if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
}