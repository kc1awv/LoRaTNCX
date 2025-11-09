#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Configuration structure for LoRa parameters
struct LoRaConfig {
    float frequency;        // MHz
    float bandwidth;        // kHz
    uint8_t spreading;      // SF 7-12
    uint8_t codingRate;     // 5-8 (for 4/5 to 4/8)
    int8_t power;           // dBm
    uint16_t syncWord;      // Sync word (2 bytes for SX126x)
    uint8_t preamble;       // Preamble length
    uint32_t magic;         // Magic number to verify valid config
};

// Configuration structure for GNSS parameters
struct GNSSConfig {
    bool enabled;           // Enable/disable GNSS
    bool serialPassthrough; // Forward NMEA to USB serial
    int8_t pinRX;           // RX pin (GNSS TX -> MCU RX)
    int8_t pinTX;           // TX pin (MCU TX -> GNSS RX)
    int8_t pinCtrl;         // Power control pin (optional)
    int8_t pinWake;         // Wake pin (optional)
    int8_t pinPPS;          // PPS pin (optional)
    int8_t pinRST;          // Reset pin (optional)
    uint32_t baudRate;      // GNSS baud rate
    uint16_t tcpPort;       // TCP port for NMEA streaming
    uint32_t magic;         // Magic number to verify valid config
};

class ConfigManager {
public:
    ConfigManager();
    
    // Initialize NVS and load configuration
    bool begin();
    
    // Save current configuration to NVS
    bool saveConfig(const LoRaConfig& config);
    
    // Load configuration from NVS
    bool loadConfig(LoRaConfig& config);
    
    // Check if valid configuration exists
    bool hasValidConfig();
    
    // Reset to default configuration
    void resetToDefaults(LoRaConfig& config);
    
    // Clear stored configuration
    bool clearConfig();
    
    // GNSS configuration methods
    bool saveGNSSConfig(const GNSSConfig& config);
    bool loadGNSSConfig(GNSSConfig& config);
    bool hasValidGNSSConfig();
    void resetGNSSToDefaults(GNSSConfig& config);
    bool clearGNSSConfig();
    
private:
    Preferences preferences;
    static const uint32_t CONFIG_MAGIC = 0xCAFEBABE;  // Magic number for validation
    static const uint32_t GNSS_MAGIC = 0xDEADBEEF;    // Magic number for GNSS config
    static const char* NVS_NAMESPACE;
    static const char* NVS_CONFIG_KEY;
    static const char* NVS_GNSS_KEY;
};

#endif // CONFIG_MANAGER_H
