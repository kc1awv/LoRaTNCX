#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMCON(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("MCON: " + String(config.mconEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.mconEnabled = true;
        sendResponse("Multiple connections enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.mconEnabled = false;
        sendResponse("Multiple connections disabled");
    } else {
        sendResponse("Usage: MCON [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
