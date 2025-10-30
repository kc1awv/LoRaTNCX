#include "CommandContext.h"

TNCCommandResult TNCCommands::handleAPRS(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("APRS: " + String(config.aprsEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.aprsEnabled = true;
        sendResponse("APRS enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.aprsEnabled = false;
        sendResponse("APRS disabled");
    } else {
        sendResponse("Usage: APRS [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
