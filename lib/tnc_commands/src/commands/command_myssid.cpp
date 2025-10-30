#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMYSSID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MYSSID: " + String(config.mySSID));
        return TNCCommandResult::SUCCESS;
    }
    
    int ssid = args[0].toInt();
    if (ssid < 0 || ssid > 15) {
        sendResponse("ERROR: SSID must be 0-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.mySSID = ssid;
    sendResponse("MYSSID set to " + String(ssid));
    
    // Auto-save if enabled
    if (config.autoSave) {
        if (saveConfigurationToFlash()) {
            sendResponse("Configuration saved to flash");
        } else {
            sendResponse("Warning: Failed to save configuration");
        }
    } else {
        sendResponse("Use SAVE command to persist this setting");
    }
    
    return TNCCommandResult::SUCCESS;
}
