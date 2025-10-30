/**
 * @file TNCCommandIntegration.h
 * @brief Integration plan for comprehensive TNC commands with existing LoRaTNCX
 * @author LoRaTNCX Project
 * @date October 29, 2025
 *
 * This file shows how to integrate the comprehensive command system with
 * your existing working TNC implementation without breaking current functionality.
 */

#ifndef TNC_COMMAND_INTEGRATION_H
#define TNC_COMMAND_INTEGRATION_H

#include "TNCCommands.h"
#include "StationConfig.h"
#include "TNCManager.h"

/**
 * Enhanced TNC Manager with comprehensive command support
 * Extends your existing TNCManager to include the full command system
 */
class EnhancedTNCManager : public TNCManager
{
public:
    EnhancedTNCManager();

    // Enhanced initialization
    bool begin() override;
    void update() override;

    // Command processing integration
    void processSerialInput();
    void handleCommandModeInput(const String &input);
    void handleKISSModeDetection();

    // Mode management
    void enterCommandMode();
    void enterKISSMode();
    void enterTerminalMode();
    void enterTransparentMode();

    // Configuration integration
    bool saveConfiguration();
    bool loadConfiguration();
    void resetToDefaults();

    // Status and monitoring
    String getComprehensiveStatus();
    void printDetailedStatistics();

private:
    TNCCommandSystem *commandSystem;
    StationConfig *stationConfig;
    String inputBuffer;
    unsigned long lastCommandTime;
    bool inCommandMode;

    // Integration helpers
    void initializeEnhancedFeatures();
    void detectModeSwitch();
    bool isKISSEscapeSequence(const String &input);
    bool isCommandEscapeSequence(const String &input);
};

/**
 * Integration Implementation Examples
 */

// Example 1: Radio Parameter Command Integration
class RadioParameterIntegration
{
public:
    // Integrate FREQ command with existing ConfigurationManager
    static TNCCommandResult handleFREQ(const std::vector<String> &args,
                                       ConfigurationManager *configMgr)
    {
        if (args.empty())
        {
            // Display current frequency
            LoRaConfiguration config = configMgr->getCurrentConfiguration();
            Serial.printf("FREQ: %.3f MHz\n", config.frequency);
            return TNCCommandResult::SUCCESS;
        }

        float freq = args[0].toFloat();
        if (freq < 144.0 || freq > 1300.0)
        {
            return TNCCommandResult::ERROR_PARAMETER_OUT_OF_RANGE;
        }

        // Create custom configuration with new frequency
        LoRaConfiguration current = configMgr->getCurrentConfiguration();
        bool result = configMgr->setCustomConfiguration(
            freq, current.bandwidth,
            current.spreadingFactor, current.codingRate);

        return result ? TNCCommandResult::SUCCESS : TNCCommandResult::ERROR_OPERATION_FAILED;
    }

    // Integrate POWER command with LoRaRadio
    static TNCCommandResult handlePOWER(const std::vector<String> &args,
                                        LoRaRadio *radio)
    {
        if (args.empty())
        {
            Serial.println("POWER: 22 dBm"); // Current power from radio
            return TNCCommandResult::SUCCESS;
        }

        int power = args[0].toInt();
        if (power < -3 || power > 22)
        {
            return TNCCommandResult::ERROR_PARAMETER_OUT_OF_RANGE;
        }

        // Would need to enhance LoRaRadio to support runtime power changes
        Serial.printf("Power set to %d dBm (restart required)\n", power);
        return TNCCommandResult::SUCCESS;
    }
};

// Example 2: Station Configuration Integration
class StationConfigIntegration
{
public:
    // Integrate MYCALL command with station configuration
    static TNCCommandResult handleMYCALL(const std::vector<String> &args,
                                         StationConfig *stationConfig)
    {
        if (args.empty())
        {
            Serial.printf("MYCALL: %s\n", stationConfig->getCallsign().c_str());
            return TNCCommandResult::SUCCESS;
        }

        if (stationConfig->setCallsign(args[0]))
        {
            Serial.printf("Callsign set to %s\n", args[0].c_str());
            stationConfig->save(); // Persist to flash
            return TNCCommandResult::SUCCESS;
        }
        else
        {
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
    }

    // Integrate BCON command with beacon functionality
    static TNCCommandResult handleBCON(const std::vector<String> &args,
                                       StationConfig *stationConfig)
    {
        if (args.empty())
        {
            bool enabled = stationConfig->isBeaconEnabled();
            uint16_t interval = stationConfig->getBeaconInterval();
            Serial.printf("BCON: %s", enabled ? "ON" : "OFF");
            if (enabled)
            {
                Serial.printf(" %d", interval);
            }
            Serial.println();
            return TNCCommandResult::SUCCESS;
        }

        String mode = args[0];
        mode.toUpperCase();

        if (mode == "ON")
        {
            uint16_t interval = (args.size() > 1) ? args[1].toInt() : 300;
            stationConfig->setBeaconEnabled(true);
            stationConfig->setBeaconInterval(interval);
            stationConfig->save();
            Serial.printf("Beacon enabled, interval %d seconds\n", interval);
            return TNCCommandResult::SUCCESS;
        }
        else if (mode == "OFF")
        {
            stationConfig->setBeaconEnabled(false);
            stationConfig->save();
            Serial.println("Beacon disabled");
            return TNCCommandResult::SUCCESS;
        }

        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
};

// Example 3: Protocol Stack Integration
class ProtocolStackIntegration
{
public:
    // Integrate TXDELAY with existing KISS protocol
    static TNCCommandResult handleTXDELAY(const std::vector<String> &args,
                                          KISSProtocol *kiss)
    {
        if (args.empty())
        {
            // Get current value from KISS protocol
            Serial.println("TXDELAY: 30"); // Would get from KISSProtocol
            return TNCCommandResult::SUCCESS;
        }

        int delay = args[0].toInt();
        if (delay < 0 || delay > 255)
        {
            return TNCCommandResult::ERROR_PARAMETER_OUT_OF_RANGE;
        }

        // Use existing KISS command processing
        bool result = kiss->processCommand(CMD_TXDELAY, (uint8_t)delay);
        return result ? TNCCommandResult::SUCCESS : TNCCommandResult::ERROR_OPERATION_FAILED;
    }
};

// Example 4: Monitoring Integration
class MonitoringIntegration
{
public:
    // Comprehensive status using existing components
    static String getSystemStatus(TNCManager *tnc, LoRaRadio *radio,
                                  ConfigurationManager *configMgr,
                                  StationConfig *stationConfig)
    {
        String status = "=== LoRaTNCX System Status ===\n";

        // Station information
        status += "Station: " + stationConfig->getFullCallsign() + "\n";
        status += "Mode: KISS\n"; // Would get from current mode
        status += "Uptime: " + formatUptime(millis()) + "\n";

        // Radio status
        status += "\n=== Radio Status ===\n";
        LoRaConfiguration config = configMgr->getCurrentConfiguration();
        status += "Configuration: " + config.name + "\n";
        status += "Frequency: " + String(config.frequency, 3) + " MHz\n";
        status += "Power: 22 dBm\n";
        status += radio->getStatus() + "\n";

        // Memory status
        status += "\n=== System Resources ===\n";
        status += "Free Heap: " + formatBytes(ESP.getFreeHeap()) + "\n";
        status += "Flash Size: " + formatBytes(ESP.getFlashChipSize()) + "\n";

        return status;
    }

private:
    static String formatUptime(unsigned long ms)
    {
        unsigned long seconds = ms / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;

        return String(hours) + "h " + String(minutes % 60) + "m " + String(seconds % 60) + "s";
    }

    static String formatBytes(size_t bytes)
    {
        if (bytes < 1024)
            return String(bytes) + " B";
        else if (bytes < 1024 * 1024)
            return String(bytes / 1024) + " KB";
        else
            return String(bytes / (1024 * 1024)) + " MB";
    }
};

#endif // TNC_COMMAND_INTEGRATION_H