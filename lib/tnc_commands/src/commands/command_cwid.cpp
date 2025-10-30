#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCWID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("CW ID: " + String(config.cwidEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.cwidEnabled = true;
        sendResponse("CW ID enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.cwidEnabled = false;
        sendResponse("CW ID disabled");
    } else {
        sendResponse("Usage: CWID [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
