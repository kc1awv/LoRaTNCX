#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include "AmateurRadioLoRaConfigs.h"
#include "LoRaRadio.h"

/**
 * @file ConfigurationManager.h
 * @brief Manages LoRa configuration selection and switching for amateur radio use
 * 
 * Provides runtime configuration management for optimized amateur radio LoRa settings.
 * Allows switching between different configurations via KISS commands or serial interface.
 */

enum class LoRaConfigPreset {
    HIGH_SPEED = 0,
    FAST_BALANCED = 1,
    BALANCED = 2,
    ROBUST_BALANCED = 3,
    LONG_RANGE = 4,
    MAX_RANGE = 5,
    BAND_70CM = 6,
    BAND_33CM = 7,
    BAND_23CM = 8,
    CUSTOM = 99
};

struct LoRaConfiguration {
    String name;
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint16_t maxPayload;
    String expectedRange;
    String expectedThroughput;
    String useCase;
};

class ConfigurationManager {
private:
    LoRaRadio* radio;
    LoRaConfigPreset currentPreset;
    LoRaConfiguration currentConfig;
    LoRaConfiguration customConfig;
    
    static const LoRaConfiguration presetConfigurations[];
    
public:
    ConfigurationManager(LoRaRadio* radioInstance);
    
    // Configuration management
    bool setConfiguration(LoRaConfigPreset preset);
    bool setCustomConfiguration(float frequency, float bandwidth, uint8_t sf, uint8_t cr);
    LoRaConfigPreset getCurrentPreset() const { return currentPreset; }
    LoRaConfiguration getCurrentConfiguration() const { return currentConfig; }
    
    // Configuration queries
    String getConfigurationName() const;
    String getConfigurationInfo() const;
    uint16_t getMaxPayloadSize() const { return currentConfig.maxPayload; }
    
    // Preset utilities
    static String getPresetName(LoRaConfigPreset preset);
    static LoRaConfiguration getPresetConfiguration(LoRaConfigPreset preset);
    static void listAllPresets();
    
    // Command interface
    bool processConfigCommand(const String& command);
    String getConfigStatus() const;
    
    // Validation
    static bool validateConfiguration(float frequency, float bandwidth, uint8_t sf, uint8_t cr);
    static uint16_t calculateMaxPayload(uint8_t sf, uint8_t cr, float bandwidth);
};



#endif // CONFIGURATION_MANAGER_H