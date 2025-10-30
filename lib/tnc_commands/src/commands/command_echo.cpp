#include "CommandContext.h"

TNCCommandResult TNCCommands::handleECHO(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Echo: " + String(config.echoEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.echoEnabled = true;
        echoEnabled = true;
        sendResponse("Echo enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.echoEnabled = false;
        echoEnabled = false;
        sendResponse("Echo disabled");
    } else {
        sendResponse("Usage: ECHO [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
