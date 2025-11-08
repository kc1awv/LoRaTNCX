#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

// WiFi modes (using TNC prefix to avoid conflicts with ESP32 WiFi library)
enum TNCWiFiMode {
    TNC_WIFI_OFF = 0,
    TNC_WIFI_AP = 1,      // Access Point mode
    TNC_WIFI_STA = 2,     // Station mode (client)
    TNC_WIFI_AP_STA = 3   // Both AP and STA
};

// WiFi configuration structure
struct WiFiConfig {
    char ssid[32];
    char password[64];
    char ap_ssid[32];
    char ap_password[64];
    uint8_t mode;          // WiFiMode enum value
    bool dhcp;             // Use DHCP in STA mode
    uint8_t ip[4];         // Static IP (if DHCP disabled)
    uint8_t gateway[4];    // Gateway IP
    uint8_t subnet[4];     // Subnet mask
    uint8_t dns[4];        // DNS server
    bool tcp_kiss_enabled; // Enable TCP KISS server
    uint16_t tcp_kiss_port;// TCP KISS server port
    uint32_t magic;        // Magic number for validation
};

class WiFiManager {
public:
    WiFiManager();
    
    // Initialize WiFi manager
    bool begin();
    
    // Start WiFi with saved/default configuration
    bool start();
    
    // Stop WiFi
    void stop();
    
    // Update WiFi - call in loop()
    void update();
    
    // Get current WiFi status
    bool isConnected();
    bool isAPActive();
    String getIPAddress();
    String getAPIPAddress();
    int getRSSI();
    
    // Configuration management
    bool saveConfig(const WiFiConfig& config);
    bool loadConfig(WiFiConfig& config);
    bool hasValidConfig();
    void resetToDefaults(WiFiConfig& config);
    bool clearConfig();
    
    // Apply configuration
    bool applyConfig(const WiFiConfig& config);
    
    // Get current configuration
    void getCurrentConfig(WiFiConfig& config);
    
    // Scan for available networks
    int scanNetworks();
    String getScannedSSID(int index);
    int getScannedRSSI(int index);
    bool getScannedEncryption(int index);
    
private:
    Preferences preferences;
    WiFiConfig currentConfig;
    bool initialized;
    bool apStarted;
    bool staConnected;
    unsigned long lastReconnectAttempt;
    int scanResults;
    
    static const uint32_t CONFIG_MAGIC = 0xFEEDBEEF;
    static const char* NVS_NAMESPACE;
    static const char* NVS_WIFI_KEY;
    static const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds
    
    // Internal methods
    bool startAP();
    bool startSTA();
    void checkConnection();
};

#endif // WIFI_MANAGER_H
