#include "CommandContext.h"

TNCCommandResult TNCCommands::handleUIDWAIT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("UIDWAIT: " + String(config.uidWait ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.uidWait = true;
        sendResponse("UIDWAIT enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.uidWait = false;
        sendResponse("UIDWAIT disabled");
    } else {
        sendResponse("Usage: UIDWAIT [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
