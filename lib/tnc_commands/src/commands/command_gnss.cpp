#include "CommandContext.h"

TNCCommandResult TNCCommands::handleGNSS(const String args[], int argCount) {
    if (!gnssGetEnabledCallback || !gnssSetEnabledCallback) {
        sendResponse("ERROR: GNSS not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    if (argCount == 0) {
        // Show current GNSS status
        bool enabled = gnssGetEnabledCallback();
        sendResponse("GNSS: " + String(enabled ? "ENABLED" : "DISABLED"));
        return TNCCommandResult::SUCCESS;
    }
    
    String arg = args[0];
    arg.toUpperCase();
    
    if (arg == "ON" || arg == "1" || arg == "ENABLE" || arg == "TRUE") {
        if (gnssSetEnabledCallback(true)) {
            if (gnssGetEnabledCallback) {
                config.gnssEnabled = gnssGetEnabledCallback();
            } else {
                config.gnssEnabled = true;
            }
            sendResponse("GNSS enabled");
            return TNCCommandResult::SUCCESS;
        } else {
            if (gnssGetEnabledCallback) {
                config.gnssEnabled = gnssGetEnabledCallback();
            }
            sendResponse("ERROR: Failed to enable GNSS");
            return TNCCommandResult::ERROR_HARDWARE_ERROR;
        }
    } else if (arg == "OFF" || arg == "0" || arg == "DISABLE" || arg == "FALSE") {
        if (gnssSetEnabledCallback(false)) {
            if (gnssGetEnabledCallback) {
                config.gnssEnabled = gnssGetEnabledCallback();
            } else {
                config.gnssEnabled = false;
            }
            sendResponse("GNSS disabled");
            return TNCCommandResult::SUCCESS;
        } else {
            if (gnssGetEnabledCallback) {
                config.gnssEnabled = gnssGetEnabledCallback();
            }
            sendResponse("ERROR: Failed to disable GNSS");
            return TNCCommandResult::ERROR_SYSTEM_ERROR;
        }
    } else if (arg == "STATUS") {
        // Show detailed GNSS status
        bool enabled = gnssGetEnabledCallback();
        sendResponse("GNSS Status:");
        sendResponse("  Enabled: " + String(enabled ? "YES" : "NO"));
        sendResponse("  Saved state: " + String(config.gnssEnabled ? "ON" : "OFF"));
        if (enabled) {
            sendResponse("  Module: Active");
            sendResponse("  Use 'STATUS' command to see fix status");
        } else {
            sendResponse("  Module: Inactive");
        }
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Invalid argument. Use ON, OFF, or STATUS");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
}