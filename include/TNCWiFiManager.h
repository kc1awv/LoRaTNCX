/**
 * @file TNCWiFiManager.h
 * @brief WiFi management for LoRaTNCX, providing STA with AP fallback
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

/**
 * @brief WiFi manager that prefers station mode with automatic AP fallback.
 */
class TNCWiFiManager
{
public:
    TNCWiFiManager();

    /**
     * @brief Initialize WiFi subsystem, attempting STA first then AP fallback.
     * @return true on success.
     */
    bool begin();

    /**
     * @brief Update connection state machine. Should be called frequently.
     */
    void update();

    /**
     * @brief Add or update a station network credential.
     * @param ssid Network SSID
     * @param password Network password (8+ chars)
     * @param message Informational message on result
     * @return true if credentials were stored
     */
    bool addNetwork(const String &ssid, const String &password, String &message);

    /**
     * @brief Remove a stored station network credential.
     * @param ssid Network SSID to remove
     * @param message Informational message on result
     * @return true if removed
     */
    bool removeNetwork(const String &ssid, String &message);

    /**
     * @brief Summary of configured networks for presentation.
     */
    String getNetworksSummary() const;

    /**
     * @brief Summary of current WiFi operating state.
     */
    String getStatusSummary() const;

    struct StatusInfo
    {
        bool apActive = false;
        bool stationActive = false;
        bool stationConnected = false;
        bool stationAttemptActive = false;
        String apSSID;
        IPAddress apIP;
        String stationSSID;
        IPAddress stationIP;
    };

    /**
     * @brief Detailed status for UI presentation.
     */
    StatusInfo getStatusInfo() const;

    /**
     * @brief Get the current AP password for display purposes.
     * @return The current AP password, or empty string if not in AP mode.
     */
    String getAPPassword() const;

private:
    static constexpr const char *PREF_NAMESPACE = "wifi";
    static constexpr uint8_t MAX_NETWORKS = 8;
    static constexpr unsigned long STA_CONNECT_TIMEOUT_MS = 15000UL;
    static constexpr unsigned long RECONNECT_INTERVAL_MS = 30000UL;

    struct NetworkEntry
    {
        String ssid;
        String password;
    };

    NetworkEntry networks[MAX_NETWORKS];
    uint8_t networkCount;
    int8_t connectedNetworkIndex;

    bool apModeActive;
    bool stationAttemptActive;
    uint8_t stationAttemptIndex;
    uint8_t stationAttemptsTried;
    unsigned long stationAttemptStart;
    unsigned long lastReconnectCheck;

    String apSSID;
    String apPassword;

    void loadNetworksFromPreferences();
    void saveNetworksToPreferences();
    void configureDefaultAPCredentials();
    void generateRandomAPPassword();
    void startAccessPoint();
    void startStationAttempt(uint8_t index);
    void advanceStationAttempt();
    void ensureFallbackIfIdle();
};

