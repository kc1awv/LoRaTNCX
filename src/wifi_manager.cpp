#include "wifi_manager.h"

const char* WiFiManager::NVS_NAMESPACE = "wifi";
const char* WiFiManager::NVS_WIFI_KEY = "config";

WiFiManager::WiFiManager() 
    : initialized(false), apStarted(false), staConnected(false), 
      lastReconnectAttempt(0), scanResults(0) {
}

bool WiFiManager::begin() {
    initialized = true;
    
    // Try to load saved configuration
    if (!loadConfig(currentConfig)) {
        // No saved config, use defaults
        resetToDefaults(currentConfig);
        // Save defaults to create the NVS namespace
        saveConfig(currentConfig);
    }
    
    return true;
}

bool WiFiManager::start() {
    if (!initialized) {
        return false;
    }
    
    return applyConfig(currentConfig);
}

void WiFiManager::stop() {
    if (apStarted) {
        WiFi.softAPdisconnect(true);
        apStarted = false;
    }
    
    if (staConnected || WiFi.status() != WL_DISCONNECTED) {
        WiFi.disconnect(true);
        staConnected = false;
    }
    
    WiFi.mode(WIFI_OFF);
}

void WiFiManager::update() {
    if (!initialized) {
        return;
    }
    
    // Check STA connection status
    if (currentConfig.mode == TNC_WIFI_STA || currentConfig.mode == TNC_WIFI_AP_STA) {
        checkConnection();
    }
}

bool WiFiManager::isConnected() {
    return staConnected && (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::isAPActive() {
    return apStarted;
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getAPIPAddress() {
    if (isAPActive()) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

bool WiFiManager::saveConfig(const WiFiConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Create a copy with magic number
    WiFiConfig configToSave = config;
    configToSave.magic = CONFIG_MAGIC;
    
    // Write configuration as a blob
    size_t written = preferences.putBytes(NVS_WIFI_KEY, &configToSave, sizeof(WiFiConfig));
    preferences.end();
    
    return (written == sizeof(WiFiConfig));
}

bool WiFiManager::loadConfig(WiFiConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    size_t len = preferences.getBytes(NVS_WIFI_KEY, &config, sizeof(WiFiConfig));
    preferences.end();
    
    // Check if we read the correct size
    if (len != sizeof(WiFiConfig)) {
        return false;
    }
    
    // Validate magic number
    if (config.magic != CONFIG_MAGIC) {
        return false;
    }
    
    // Validate mode
    if (config.mode > TNC_WIFI_AP_STA) {
        return false;
    }
    
    return true;
}

bool WiFiManager::hasValidConfig() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    WiFiConfig config;
    size_t read = preferences.getBytes(NVS_WIFI_KEY, &config, sizeof(WiFiConfig));
    preferences.end();
    
    return (read == sizeof(WiFiConfig) && config.magic == CONFIG_MAGIC);
}

void WiFiManager::resetToDefaults(WiFiConfig& config) {
    // Default AP settings
    snprintf(config.ap_ssid, sizeof(config.ap_ssid), "LoRaTNCX-%04X", 
             (uint16_t)(ESP.getEfuseMac() & 0xFFFF));
    strncpy(config.ap_password, "loratncx", sizeof(config.ap_password));
    
    // Default STA settings (empty)
    config.ssid[0] = '\0';
    config.password[0] = '\0';
    
    // Default mode: AP only
    config.mode = TNC_WIFI_AP;
    
    // Default network settings
    config.dhcp = true;
    config.ip[0] = 192; config.ip[1] = 168; config.ip[2] = 4; config.ip[3] = 1;
    config.gateway[0] = 192; config.gateway[1] = 168; config.gateway[2] = 4; config.gateway[3] = 1;
    config.subnet[0] = 255; config.subnet[1] = 255; config.subnet[2] = 255; config.subnet[3] = 0;
    config.dns[0] = 8; config.dns[1] = 8; config.dns[2] = 8; config.dns[3] = 8;
    
    // Default TCP KISS settings
    config.tcp_kiss_enabled = true;
    config.tcp_kiss_port = 8001;
    
    config.magic = CONFIG_MAGIC;
}

bool WiFiManager::clearConfig() {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    bool result = preferences.remove(NVS_WIFI_KEY);
    preferences.end();
    
    return result;
}

bool WiFiManager::applyConfig(const WiFiConfig& config) {
    // Stop existing connections
    stop();
    
    // Update current config
    currentConfig = config;
    
    // Start WiFi based on mode
    bool success = true;
    
    switch (config.mode) {
        case TNC_WIFI_OFF:
            // WiFi is already off from stop()
            break;
            
        case TNC_WIFI_AP:
            WiFi.mode(WIFI_AP);
            success = startAP();
            break;
            
        case TNC_WIFI_STA:
            WiFi.mode(WIFI_STA);
            success = startSTA();
            break;
            
        case TNC_WIFI_AP_STA:
            WiFi.mode(WIFI_AP_STA);
            success = startAP() && startSTA();
            break;
            
        default:
            success = false;
            break;
    }
    
    return success;
}

void WiFiManager::getCurrentConfig(WiFiConfig& config) {
    config = currentConfig;
}

int WiFiManager::scanNetworks() {
    scanResults = WiFi.scanNetworks();
    return scanResults;
}

String WiFiManager::getScannedSSID(int index) {
    if (index >= 0 && index < scanResults) {
        return WiFi.SSID(index);
    }
    return "";
}

int WiFiManager::getScannedRSSI(int index) {
    if (index >= 0 && index < scanResults) {
        return WiFi.RSSI(index);
    }
    return 0;
}

bool WiFiManager::getScannedEncryption(int index) {
    if (index >= 0 && index < scanResults) {
        return WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    }
    return false;
}

bool WiFiManager::startAP() {
    // Configure AP
    bool success = WiFi.softAP(currentConfig.ap_ssid, currentConfig.ap_password);
    
    if (success) {
        // Configure AP IP
        IPAddress ip(currentConfig.ip[0], currentConfig.ip[1], currentConfig.ip[2], currentConfig.ip[3]);
        IPAddress gateway(currentConfig.gateway[0], currentConfig.gateway[1], currentConfig.gateway[2], currentConfig.gateway[3]);
        IPAddress subnet(currentConfig.subnet[0], currentConfig.subnet[1], currentConfig.subnet[2], currentConfig.subnet[3]);
        
        WiFi.softAPConfig(ip, gateway, subnet);
        apStarted = true;
    }
    
    return success;
}

bool WiFiManager::startSTA() {
    // Check if SSID is configured
    if (strlen(currentConfig.ssid) == 0) {
        return false;
    }
    
    // Configure static IP if DHCP is disabled
    if (!currentConfig.dhcp) {
        IPAddress ip(currentConfig.ip[0], currentConfig.ip[1], currentConfig.ip[2], currentConfig.ip[3]);
        IPAddress gateway(currentConfig.gateway[0], currentConfig.gateway[1], currentConfig.gateway[2], currentConfig.gateway[3]);
        IPAddress subnet(currentConfig.subnet[0], currentConfig.subnet[1], currentConfig.subnet[2], currentConfig.subnet[3]);
        IPAddress dns(currentConfig.dns[0], currentConfig.dns[1], currentConfig.dns[2], currentConfig.dns[3]);
        
        if (!WiFi.config(ip, gateway, subnet, dns)) {
            return false;
        }
    }
    
    // Start connection (non-blocking)
    WiFi.begin(currentConfig.ssid, currentConfig.password);
    
    // Don't block waiting for connection - let it connect in the background
    // The update() method will check connection status
    staConnected = false;
    lastReconnectAttempt = millis();
    
    return true;  // Return true if we started the connection attempt
}

void WiFiManager::checkConnection() {
    // If not connected and enough time has passed, try to reconnect
    if (!staConnected || WiFi.status() != WL_CONNECTED) {
        if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            staConnected = false;
            startSTA();
        }
    } else {
        staConnected = true;
    }
}
