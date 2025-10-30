#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMYCALL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MYCALL: " + config.myCall);
        return TNCCommandResult::SUCCESS;
    } else {
        // Enhanced callsign validation
        String callsign = toUpperCase(args[0]);
        
        // Basic callsign format validation
        if (callsign.length() < 3 || callsign.length() > 6) {
            sendResponse("ERROR: Callsign must be 3-6 characters");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
        // Check for valid characters (letters and numbers only)
        for (int i = 0; i < callsign.length(); i++) {
            char c = callsign.charAt(i);
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                sendResponse("ERROR: Callsign can only contain letters and numbers");
                return TNCCommandResult::ERROR_INVALID_PARAMETER;
            }
        }
        
        // Must start with a letter
        if (callsign.charAt(0) < 'A' || callsign.charAt(0) > 'Z') {
            sendResponse("ERROR: Callsign must start with a letter");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
        // Update configuration
        config.myCall = callsign;
        sendResponse("Callsign set to: " + callsign);
        
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
}
