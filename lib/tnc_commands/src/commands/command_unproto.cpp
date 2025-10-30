#include "CommandContext.h"

TNCCommandResult TNCCommands::handleUNPROTO(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("UNPROTO: " + config.unprotoAddr + " VIA " + config.unprotoPath);
        return TNCCommandResult::SUCCESS;
    }
    
    config.unprotoAddr = args[0];
    if (argCount > 2 && toUpperCase(args[1]) == "VIA") {
        config.unprotoPath = args[2];
    }
    
    sendResponse("UNPROTO set to " + config.unprotoAddr + " VIA " + config.unprotoPath);
    return TNCCommandResult::SUCCESS;
}
