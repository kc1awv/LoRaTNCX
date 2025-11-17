#include "wifi_manager.h"
#include "debug.h"

const char* WiFiManager::NVS_NAMESPACE = "wifi";
const char* WiFiManager::NVS_WIFI_KEY = "config";

WiFiManager::WiFiManager() 
    : initialized(false), apStarted(false), staConnected(false), mdnsStarted(false),
      connectionState(WIFI_STATE_DISCONNECTED), lastReconnectAttempt(0), 
      reconnectDelay(RECONNECT_BASE_INTERVAL), reconnectAttempts(0), 
      scanResults(0), dnsServer(nullptr), scanInProgress(false), scanStartTime(0),
      fallbackToAPDone(false), isConnecting(false), lastDisconnectReason(0) {
}

Result<void> WiFiManager::begin() {
    initialized = true;
    
    // Setup WiFi event handlers
    setupWiFiEvents();
    
    // Try to load saved configuration
    if (!loadConfig(currentConfig)) {
        // No saved config, use defaults
        resetToDefaults(currentConfig);
        // Save defaults to create the NVS namespace
        if (!saveConfig(currentConfig)) {
            return Result<void>(ErrorCode::CONFIG_SAVE_FAILED);
        }
    }
    
    return Result<void>();
}

Result<void> WiFiManager::start() {
    if (!initialized) {
        return Result<void>(ErrorCode::NOT_INITIALIZED);
    }
    
    if (!applyConfig(currentConfig)) {
        return Result<void>(ErrorCode::WIFI_INIT_FAILED);
    }
    
    return Result<void>();
}

void WiFiManager::stop() {
    // Stop network interfaces first
    if (apStarted) {
        WiFi.softAPdisconnect(true);
        apStarted = false;
    }
    
    if (staConnected || (WiFi.getMode() != WIFI_OFF && WiFi.status() != WL_DISCONNECTED)) {
        WiFi.disconnect(true);
        staConnected = false;
    }
    
    // Stop services
    stopCaptivePortal();
    
    // Stop mDNS
    if (mdnsStarted) {
        // MDNS.end();  // Commented out to avoid potential crash
        mdnsStarted = false;
        DEBUG_PRINTLN("mDNS stopped");
    }
    
    WiFi.mode(WIFI_OFF);
    connectionState = WIFI_STATE_DISCONNECTED;
    statusMessage = "WiFi Off";
}

void WiFiManager::update() {
    if (!initialized) {
        return;
    }
    
    // Process DNS requests for captive portal
    if (dnsServer) {
        dnsServer->processNextRequest();
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
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Check if WiFi config key exists
    if (!preferences.isKey(NVS_WIFI_KEY)) {
        preferences.end();
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
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Check if WiFi config key exists
    if (!preferences.isKey(NVS_WIFI_KEY)) {
        preferences.end();
        return false;
    }
    
    WiFiConfig config;
    size_t read = preferences.getBytes(NVS_WIFI_KEY, &config, sizeof(WiFiConfig));
    preferences.end();
    
    return (read == sizeof(WiFiConfig) && config.magic == CONFIG_MAGIC);
}

void WiFiManager::resetToDefaults(WiFiConfig& config) {
    // Default AP settings
    snprintf(config.ap_ssid, sizeof(config.ap_ssid), "LoRaTNCX-%012llX", 
             ESP.getEfuseMac());
    generateSecurePassword(config.ap_password, sizeof(config.ap_password));
    
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

bool WiFiManager::startAsyncScan() {
    // Check if WiFi scanning is supported in current mode
    if (currentConfig.mode == TNC_WIFI_OFF) {
        return false; // Cannot scan when WiFi is disabled
    }

    // Check if scan is already in progress
    if (scanInProgress) {
        // Check for timeout (10 seconds max)
        if (millis() - scanStartTime > 10000) {
            // Scan timed out, reset state
            scanInProgress = false;
            WiFi.scanDelete(); // Clean up any pending scan
        } else {
            return false; // Scan still in progress
        }
    }

    // Start async scan (true = async, false = show_hidden, false = passive, 300 = max_ms)
    scanInProgress = true;
    scanStartTime = millis();

    if (WiFi.scanNetworks(true, false, false, 300) == WIFI_SCAN_RUNNING) {
        return true;
    } else {
        scanInProgress = false;
        return false;
    }
}

bool WiFiManager::isScanComplete() {
    if (!scanInProgress) {
        return false;
    }

    // Check for timeout (10 seconds max)
    if (millis() - scanStartTime > 10000) {
        // Scan timed out, reset state
        scanInProgress = false;
        scanResults = 0;
        WiFi.scanDelete(); // Clean up any pending scan
        return true; // Consider timed out scan as "complete" with no results
    }
    
    // Check if scan completed
    int status = WiFi.scanComplete();
    if (status >= 0) {
        // Scan completed successfully
        scanResults = status;
        scanInProgress = false;
        return true;
    } else if (status == WIFI_SCAN_FAILED) {
        // Scan failed
        scanResults = 0;
        scanInProgress = false;
        return true; // Consider failed scan as "complete"
    }
    
    // Still scanning
    return false;
}

int WiFiManager::getScanProgress() {
    if (!scanInProgress) {
        return scanResults > 0 ? 100 : 0;
    }
    
    // Estimate progress based on time (typical scan takes 1-3 seconds)
    unsigned long elapsed = millis() - scanStartTime;
    int progress = min(95, (int)(elapsed * 100 / 3000)); // Max 95% until actually complete
    return progress;
}

bool WiFiManager::isScanInProgress() {
    return scanInProgress;
}

int WiFiManager::getScanResults() {
    return scanResults;
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
        statusMessage = "AP: " + String(currentConfig.ap_ssid);
    } else {
        statusMessage = "AP Start Failed";
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
    connectionState = WIFI_STATE_CONNECTING;
    statusMessage = "Connecting to " + String(currentConfig.ssid);
    lastReconnectAttempt = millis();
    isConnecting = true;
    
    return true;  // Return true if we started the connection attempt
}

void WiFiManager::checkConnection() {
    // Exponential backoff reconnection logic
    if ((!staConnected || WiFi.status() != WL_CONNECTED) && strlen(currentConfig.ssid) > 0 && reconnectAttempts < RECONNECT_MAX_ATTEMPTS && !isConnecting && lastDisconnectReason != 15) {
        if (millis() - lastReconnectAttempt >= reconnectDelay) {
            DEBUG_PRINT("WiFi reconnect attempt ");
            DEBUG_PRINT(reconnectAttempts + 1);
            DEBUG_PRINT(", delay: ");
            DEBUG_PRINTLN(reconnectDelay);
            
            staConnected = false;
            connectionState = WIFI_STATE_CONNECTING;
            statusMessage = "Reconnecting...";
            startSTA();
            
            // Increase delay for next attempt (exponential backoff)
            reconnectAttempts++;
            reconnectDelay = min(reconnectDelay * 2, RECONNECT_MAX_INTERVAL);
        }
    } else if (reconnectAttempts >= RECONNECT_MAX_ATTEMPTS) {
        // Give up after max attempts
        if (lastDisconnectReason == 15) {
            statusMessage = "Key exchange failed - check WiFi network/AP settings";
        } else if (!fallbackToAPDone && currentConfig.mode == TNC_WIFI_STA) {
            // Fall back to AP mode so user can reconfigure
            DEBUG_PRINTLN("STA connection failed, falling back to AP mode");
            fallbackToAPDone = true;
            currentConfig.mode = TNC_WIFI_AP;
            saveConfig(currentConfig);
            applyConfig(currentConfig);
            statusMessage = "Switched to AP mode - connect to configure WiFi";
            connectionState = WIFI_STATE_DISCONNECTED;
        } else {
            statusMessage = "Connection failed - check WiFi settings";
            connectionState = WIFI_STATE_FAILED;
        }
    } else {
        if (!staConnected) {
            DEBUG_PRINTLN("WiFi connected!");
            // Reset reconnection parameters on successful connection
            reconnectAttempts = 0;
            reconnectDelay = RECONNECT_BASE_INTERVAL;
            fallbackToAPDone = false;  // Reset fallback flag on successful connection
        }
        staConnected = true;
        connectionState = WIFI_STATE_CONNECTED;
    }
}

WiFiConnectionState WiFiManager::getConnectionState() {
    return connectionState;
}

bool WiFiManager::isReady() {
    return (apStarted || staConnected);
}

String WiFiManager::getStatusMessage() {
    return statusMessage;
}

void WiFiManager::setupWiFiEvents() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch(event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                DEBUG_PRINTLN("WiFi STA started");
                statusMessage = "Connecting...";
                connectionState = WIFI_STATE_CONNECTING;
                break;
                
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                DEBUG_PRINTLN("WiFi STA associated");
                statusMessage = "Associated";
                connectionState = WIFI_STATE_CONNECTED;
                isConnecting = false;
                break;
                
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                DEBUG_PRINT("WiFi got IP: ");
                DEBUG_PRINTLN(WiFi.localIP());
                statusMessage = "Connected";
                staConnected = true;
                connectionState = WIFI_STATE_CONNECTED;
                isConnecting = false;
                // Setup mDNS after getting IP
                setupMDNS();
                break;
                
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                DEBUG_PRINT("WiFi STA disconnected, reason: ");
                DEBUG_PRINTLN(info.wifi_sta_disconnected.reason);
                lastDisconnectReason = info.wifi_sta_disconnected.reason;
                if (info.wifi_sta_disconnected.reason == 15) {
                    // Group key update timeout - likely persistent issue, give up quickly
                    reconnectAttempts = RECONNECT_MAX_ATTEMPTS;
                }
                statusMessage = "Disconnected";
                staConnected = false;
                isConnecting = false;
                if (connectionState == WIFI_STATE_CONNECTING) {
                    connectionState = WIFI_STATE_FAILED;
                } else {
                    connectionState = WIFI_STATE_DISCONNECTED;
                }
                break;
                
            case ARDUINO_EVENT_WIFI_AP_START:
                DEBUG_PRINTLN("WiFi AP started");
                statusMessage = "AP Started";
                apStarted = true;
                // Start captive portal
                startCaptivePortal();
                // Setup mDNS for AP
                setupMDNS();
                break;
                
            case ARDUINO_EVENT_WIFI_AP_STOP:
                DEBUG_PRINTLN("WiFi AP stopped");
                apStarted = false;
                stopCaptivePortal();
                break;
                
            case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
                DEBUG_PRINTLN("Client connected to AP");
                break;
                
            case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
                DEBUG_PRINTLN("Client disconnected from AP");
                break;
                
            default:
                break;
        }
    });
}

void WiFiManager::startCaptivePortal() {
    if (dnsServer) {
        return;  // Already running
    }
    
    IPAddress apIP = WiFi.softAPIP();
    if (apIP == IPAddress(0, 0, 0, 0)) {
        DEBUG_PRINTLN("AP IP not ready, skipping captive portal start");
        return;
    }
    
    dnsServer = new DNSServer();
    if (dnsServer) {
        // Redirect all DNS requests to our AP IP
        dnsServer->start(53, "*", apIP);
        DEBUG_PRINTLN("Captive portal DNS started");
    }
}

void WiFiManager::stopCaptivePortal() {
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
        DEBUG_PRINTLN("Captive portal DNS stopped");
    }
}

bool WiFiManager::setupMDNS() {
    // Only initialize mDNS once
    if (mdnsStarted) {
        DEBUG_PRINTLN("mDNS already started");
        return true;
    }
    
    if (!MDNS.begin("loratncx")) {
        DEBUG_PRINTLN("Error setting up mDNS");
        return false;
    }
    
    DEBUG_PRINTLN("mDNS started: loratncx.local");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("kiss", "tcp", currentConfig.tcp_kiss_port);
    
    mdnsStarted = true;
    return true;
}

void WiFiManager::generateSecurePassword(char* buffer, size_t bufferSize) {
    // Character set for secure password: uppercase, lowercase, numbers, and some special chars
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    const size_t charsetSize = sizeof(charset) - 1; // Exclude null terminator
    
    // Generate 12-character password (secure minimum for WPA2)
    const size_t passwordLength = 12;
    
    if (bufferSize < passwordLength + 1) {
        // Buffer too small, use fallback
        strncpy(buffer, "SecurePass123!", bufferSize);
        return;
    }
    
    // Seed ESP32's random number generator with current time and some entropy
    randomSeed(micros() ^ ESP.getCycleCount() ^ analogRead(0));
    
    for (size_t i = 0; i < passwordLength; ++i) {
        uint32_t randomValue = esp_random(); // Use ESP32's hardware RNG
        buffer[i] = charset[randomValue % charsetSize];
    }
    
    buffer[passwordLength] = '\0';
}
