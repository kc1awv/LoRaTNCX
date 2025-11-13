#include "config_manager.h"
#include "config.h"
#include "board_config.h"

const char* ConfigManager::NVS_NAMESPACE = "lora";
const char* ConfigManager::NVS_CONFIG_KEY = "config";
const char* ConfigManager::NVS_GNSS_KEY = "gnss";

ConfigManager::ConfigManager() {
}

bool ConfigManager::begin() {
    // NVS will be initialized on first use (save/load)
    // No need to keep preferences open during entire runtime
    return true;
}

bool ConfigManager::saveConfig(const LoRaConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Create a copy with magic number
    LoRaConfig configToSave = config;
    configToSave.magic = CONFIG_MAGIC;
    
    // Write configuration as a blob
    size_t written = preferences.putBytes(NVS_CONFIG_KEY, &configToSave, sizeof(LoRaConfig));
    preferences.end();
    
    if (written != sizeof(LoRaConfig)) {
        return false;
    }
    
    return true;
}

bool ConfigManager::loadConfig(LoRaConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    // Check if config key exists to avoid error logs on first boot
    if (!preferences.isKey(NVS_CONFIG_KEY)) {
        preferences.end();
        return false;
    }
    
    size_t len = preferences.getBytes(NVS_CONFIG_KEY, &config, sizeof(LoRaConfig));
    preferences.end();
    
    // Check if we read the correct size
    if (len != sizeof(LoRaConfig)) {
        return false;
    }
    
    // Validate magic number
    if (config.magic != CONFIG_MAGIC) {
        return false;
    }
    
    // Validate ranges
    if (config.frequency < 137.0 || config.frequency > 1020.0 ||
        config.bandwidth < 7.8 || config.bandwidth > 500.0 ||
        config.spreading < 6 || config.spreading > 12 ||
        config.codingRate < 5 || config.codingRate > 8 ||
        config.power < -9 || config.power > 22) {
        return false;
    }
    
    return true;
}

bool ConfigManager::hasValidConfig() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    // Check if config key exists to avoid error logs
    if (!preferences.isKey(NVS_CONFIG_KEY)) {
        preferences.end();
        return false;
    }
    
    LoRaConfig config;
    size_t read = preferences.getBytes(NVS_CONFIG_KEY, &config, sizeof(LoRaConfig));
    preferences.end();
    
    return (read == sizeof(LoRaConfig) && config.magic == CONFIG_MAGIC);
}

void ConfigManager::resetToDefaults(LoRaConfig& config) {
    config.frequency = LORA_FREQUENCY;
    config.bandwidth = LORA_BANDWIDTH;
    config.spreading = LORA_SPREADING;
    config.codingRate = LORA_CODINGRATE;
    config.power = LORA_POWER;
    config.syncWord = LORA_SYNCWORD;
    config.preamble = LORA_PREAMBLE;
    config.magic = CONFIG_MAGIC;
}

bool ConfigManager::clearConfig() {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    bool result = preferences.remove(NVS_CONFIG_KEY);
    preferences.end();
    
    return result;
}

// ===== GNSS Configuration Methods =====

bool ConfigManager::saveGNSSConfig(const GNSSConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    // Create a copy with magic number
    GNSSConfig configToSave = config;
    configToSave.magic = GNSS_MAGIC;
    
    // Write configuration as a blob
    size_t written = preferences.putBytes(NVS_GNSS_KEY, &configToSave, sizeof(GNSSConfig));
    preferences.end();
    
    if (written != sizeof(GNSSConfig)) {
        return false;
    }
    
    return true;
}

bool ConfigManager::loadGNSSConfig(GNSSConfig& config) {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    // Check if GNSS config key exists to avoid error logs on first boot
    if (!preferences.isKey(NVS_GNSS_KEY)) {
        preferences.end();
        return false;
    }
    
    size_t len = preferences.getBytes(NVS_GNSS_KEY, &config, sizeof(GNSSConfig));
    preferences.end();
    
    // Check if we read the correct size
    if (len != sizeof(GNSSConfig)) {
        return false;
    }
    
    // Validate magic number
    if (config.magic != GNSS_MAGIC) {
        return false;
    }
    
    // Validate baud rate (common GNSS rates)
    if (config.baudRate != 4800 && config.baudRate != 9600 && 
        config.baudRate != 19200 && config.baudRate != 38400 &&
        config.baudRate != 57600 && config.baudRate != 115200) {
        return false;
    }
    
    return true;
}

bool ConfigManager::hasValidGNSSConfig() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    
    // Check if GNSS config key exists to avoid error logs
    if (!preferences.isKey(NVS_GNSS_KEY)) {
        preferences.end();
        return false;
    }
    
    GNSSConfig config;
    size_t read = preferences.getBytes(NVS_GNSS_KEY, &config, sizeof(GNSSConfig));
    preferences.end();
    
    return (read == sizeof(GNSSConfig) && config.magic == GNSS_MAGIC);
}

void ConfigManager::resetGNSSToDefaults(GNSSConfig& config) {
    config.enabled = GNSS_ENABLED;
    config.serialPassthrough = false;  // Disabled by default
    config.baudRate = GNSS_BAUD_RATE;
    config.tcpPort = GNSS_TCP_PORT;
    
    // Pin configuration depends on board variant
#ifdef HAS_GNSS_PORT
    #if HAS_GNSS_PORT == 1
        // V4 board with built-in GNSS port
        config.pinRX = PIN_GNSS_RX;
        config.pinTX = PIN_GNSS_TX;
        config.pinCtrl = PIN_GNSS_CTRL;
        config.pinWake = PIN_GNSS_WAKE;
        config.pinPPS = PIN_GNSS_PPS;
        config.pinRST = PIN_GNSS_RST;
    #else
        // V3 or other board - no default pins
        config.pinRX = -1;
        config.pinTX = -1;
        config.pinCtrl = -1;
        config.pinWake = -1;
        config.pinPPS = -1;
        config.pinRST = -1;
        config.enabled = false;  // Disabled by default on V3
    #endif
#else
    // No GNSS port defined
    config.pinRX = -1;
    config.pinTX = -1;
    config.pinCtrl = -1;
    config.pinWake = -1;
    config.pinPPS = -1;
    config.pinRST = -1;
    config.enabled = false;
#endif
    
    config.magic = GNSS_MAGIC;
}

bool ConfigManager::clearGNSSConfig() {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    
    bool result = preferences.remove(NVS_GNSS_KEY);
    preferences.end();
    
    return result;
}
