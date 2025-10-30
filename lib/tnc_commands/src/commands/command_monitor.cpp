#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMONITOR(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Monitor: " + String(config.monitorEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.monitorEnabled = true;
        sendResponse("Monitor enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.monitorEnabled = false;
        sendResponse("Monitor disabled");
    } else {
        sendResponse("Usage: MONITOR [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
