#include "CommandContext.h"

TNCCommandResult TNCCommands::handleOLED(const String args[], int argCount) {
    if (!oledGetEnabledCallback || !oledSetEnabledCallback) {
        sendResponse("ERROR: OLED control not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }

    if (argCount == 0) {
        bool enabled = oledGetEnabledCallback();
        sendResponse("OLED: " + String(enabled ? "ENABLED" : "DISABLED"));
        return TNCCommandResult::SUCCESS;
    }

    String arg = args[0];
    arg.toUpperCase();

    if (arg == "ON" || arg == "1" || arg == "ENABLE" || arg == "TRUE") {
        if (oledSetEnabledCallback(true)) {
            if (oledGetEnabledCallback) {
                config.oledEnabled = oledGetEnabledCallback();
            } else {
                config.oledEnabled = true;
            }
            sendResponse("OLED enabled");
            return TNCCommandResult::SUCCESS;
        } else {
            if (oledGetEnabledCallback) {
                config.oledEnabled = oledGetEnabledCallback();
            }
            sendResponse("ERROR: Failed to enable OLED");
            return TNCCommandResult::ERROR_HARDWARE_ERROR;
        }
    } else if (arg == "OFF" || arg == "0" || arg == "DISABLE" || arg == "FALSE") {
        if (oledSetEnabledCallback(false)) {
            if (oledGetEnabledCallback) {
                config.oledEnabled = oledGetEnabledCallback();
            } else {
                config.oledEnabled = false;
            }
            sendResponse("OLED disabled");
            return TNCCommandResult::SUCCESS;
        } else {
            if (oledGetEnabledCallback) {
                config.oledEnabled = oledGetEnabledCallback();
            }
            sendResponse("ERROR: Failed to disable OLED");
            return TNCCommandResult::ERROR_SYSTEM_ERROR;
        }
    } else if (arg == "STATUS") {
        bool enabled = oledGetEnabledCallback();
        sendResponse("OLED Status:");
        sendResponse("  Enabled: " + String(enabled ? "YES" : "NO"));
        sendResponse("  Saved state: " + String(config.oledEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Invalid argument. Use ON, OFF, or STATUS");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
}
