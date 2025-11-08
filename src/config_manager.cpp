#include "config_manager.h"
#include "config.h"

const char* ConfigManager::NVS_NAMESPACE = "lora";
const char* ConfigManager::NVS_CONFIG_KEY = "config";

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
