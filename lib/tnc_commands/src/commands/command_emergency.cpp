#include "CommandContext.h"

TNCCommandResult TNCCommands::handleEMERGENCY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Emergency mode: " + String(config.emergencyMode ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.emergencyMode = true;
        sendResponse("Emergency mode enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.emergencyMode = false;
        sendResponse("Emergency mode disabled");
    } else {
        sendResponse("Usage: EMERGENCY [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
