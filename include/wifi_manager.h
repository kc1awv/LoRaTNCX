#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

/**
 * @brief WiFi connection states for status tracking
 */
enum WiFiConnectionState {
    WIFI_STATE_DISCONNECTED,  ///< Not connected to any network
    WIFI_STATE_CONNECTING,    ///< Attempting to connect to a network
    WIFI_STATE_CONNECTED,     ///< Successfully connected to a network
    WIFI_STATE_FAILED         ///< Connection attempt failed
};

/**
 * @brief WiFi operating modes (prefixed to avoid conflicts with ESP32 WiFi library)
 */
enum TNCWiFiMode {
    TNC_WIFI_OFF = 0,     ///< WiFi completely disabled
    TNC_WIFI_AP = 1,      ///< Access Point mode only
    TNC_WIFI_STA = 2,     ///< Station/Client mode only
    TNC_WIFI_AP_STA = 3   ///< Both AP and Station modes simultaneously
};

/**
 * @brief WiFi configuration structure for persistent storage
 */
struct WiFiConfig {
    char ssid[32];           ///< Station mode SSID
    char password[64];       ///< Station mode password
    char ap_ssid[32];        ///< Access Point SSID
    char ap_password[64];    ///< Access Point password
    uint8_t mode;            ///< WiFiMode enum value
    bool dhcp;               ///< Use DHCP in station mode
    uint8_t ip[4];           ///< Static IP address (if DHCP disabled)
    uint8_t gateway[4];      ///< Gateway IP address
    uint8_t subnet[4];       ///< Subnet mask
    uint8_t dns[4];          ///< DNS server IP address
    bool tcp_kiss_enabled;   ///< Enable TCP KISS server
    uint16_t tcp_kiss_port;  ///< TCP KISS server port number
    uint32_t magic;          ///< Magic number for configuration validation
};

/**
 * @brief WiFi Manager for ESP32 WiFi functionality
 *
 * Provides comprehensive WiFi management including:
 * - Access Point and Station modes
 * - Configuration persistence in NVS
 * - Automatic reconnection with exponential backoff
 * - TCP KISS server for network clients
 * - mDNS service discovery
 * - Captive portal for configuration
 *
 * Supports simultaneous AP+STA operation for maximum flexibility.
 */
class WiFiManager {
public:
    /**
     * @brief Construct a new WiFi Manager
     */
    WiFiManager();
    
    /**
     * @brief Initialize the WiFi manager
     *
     * Sets up preferences storage and prepares for WiFi operations.
     * Must be called before using other WiFi functions.
     *
     * @return true if initialization successful
     * @return false if initialization failed
     */
    bool begin();
    
    /**
     * @brief Start WiFi with saved or default configuration
     *
     * Loads the saved WiFi configuration and starts WiFi in the configured mode.
     * If no valid configuration exists, starts with default AP mode.
     *
     * @return true if WiFi started successfully
     * @return false if startup failed
     */
    bool start();
    
    /**
     * @brief Stop all WiFi operations
     *
     * Disables both AP and STA modes, stops TCP server, and cleans up resources.
     */
    void stop();
    
    /**
     * @brief Update WiFi manager state
     *
     * Main update function that should be called regularly in the main loop.
     * Handles connection monitoring, reconnection attempts, and status updates.
     */
    void update();
    
    /**
     * @brief Check if connected to an external WiFi network (STA mode)
     *
     * @return true if connected as station to an external network
     * @return false if not connected or in AP-only mode
     */
    bool isConnected();
    
    /**
     * @brief Check if access point is active
     *
     * @return true if AP mode is active and clients can connect
     * @return false if AP mode is disabled
     */
    bool isAPActive();
    
    /**
     * @brief Get the station mode IP address
     *
     * @return IP address as string (empty if not connected)
     */
    String getIPAddress();
    
    /**
     * @brief Get the access point IP address
     *
     * @return AP IP address as string (empty if AP not active)
     */
    String getAPIPAddress();
    
    /**
     * @brief Get the current WiFi signal strength
     *
     * @return RSSI in dBm (negative values, e.g., -50 for strong signal)
     */
    int getRSSI();
    
    /**
     * @brief Get the current connection state
     *
     * @return Current WiFiConnectionState enum value
     */
    WiFiConnectionState getConnectionState();
    
    /**
     * @brief Check if WiFi is ready for client connections
     *
     * WiFi is considered ready if either connected to external network
     * or access point is active.
     *
     * @return true if WiFi is operational
     * @return false if WiFi is disabled or failed
     */
    bool isReady();
    
    /**
     * @brief Get a human-readable status message
     *
     * @return Status string describing current WiFi state
     */
    String getStatusMessage();
    
    /**
     * @brief Save WiFi configuration to persistent storage
     *
     * Stores the provided configuration in NVS (Non-Volatile Storage)
     * for persistence across reboots.
     *
     * @param config Reference to WiFiConfig structure to save
     * @return true if configuration saved successfully
     * @return false if save operation failed
     */
    bool saveConfig(const WiFiConfig& config);
    
    /**
     * @brief Load WiFi configuration from persistent storage
     *
     * Retrieves the saved configuration from NVS. If no valid configuration
     * exists, the config structure is populated with defaults.
     *
     * @param config Reference to WiFiConfig structure to fill
     * @return true if valid configuration loaded
     * @return false if no valid configuration found (defaults used)
     */
    bool loadConfig(WiFiConfig& config);
    
    /**
     * @brief Check if a valid WiFi configuration exists in storage
     *
     * @return true if valid configuration is stored
     * @return false if no configuration or invalid data
     */
    bool hasValidConfig();
    
    /**
     * @brief Reset WiFi configuration to factory defaults
     *
     * Populates the config structure with safe default values.
     *
     * @param config Reference to WiFiConfig structure to reset
     */
    void resetToDefaults(WiFiConfig& config);
    
    /**
     * @brief Clear saved WiFi configuration from storage
     *
     * Removes the configuration from NVS. Next boot will use defaults.
     *
     * @return true if configuration cleared successfully
     * @return false if clear operation failed
     */
    bool clearConfig();
    
    /**
     * @brief Apply WiFi configuration and restart WiFi services
     *
     * Applies the provided configuration and restarts WiFi with new settings.
     * This may disconnect existing connections.
     *
     * @param config Reference to WiFiConfig structure to apply
     * @return true if configuration applied successfully
     * @return false if configuration invalid or application failed
     */
    bool applyConfig(const WiFiConfig& config);
    
    /**
     * @brief Get the current active WiFi configuration
     *
     * Returns the currently active configuration, which may differ from
     * the saved configuration if changes are pending.
     *
     * @param config Reference to WiFiConfig structure to fill
     */
    void getCurrentConfig(WiFiConfig& config);
    
    /**
     * @brief Scan for available WiFi networks
     *
     * Performs a WiFi network scan and caches results for retrieval.
     * This is an asynchronous operation - call multiple times if needed.
     *
     * @return Number of networks found (0 if scan failed or in progress)
     */
    int scanNetworks();
    
    /**
     * @brief Get SSID of a scanned network
     *
     * @param index Network index (0 to scanNetworks()-1)
     * @return Network SSID as string (empty if invalid index)
     */
    String getScannedSSID(int index);
    
    /**
     * @brief Get RSSI of a scanned network
     *
     * @param index Network index (0 to scanNetworks()-1)
     * @return Signal strength in dBm (0 if invalid index)
     */
    int getScannedRSSI(int index);
    
    /**
     * @brief Check if a scanned network uses encryption
     *
     * @param index Network index (0 to scanNetworks()-1)
     * @return true if network is encrypted
     * @return false if open network or invalid index
     */
    bool getScannedEncryption(int index);
    
private:
    Preferences preferences;
    WiFiConfig currentConfig;
    DNSServer* dnsServer;
    bool initialized;
    bool apStarted;
    bool staConnected;
    bool mdnsStarted;
    WiFiConnectionState connectionState;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectDelay;
    int reconnectAttempts;
    int scanResults;
    String statusMessage;
    
    static const uint32_t CONFIG_MAGIC = 0xFEEDBEEF;
    static const char* NVS_NAMESPACE;
    static const char* NVS_WIFI_KEY;
    static const unsigned long RECONNECT_BASE_INTERVAL = 5000;   // 5 seconds
    static const unsigned long RECONNECT_MAX_INTERVAL = 60000;   // 60 seconds
    static const unsigned long CONNECTION_TIMEOUT = 15000;       // 15 seconds
    
    // Internal methods
    bool startAP();
    bool startSTA();
    void checkConnection();
    void setupWiFiEvents();
    void startCaptivePortal();
    void stopCaptivePortal();
    bool setupMDNS();
};

#endif // WIFI_MANAGER_H
