#include "web_server.h"
#include <SPIFFS.h>

TNCWebServer::TNCWebServer(WiFiManager* wifiMgr, LoRaRadio* radio, ConfigManager* configMgr)
    : wifiManager(wifiMgr), loraRadio(radio), configManager(configMgr), 
      server(nullptr), running(false), pendingWiFiChange(false), wifiChangeTime(0) {
}

bool TNCWebServer::begin() {
    if (running) {
        return true;
    }
    
    // Create server instance
    server = new AsyncWebServer(80);
    
    if (!server) {
        return false;
    }
    
    // Setup routes
    setupRoutes();
    
    // Start server
    server->begin();
    running = true;
    
    return true;
}

void TNCWebServer::stop() {
    if (running && server) {
        server->end();
        delete server;
        server = nullptr;
        running = false;
    }
}

void TNCWebServer::update() {
    // AsyncWebServer handles requests automatically
    
    // Check if we have a pending WiFi configuration change
    if (pendingWiFiChange && millis() - wifiChangeTime >= 500) {
        pendingWiFiChange = false;
        wifiManager->applyConfig(pendingWiFiConfig);
        // Save to NVS so it persists after reboot
        wifiManager->saveConfig(pendingWiFiConfig);
    }
}

bool TNCWebServer::isRunning() {
    return running;
}

void TNCWebServer::setupRoutes() {
    // Serve static files from SPIFFS
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
    // API routes - System status
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    server->on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetSystemInfo(request);
    });
    
    // API routes - LoRa configuration
    server->on("/api/lora/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetLoRaConfig(request);
    });
    
    server->on("/api/lora/config", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Parse JSON body
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            // Apply configuration
            bool needsReconfig = false;
            
            if (!doc["frequency"].isNull()) {
                float freq = doc["frequency"].as<float>();
                if (freq >= 150.0 && freq <= 960.0) {
                    loraRadio->setFrequency(freq);
                    needsReconfig = true;
                }
            }
            
            if (!doc["bandwidth"].isNull()) {
                float bw = doc["bandwidth"].as<float>();
                loraRadio->setBandwidth(bw);
                needsReconfig = true;
            }
            
            if (!doc["spreading"].isNull()) {
                uint8_t sf = doc["spreading"].as<uint8_t>();
                if (sf >= 7 && sf <= 12) {
                    loraRadio->setSpreadingFactor(sf);
                    needsReconfig = true;
                }
            }
            
            if (!doc["codingRate"].isNull()) {
                uint8_t cr = doc["codingRate"].as<uint8_t>();
                if (cr >= 5 && cr <= 8) {
                    loraRadio->setCodingRate(cr);
                    needsReconfig = true;
                }
            }
            
            if (!doc["power"].isNull()) {
                int8_t pwr = doc["power"].as<int8_t>();
                if (pwr >= -9 && pwr <= 22) {
                    loraRadio->setOutputPower(pwr);
                    needsReconfig = true;
                }
            }
            
            if (!doc["syncWord"].isNull()) {
                uint16_t sw = doc["syncWord"].as<uint16_t>();
                loraRadio->setSyncWord(sw);
                needsReconfig = true;
            }
            
            if (needsReconfig) {
                loraRadio->reconfigure();
            }
            
            AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
                "{\"success\":true,\"message\":\"Configuration applied\"}");
            addCORSHeaders(response);
            request->send(response);
        });
    
    server->on("/api/lora/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSaveLoRaConfig(request);
    });
    
    server->on("/api/lora/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleResetLoRaConfig(request);
    });
    
    // API routes - WiFi configuration
    server->on("/api/wifi/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetWiFiConfig(request);
    });
    
    server->on("/api/wifi/config", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Parse JSON body
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            // Get current config
            WiFiConfig config;
            wifiManager->getCurrentConfig(config);
            
            // Update fields - use more flexible type checking
            if (!doc["ssid"].isNull()) {
                const char* ssid = doc["ssid"];
                strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
                config.ssid[sizeof(config.ssid) - 1] = '\0';
            }
            
            if (!doc["password"].isNull()) {
                const char* password = doc["password"];
                strncpy(config.password, password, sizeof(config.password) - 1);
                config.password[sizeof(config.password) - 1] = '\0';
            }
            
            if (!doc["ap_ssid"].isNull()) {
                const char* ap_ssid = doc["ap_ssid"];
                strncpy(config.ap_ssid, ap_ssid, sizeof(config.ap_ssid) - 1);
                config.ap_ssid[sizeof(config.ap_ssid) - 1] = '\0';
            }
            
            if (!doc["ap_password"].isNull()) {
                const char* ap_password = doc["ap_password"];
                strncpy(config.ap_password, ap_password, sizeof(config.ap_password) - 1);
                config.ap_password[sizeof(config.ap_password) - 1] = '\0';
            }
            
            if (!doc["mode"].isNull()) {
                config.mode = doc["mode"].as<uint8_t>();
            }
            
            if (!doc["dhcp"].isNull()) {
                config.dhcp = doc["dhcp"].as<bool>();
            }
            
            // Send response immediately
            AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
                "{\"success\":true,\"message\":\"WiFi configuration will be applied\"}");
            addCORSHeaders(response);
            request->send(response);
            
            // Schedule WiFi change for later (outside of callback)
            pendingWiFiConfig = config;
            pendingWiFiChange = true;
            wifiChangeTime = millis();
        });
    
    server->on("/api/wifi/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSaveWiFiConfig(request);
    });
    
    server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleScanWiFi(request);
    });
    
    // API routes - Control
    server->on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleReboot(request);
    });
    
    // 404 handler
    server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

void TNCWebServer::handleGetStatus(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", getJSONStatus());
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleGetSystemInfo(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", getJSONSystemInfo());
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleGetLoRaConfig(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", getJSONLoRaConfig());
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleSaveLoRaConfig(AsyncWebServerRequest* request) {
    LoRaConfig currentConfig;
    loraRadio->getCurrentConfig(currentConfig);
    
    bool saved = configManager->saveConfig(currentConfig);
    
    String response;
    if (saved) {
        response = "{\"success\":true,\"message\":\"Configuration saved\"}";
    } else {
        response = "{\"success\":false,\"error\":\"Failed to save configuration\"}";
    }
    
    AsyncWebServerResponse* resp = request->beginResponse(200, "application/json", response);
    addCORSHeaders(resp);
    request->send(resp);
}

void TNCWebServer::handleResetLoRaConfig(AsyncWebServerRequest* request) {
    loraRadio->setFrequency(LORA_FREQUENCY);
    loraRadio->setBandwidth(LORA_BANDWIDTH);
    loraRadio->setSpreadingFactor(LORA_SPREADING);
    loraRadio->setCodingRate(LORA_CODINGRATE);
    loraRadio->setOutputPower(LORA_POWER);
    loraRadio->setSyncWord(LORA_SYNCWORD);
    loraRadio->reconfigure();
    
    configManager->clearConfig();
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
        "{\"success\":true,\"message\":\"Configuration reset to defaults\"}");
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleGetWiFiConfig(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", getJSONWiFiConfig());
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleSaveWiFiConfig(AsyncWebServerRequest* request) {
    WiFiConfig config;
    wifiManager->getCurrentConfig(config);
    
    if (wifiManager->saveConfig(config)) {
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
            "{\"success\":true,\"message\":\"WiFi configuration saved\"}");
        addCORSHeaders(response);
        request->send(response);
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save WiFi configuration\"}");
    }
}

void TNCWebServer::handleScanWiFi(AsyncWebServerRequest* request) {
    int n = wifiManager->scanNetworks();
    
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    
    for (int i = 0; i < n; i++) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = wifiManager->getScannedSSID(i);
        network["rssi"] = wifiManager->getScannedRSSI(i);
        network["encrypted"] = wifiManager->getScannedEncryption(i);
    }
    
    String output;
    serializeJson(doc, output);
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", output);
    addCORSHeaders(response);
    request->send(response);
}

void TNCWebServer::handleReboot(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
        "{\"success\":true,\"message\":\"Rebooting...\"}");
    addCORSHeaders(response);
    request->send(response);
    
    // Delay and reboot
    delay(1000);
    ESP.restart();
}

String TNCWebServer::getJSONStatus() {
    JsonDocument doc;
    
    // WiFi status
    doc["wifi"]["connected"] = wifiManager->isConnected();
    doc["wifi"]["ap_active"] = wifiManager->isAPActive();
    doc["wifi"]["sta_ip"] = wifiManager->getIPAddress();
    doc["wifi"]["ap_ip"] = wifiManager->getAPIPAddress();
    doc["wifi"]["rssi"] = wifiManager->getRSSI();
    
    // Battery
    doc["battery"]["voltage"] = readBatteryVoltage();
    
    // Uptime
    doc["system"]["uptime"] = millis() / 1000;
    doc["system"]["free_heap"] = ESP.getFreeHeap();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String TNCWebServer::getJSONSystemInfo() {
    JsonDocument doc;
    
    doc["board"]["name"] = BOARD_NAME;
    doc["board"]["type"] = (int)BOARD_TYPE;
    doc["chip"]["model"] = ESP.getChipModel();
    doc["chip"]["revision"] = ESP.getChipRevision();
    doc["chip"]["cores"] = ESP.getChipCores();
    doc["chip"]["frequency"] = ESP.getCpuFreqMHz();
    doc["memory"]["flash_size"] = ESP.getFlashChipSize();
    doc["memory"]["free_heap"] = ESP.getFreeHeap();
    doc["memory"]["heap_size"] = ESP.getHeapSize();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String TNCWebServer::getJSONLoRaConfig() {
    JsonDocument doc;
    
    doc["frequency"] = loraRadio->getFrequency();
    doc["bandwidth"] = loraRadio->getBandwidth();
    doc["spreading"] = loraRadio->getSpreadingFactor();
    doc["codingRate"] = loraRadio->getCodingRate();
    doc["power"] = loraRadio->getOutputPower();
    doc["syncWord"] = loraRadio->getSyncWord();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String TNCWebServer::getJSONWiFiConfig() {
    JsonDocument doc;
    
    WiFiConfig config;
    wifiManager->getCurrentConfig(config);
    
    doc["ssid"] = String(config.ssid);
    doc["ap_ssid"] = String(config.ap_ssid);
    doc["mode"] = config.mode;
    doc["dhcp"] = config.dhcp;
    doc["ip"] = String(config.ip[0]) + "." + String(config.ip[1]) + "." + 
                String(config.ip[2]) + "." + String(config.ip[3]);
    doc["gateway"] = String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + 
                     String(config.gateway[2]) + "." + String(config.gateway[3]);
    doc["subnet"] = String(config.subnet[0]) + "." + String(config.subnet[1]) + "." + 
                    String(config.subnet[2]) + "." + String(config.subnet[3]);
    doc["dns"] = String(config.dns[0]) + "." + String(config.dns[1]) + "." + 
                 String(config.dns[2]) + "." + String(config.dns[3]);
    
    // Don't send passwords in the response
    
    String output;
    serializeJson(doc, output);
    return output;
}

void TNCWebServer::addCORSHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}
