#include "CommandContext.h"

TNCCommandResult TNCCommands::handleID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Station ID: " + String(config.idEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.idEnabled = true;
        sendResponse("Station ID enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.idEnabled = false;
        sendResponse("Station ID disabled");
    } else {
        sendResponse("Usage: ID [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
