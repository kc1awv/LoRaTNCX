/**
 * @file SimpleWiFiManager.cpp
 * @brief Simple, robust WiFi management implementation
 */

#include "SimpleWiFiManager.h"
#include "SystemLogger.h"
#include <esp_system.h>

SimpleWiFiManager::SimpleWiFiManager() 
    : currentState(State::INIT)
    , currentNetworkIndex(0)
    , stateStartTime(0)
    , lastReconnectCheck(0)
    , lastReadyState(false)
{
    generateAPCredentials();
}

bool SimpleWiFiManager::begin()
{
    LOG_WIFI_INFO("SimpleWiFiManager: Starting...");
    
    // Clean WiFi initialization
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    loadNetworks();
    setState(State::INIT);
    
    return true;
}

void SimpleWiFiManager::update()
{
    switch (currentState) {
        case State::INIT:
            handleInit();
            break;
        case State::STA_CONNECTING:
            handleSTAConnecting();
            break;
        case State::STA_CONNECTED:
            handleSTAConnected();
            break;
        case State::AP_STARTING:
            handleAPStarting();
            break;
        case State::AP_READY:
            handleAPReady();
            break;
        case State::ERROR:
            handleError();
            break;
    }
    
    notifyStateChange();
}

SimpleWiFiManager::Status SimpleWiFiManager::getStatus() const
{
    Status status;
    status.state = currentState;
    status.apSSID = apSSID;
    status.apPassword = apPassword;
    status.isReady = (currentState == State::STA_CONNECTED || currentState == State::AP_READY);
    
    if (currentState == State::STA_CONNECTED) {
        status.currentSSID = WiFi.SSID();
        status.currentIP = WiFi.localIP();
    }
    
    if (currentState == State::AP_READY) {
        status.apIP = WiFi.softAPIP();
    }
    
    return status;
}

bool SimpleWiFiManager::addNetwork(const String& ssid, const String& password)
{
    if (ssid.isEmpty() || password.length() < 8) {
        return false;
    }
    
    // Remove existing entry if it exists
    removeNetwork(ssid);
    
    // Check if we have space
    if (networks.size() >= MAX_NETWORKS) {
        return false;
    }
    
    Network network;
    network.ssid = ssid;
    network.password = password;
    networks.push_back(network);
    
    saveNetworks();
    
    Serial.printf("SimpleWiFiManager: Added network '%s'\n", ssid.c_str());
    return true;
}

bool SimpleWiFiManager::removeNetwork(const String& ssid)
{
    auto it = std::find_if(networks.begin(), networks.end(), 
        [&ssid](const Network& n) { return n.ssid == ssid; });
    
    if (it != networks.end()) {
        networks.erase(it);
        saveNetworks();
        Serial.printf("SimpleWiFiManager: Removed network '%s'\n", ssid.c_str());
        return true;
    }
    
    return false;
}

void SimpleWiFiManager::clearAllNetworks()
{
    networks.clear();
    saveNetworks();
    Serial.println("SimpleWiFiManager: Cleared all networks");
}

std::vector<SimpleWiFiManager::Network> SimpleWiFiManager::getStoredNetworks() const
{
    return networks;
}

void SimpleWiFiManager::onStateChange(std::function<void(bool)> callback)
{
    stateCallback = callback;
}

void SimpleWiFiManager::forceAP()
{
    Serial.println("SimpleWiFiManager: Forcing AP mode");
    setState(State::AP_STARTING);
}

void SimpleWiFiManager::forceReconnect()
{
    Serial.println("SimpleWiFiManager: Forcing reconnection");
    if (!networks.empty()) {
        setState(State::STA_CONNECTING);
    } else {
        setState(State::AP_STARTING);
    }
}

void SimpleWiFiManager::loadNetworks()
{
    networks.clear();
    
    Preferences prefs;
    if (!prefs.begin(PREFS_NAMESPACE, true)) {
        Serial.println("SimpleWiFiManager: Failed to open preferences for reading");
        return;
    }
    
    uint8_t count = prefs.getUChar("count", 0);
    LOG_WIFI_INFO("SimpleWiFiManager: Loading " + String(count) + " networks");
    
    for (uint8_t i = 0; i < count && i < MAX_NETWORKS; i++) {
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        
        String ssid = prefs.getString(ssidKey.c_str(), "");
        String password = prefs.getString(passKey.c_str(), "");
        
        if (!ssid.isEmpty() && !password.isEmpty()) {
            Network network;
            network.ssid = ssid;
            network.password = password;
            networks.push_back(network);
            LOG_WIFI_INFO("SimpleWiFiManager: Loaded network '" + ssid + "'");
        }
    }
    
    prefs.end();
}

void SimpleWiFiManager::saveNetworks()
{
    Preferences prefs;
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("SimpleWiFiManager: Failed to open preferences for writing");
        return;
    }
    
    prefs.clear();
    prefs.putUChar("count", networks.size());
    
    for (size_t i = 0; i < networks.size(); i++) {
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        prefs.putString(ssidKey.c_str(), networks[i].ssid);
        prefs.putString(passKey.c_str(), networks[i].password);
    }
    
    prefs.end();
    Serial.printf("SimpleWiFiManager: Saved %d networks\n", networks.size());
}

void SimpleWiFiManager::generateAPCredentials()
{
    apSSID = "LoRaTNCX-" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 4);
    
    // Generate random password
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    apPassword = "";
    for (int i = 0; i < 8; i++) {
        apPassword += charset[esp_random() % (sizeof(charset) - 1)];
    }
    
    Serial.printf("SimpleWiFiManager: Generated AP credentials - SSID: %s, Password: %s\n", 
                  apSSID.c_str(), apPassword.c_str());
}

void SimpleWiFiManager::setState(State newState)
{
    if (currentState != newState) {
        Serial.printf("SimpleWiFiManager: State %d -> %d\n", (int)currentState, (int)newState);
        currentState = newState;
        stateStartTime = millis();
    }
}

void SimpleWiFiManager::handleInit()
{
    if (!networks.empty()) {
        Serial.printf("SimpleWiFiManager: Found %d saved networks, trying to connect\n", networks.size());
        setState(State::STA_CONNECTING);
    } else {
        Serial.println("SimpleWiFiManager: No saved networks, starting AP mode");
        setState(State::AP_STARTING);
    }
}

void SimpleWiFiManager::handleSTAConnecting()
{
    static bool connectionStarted = false;
    
    if (!connectionStarted) {
        startSTAConnection();
        connectionStarted = true;
        return;
    }
    
    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("SimpleWiFiManager: Connected to '%s' (IP: %s)\n", 
                     WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        connectionStarted = false;
        setState(State::STA_CONNECTED);
        return;
    }
    
    // Check timeout
    if (millis() - stateStartTime > STA_TIMEOUT_MS) {
        Serial.printf("SimpleWiFiManager: Connection timeout for network %d\n", currentNetworkIndex);
        connectionStarted = false;
        tryNextNetwork();
    }
}

void SimpleWiFiManager::handleSTAConnected()
{
    // Check if we're still connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("SimpleWiFiManager: Lost STA connection");
        lastReconnectCheck = millis();
        setState(State::STA_CONNECTING);
        return;
    }
    
    // Periodic reconnection attempts if needed
    if (millis() - lastReconnectCheck > RECONNECT_INTERVAL_MS) {
        lastReconnectCheck = millis();
        // Connection is good, just update timestamp
    }
}

void SimpleWiFiManager::handleAPStarting()
{
    startAPMode();
}

void SimpleWiFiManager::handleAPReady()
{
    // Check if AP is still active
    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
        Serial.println("SimpleWiFiManager: AP mode lost, restarting");
        setState(State::AP_STARTING);
        return;
    }
    
    // Periodically check if we should try STA again
    if (!networks.empty() && millis() - lastReconnectCheck > RECONNECT_INTERVAL_MS) {
        lastReconnectCheck = millis();
        Serial.println("SimpleWiFiManager: Trying STA connection from AP mode");
        setState(State::STA_CONNECTING);
    }
}

void SimpleWiFiManager::handleError()
{
    // Try to recover after a delay
    if (millis() - stateStartTime > 5000) {
        Serial.println("SimpleWiFiManager: Recovering from error");
        setState(State::INIT);
    }
}

void SimpleWiFiManager::startSTAConnection()
{
    if (networks.empty()) {
        setState(State::AP_STARTING);
        return;
    }
    
    if (currentNetworkIndex >= networks.size()) {
        currentNetworkIndex = 0;
    }
    
    const Network& network = networks[currentNetworkIndex];
    Serial.printf("SimpleWiFiManager: Connecting to '%s'\n", network.ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(network.ssid.c_str(), network.password.c_str());
}

void SimpleWiFiManager::startAPMode()
{
    Serial.println("SimpleWiFiManager: Starting AP mode");
    
    WiFi.mode(WIFI_AP);
    
    if (WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
        Serial.printf("SimpleWiFiManager: AP started - SSID: %s, IP: %s\n", 
                     apSSID.c_str(), WiFi.softAPIP().toString().c_str());
        setState(State::AP_READY);
    } else {
        Serial.println("SimpleWiFiManager: Failed to start AP");
        setState(State::ERROR);
    }
}

void SimpleWiFiManager::tryNextNetwork()
{
    currentNetworkIndex++;
    
    if (currentNetworkIndex >= networks.size()) {
        Serial.println("SimpleWiFiManager: All networks failed, starting AP mode");
        currentNetworkIndex = 0;
        setState(State::AP_STARTING);
    } else {
        Serial.printf("SimpleWiFiManager: Trying next network (%d/%d)\n", 
                     currentNetworkIndex + 1, (int)networks.size());
        setState(State::STA_CONNECTING);
    }
}

void SimpleWiFiManager::notifyStateChange()
{
    bool currentReady = (currentState == State::STA_CONNECTED || currentState == State::AP_READY);
    
    if (currentReady != lastReadyState) {
        lastReadyState = currentReady;
        if (stateCallback) {
            stateCallback(currentReady);
        }
    }
}