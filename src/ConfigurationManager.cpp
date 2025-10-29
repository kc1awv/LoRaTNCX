#include "ConfigurationManager.h"
#include "Arduino.h"

// Static configuration definitions
const LoRaConfiguration ConfigurationManager::presetConfigurations[] = {
    // HIGH_SPEED
    {
        "High Speed (SF7, BW500kHz)",
        LORA_HIGH_SPEED_FREQUENCY,
        LORA_HIGH_SPEED_BW,
        LORA_HIGH_SPEED_SF,
        LORA_HIGH_SPEED_CR,
        LORA_HIGH_SPEED_MAX_PAYLOAD,
        "2-8 km",
        "20,420 bps",
        "Fast file transfer, streaming data"
    },
    // FAST_BALANCED
    {
        "Fast Balanced (SF8, BW500kHz)",
        LORA_FAST_BALANCED_FREQUENCY,
        LORA_FAST_BALANCED_BW,
        LORA_FAST_BALANCED_SF,
        LORA_FAST_BALANCED_CR,
        LORA_FAST_BALANCED_MAX_PAYLOAD,
        "5-15 km",
        "11,541 bps",
        "General purpose high-speed data"
    },
    // BALANCED
    {
        "Standard Balanced (SF8, BW250kHz)",
        LORA_BALANCED_FREQUENCY,
        LORA_BALANCED_BW,
        LORA_BALANCED_SF,
        LORA_BALANCED_CR,
        LORA_BALANCED_MAX_PAYLOAD,
        "8-20 km",
        "5,770 bps",
        "General purpose packet radio"
    },
    // ROBUST_BALANCED
    {
        "Robust Balanced (SF9, BW250kHz)",
        LORA_ROBUST_BALANCED_FREQUENCY,
        LORA_ROBUST_BALANCED_BW,
        LORA_ROBUST_BALANCED_SF,
        LORA_ROBUST_BALANCED_CR,
        LORA_ROBUST_BALANCED_MAX_PAYLOAD,
        "10-25 km",
        "3,263 bps",
        "Reliable data transfer in poor conditions"
    },
    // LONG_RANGE
    {
        "Long Range (SF10, BW125kHz)",
        LORA_LONG_RANGE_FREQUENCY,
        LORA_LONG_RANGE_BW,
        LORA_LONG_RANGE_SF,
        LORA_LONG_RANGE_CR,
        LORA_LONG_RANGE_MAX_PAYLOAD,
        "15-40 km",
        "804 bps",
        "Emergency communications, remote monitoring"
    },
    // MAX_RANGE
    {
        "Maximum Range (SF11, BW125kHz)",
        LORA_MAX_RANGE_FREQUENCY,
        LORA_MAX_RANGE_BW,
        LORA_MAX_RANGE_SF,
        LORA_MAX_RANGE_CR,
        LORA_MAX_RANGE_MAX_PAYLOAD,
        "20-60 km",
        "234 bps",
        "Emergency beacons, minimal data transfer"
    },
    // BAND_70CM
    {
        "70cm Band Optimized (432.6 MHz)",
        LORA_70CM_FREQUENCY,
        LORA_70CM_BW,
        LORA_70CM_SF,
        LORA_70CM_CR,
        LORA_70CM_MAX_PAYLOAD,
        "8-20 km",
        "5,770 bps",
        "Most popular amateur band, good propagation"
    },
    // BAND_33CM
    {
        "33cm Band Optimized (906 MHz)",
        LORA_33CM_FREQUENCY,
        LORA_33CM_BW,
        LORA_33CM_SF,
        LORA_33CM_CR,
        LORA_33CM_MAX_PAYLOAD,
        "2-8 km",
        "20,420 bps",
        "High bandwidth to avoid ISM interference"
    },
    // BAND_23CM
    {
        "23cm Band Optimized (1290 MHz)",
        LORA_23CM_FREQUENCY,
        LORA_23CM_BW,
        LORA_23CM_SF,
        LORA_23CM_CR,
        LORA_23CM_MAX_PAYLOAD,
        "1-5 km (line-of-sight)",
        "20,420 bps",
        "Microwave band, primarily line-of-sight"
    }
};

ConfigurationManager::ConfigurationManager(LoRaRadio* radioInstance) 
    : radio(radioInstance), currentPreset(LoRaConfigPreset::BALANCED) {
    // Initialize with balanced configuration
    currentConfig = presetConfigurations[static_cast<int>(LoRaConfigPreset::BALANCED)];
}

bool ConfigurationManager::setConfiguration(LoRaConfigPreset preset) {
    if (preset == LoRaConfigPreset::CUSTOM) {
        // Use custom configuration
        currentConfig = customConfig;
    } else {
        // Use preset configuration
        int presetIndex = static_cast<int>(preset);
        if (presetIndex < 0 || presetIndex >= 9) {  // 9 preset configurations
            Serial.println("ERROR: Invalid preset index");
            return false;
        }
        currentConfig = presetConfigurations[presetIndex];
    }
    
    // Apply configuration to radio
    Serial.print("Configuring radio: ");
    Serial.println(currentConfig.name);
    Serial.print("  Frequency: ");
    Serial.print(currentConfig.frequency, 1);
    Serial.println(" MHz");
    Serial.print("  Bandwidth: ");
    Serial.print(currentConfig.bandwidth, 1);
    Serial.println(" kHz");
    Serial.print("  Spreading Factor: SF");
    Serial.println(currentConfig.spreadingFactor);
    Serial.print("  Coding Rate: 4/");
    Serial.println(currentConfig.codingRate);
    Serial.print("  Max Payload: ");
    Serial.print(currentConfig.maxPayload);
    Serial.println(" bytes");
    
    bool result = radio->begin(
        currentConfig.frequency,
        currentConfig.bandwidth,
        currentConfig.spreadingFactor,
        currentConfig.codingRate
    );
    
    if (result) {
        currentPreset = preset;
        Serial.println("Configuration applied successfully");
        Serial.print("Expected range: ");
        Serial.println(currentConfig.expectedRange);
        Serial.print("Expected throughput: ");
        Serial.println(currentConfig.expectedThroughput);
        Serial.print("Use case: ");
        Serial.println(currentConfig.useCase);
    } else {
        Serial.println("ERROR: Failed to apply configuration");
    }
    
    return result;
}

bool ConfigurationManager::setCustomConfiguration(float frequency, float bandwidth, uint8_t sf, uint8_t cr) {
    if (!validateConfiguration(frequency, bandwidth, sf, cr)) {
        Serial.println("ERROR: Invalid custom configuration parameters");
        return false;
    }
    
    // Create custom configuration
    customConfig.name = "Custom Configuration";
    customConfig.frequency = frequency;
    customConfig.bandwidth = bandwidth;
    customConfig.spreadingFactor = sf;
    customConfig.codingRate = cr;
    customConfig.maxPayload = calculateMaxPayload(sf, cr, bandwidth);
    customConfig.expectedRange = "Custom";
    customConfig.expectedThroughput = "Custom";
    customConfig.useCase = "User-defined custom settings";
    
    // Apply custom configuration
    return setConfiguration(LoRaConfigPreset::CUSTOM);
}

String ConfigurationManager::getConfigurationName() const {
    return currentConfig.name;
}

String ConfigurationManager::getConfigurationInfo() const {
    String info = "Configuration: " + currentConfig.name + "\n";
    info += "Frequency: " + String(currentConfig.frequency, 1) + " MHz\n";
    info += "Bandwidth: " + String(currentConfig.bandwidth, 1) + " kHz\n";
    info += "Spreading Factor: SF" + String(currentConfig.spreadingFactor) + "\n";
    info += "Coding Rate: 4/" + String(currentConfig.codingRate) + "\n";
    info += "Max Payload: " + String(currentConfig.maxPayload) + " bytes\n";
    info += "Expected Range: " + currentConfig.expectedRange + "\n";
    info += "Expected Throughput: " + currentConfig.expectedThroughput + "\n";
    info += "Use Case: " + currentConfig.useCase;
    return info;
}

String ConfigurationManager::getPresetName(LoRaConfigPreset preset) {
    int presetIndex = static_cast<int>(preset);
    if (presetIndex >= 0 && presetIndex < 9) {
        return presetConfigurations[presetIndex].name;
    } else if (preset == LoRaConfigPreset::CUSTOM) {
        return "Custom Configuration";
    }
    return "Unknown";
}

LoRaConfiguration ConfigurationManager::getPresetConfiguration(LoRaConfigPreset preset) {
    int presetIndex = static_cast<int>(preset);
    if (presetIndex >= 0 && presetIndex < 9) {
        return presetConfigurations[presetIndex];
    }
    // Return default balanced configuration for invalid presets
    return presetConfigurations[static_cast<int>(LoRaConfigPreset::BALANCED)];
}

void ConfigurationManager::listAllPresets() {
    Serial.println("\nAvailable LoRa Configuration Presets:");
    Serial.println("=====================================");
    
    const char* presetNames[] = {
        "high_speed", "fast_balanced", "balanced", 
        "robust_balanced", "long_range", "max_range", 
        "band_70cm", "band_33cm", "band_23cm"
    };
    
    for (int i = 0; i < 9; i++) {
        LoRaConfiguration config = presetConfigurations[i];
        Serial.print(i);
        Serial.print(" (");
        Serial.print(presetNames[i]);
        Serial.print("): ");
        Serial.println(config.name);
        Serial.print("   Range: ");
        Serial.print(config.expectedRange);
        Serial.print(", Throughput: ");
        Serial.println(config.expectedThroughput);
        Serial.print("   Use: ");
        Serial.println(config.useCase);
        Serial.println();
    }
    
    Serial.println("Usage examples:");
    Serial.println("  SETCONFIG 3              - Set to balanced preset");
    Serial.println("  SETCONFIG balanced       - Set to balanced preset by name");
    Serial.println("  SETCONFIG 915.0 250 8 5 - Set custom config (freq, bw, sf, cr)");
}

bool ConfigurationManager::processConfigCommand(const String& command) {
    // Remove leading/trailing whitespace
    String cmd = command;
    cmd.trim();
    
    if (cmd.startsWith("LISTCONFIG") || cmd.startsWith("LIST")) {
        listAllPresets();
        return true;
    }
    
    if (cmd.startsWith("GETCONFIG") || cmd.startsWith("STATUS")) {
        Serial.println(getConfigStatus());
        return true;
    }
    
    if (cmd.startsWith("SETCONFIG ") || cmd.startsWith("CONFIG ")) {
        // Extract parameters
        String params = cmd.substring(cmd.indexOf(' ') + 1);
        params.trim();
        
        // Try to parse as preset number first
        if (params.length() == 1 && isdigit(params.charAt(0))) {
            int presetNum = params.toInt();
            if (presetNum >= 0 && presetNum < 9) {
                return setConfiguration(static_cast<LoRaConfigPreset>(presetNum));
            }
        }
        
        // Try to parse as preset name
        params.toLowerCase();
        if (params == "high_speed" || params == "fast") {
            return setConfiguration(LoRaConfigPreset::HIGH_SPEED);
        } else if (params == "fast_balanced") {
            return setConfiguration(LoRaConfigPreset::FAST_BALANCED);
        } else if (params == "balanced" || params == "default") {
            return setConfiguration(LoRaConfigPreset::BALANCED);
        } else if (params == "robust_balanced" || params == "robust") {
            return setConfiguration(LoRaConfigPreset::ROBUST_BALANCED);
        } else if (params == "long_range" || params == "longrange") {
            return setConfiguration(LoRaConfigPreset::LONG_RANGE);
        } else if (params == "max_range" || params == "maxrange") {
            return setConfiguration(LoRaConfigPreset::MAX_RANGE);
        } else if (params == "band_70cm" || params == "70cm") {
            return setConfiguration(LoRaConfigPreset::BAND_70CM);
        } else if (params == "band_33cm" || params == "33cm") {
            return setConfiguration(LoRaConfigPreset::BAND_33CM);
        } else if (params == "band_23cm" || params == "23cm") {
            return setConfiguration(LoRaConfigPreset::BAND_23CM);
        }
        
        // Try to parse as custom configuration: "frequency bandwidth sf cr"
        float freq, bw;
        int sf, cr;
        if (sscanf(params.c_str(), "%f %f %d %d", &freq, &bw, &sf, &cr) == 4) {
            return setCustomConfiguration(freq, bw, (uint8_t)sf, (uint8_t)cr);
        }
        
        Serial.println("ERROR: Invalid configuration parameters");
        Serial.println("Usage: SETCONFIG <preset_number|preset_name|freq bw sf cr>");
        return false;
    }
    
    Serial.println("ERROR: Unknown configuration command");
    Serial.println("Available commands: LISTCONFIG, GETCONFIG, SETCONFIG");
    return false;
}

String ConfigurationManager::getConfigStatus() const {
    String status = "\nCurrent LoRa Configuration Status:\n";
    status += "==================================\n";
    status += "Preset: ";
    if (currentPreset == LoRaConfigPreset::CUSTOM) {
        status += "Custom\n";
    } else {
        status += String(static_cast<int>(currentPreset)) + " (";
        status += getPresetName(currentPreset) + ")\n";
    }
    status += getConfigurationInfo();
    return status;
}

bool ConfigurationManager::validateConfiguration(float frequency, float bandwidth, uint8_t sf, uint8_t cr) {
    // Frequency validation (amateur radio bands)
    if (frequency < 144.0 || frequency > 1300.0) {
        Serial.println("ERROR: Frequency out of typical amateur radio range (144-1300 MHz)");
        return false;
    }
    
    // Bandwidth validation
    if (bandwidth != 7.8 && bandwidth != 10.4 && bandwidth != 15.6 && 
        bandwidth != 20.8 && bandwidth != 31.25 && bandwidth != 41.7 && 
        bandwidth != 62.5 && bandwidth != 125.0 && bandwidth != 250.0 && 
        bandwidth != 500.0) {
        Serial.println("ERROR: Invalid bandwidth (must be 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, or 500 kHz)");
        return false;
    }
    
    // Spreading factor validation
    if (sf < 5 || sf > 12) {
        Serial.println("ERROR: Spreading factor must be 5-12");
        return false;
    }
    
    // Coding rate validation
    if (cr < 5 || cr > 8) {
        Serial.println("ERROR: Coding rate must be 5-8 (representing 4/5 to 4/8)");
        return false;
    }
    
    return true;
}

uint16_t ConfigurationManager::calculateMaxPayload(uint8_t sf, uint8_t cr, float bandwidth) {
    // For SX1262, hardware limit is 255 bytes regardless of LoRa parameters
    // Time-on-air may further limit practical payload size for amateur radio compliance
    
    // Start with hardware maximum
    uint16_t maxPayload = 255;
    
    // Apply time-on-air constraints for amateur radio (1 second maximum)
    // This is a simplified calculation - real implementation would use precise formula
    if (sf >= 11 && bandwidth <= 125.0) {
        // Very slow configurations severely limited
        maxPayload = 50;
    } else if (sf >= 10 && bandwidth <= 125.0) {
        // Slow configurations moderately limited
        maxPayload = 100;
    } else if (sf >= 9 && bandwidth <= 250.0) {
        // Medium configurations slightly limited
        maxPayload = 200;
    }
    // High-speed configurations can use full 255 bytes
    
    return maxPayload;
}