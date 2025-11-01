/**
 * @file SimpleWiFiManager.h
 * @brief Simple, robust WiFi management for LoRaTNCX
 * 
 * This is a complete rewrite of the WiFi subsystem with a focus on simplicity and reliability.
 * No dual-mode complexity - just clean STA first, then AP fallback.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <functional>

/**
 * @brief Simple WiFi manager with clear state transitions
 * 
 * States:
 * - INIT: Starting up
 * - STA_CONNECTING: Trying to connect to saved networks
 * - STA_CONNECTED: Connected to a station network
 * - AP_STARTING: Starting access point
 * - AP_READY: Access point is active
 * - ERROR: Something went wrong
 */
class SimpleWiFiManager
{
public:
    enum class State {
        INIT,
        STA_CONNECTING,
        STA_CONNECTED,
        AP_STARTING,
        AP_READY,
        ERROR
    };

    struct Network {
        String ssid;
        String password;
    };

    struct Status {
        State state;
        String currentSSID;
        IPAddress currentIP;
        String apSSID;
        String apPassword;
        IPAddress apIP;
        bool isReady;  // true when either STA connected or AP ready
    };

    SimpleWiFiManager();
    ~SimpleWiFiManager() = default;

    // Main interface
    bool begin();
    void update();
    Status getStatus() const;
    
    // Network management
    bool addNetwork(const String& ssid, const String& password);
    bool removeNetwork(const String& ssid);
    void clearAllNetworks();
    std::vector<Network> getStoredNetworks() const;
    
    // Callbacks
    void onStateChange(std::function<void(bool)> callback);
    
    // Force mode changes
    void forceAP();
    void forceReconnect();

private:
    static constexpr const char* PREFS_NAMESPACE = "simplewifi";
    static constexpr uint32_t STA_TIMEOUT_MS = 15000;
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 30000;
    static constexpr uint8_t MAX_NETWORKS = 5;
    
    State currentState;
    std::vector<Network> networks;
    uint8_t currentNetworkIndex;
    unsigned long stateStartTime;
    unsigned long lastReconnectCheck;
    
    String apSSID;
    String apPassword;
    
    std::function<void(bool)> stateCallback;
    bool lastReadyState;
    
    void loadNetworks();
    void saveNetworks();
    void generateAPCredentials();
    
    void setState(State newState);
    void handleInit();
    void handleSTAConnecting();
    void handleSTAConnected();
    void handleAPStarting();
    void handleAPReady();
    void handleError();
    
    void startSTAConnection();
    void startAPMode();
    void tryNextNetwork();
    
    void notifyStateChange();
};