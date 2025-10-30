#include "CommandContext.h"

TNCCommandResult TNCCommands::handleFLOW(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Flow control: " + String(config.flowControl ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.flowControl = true;
        sendResponse("Flow control enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.flowControl = false;
        sendResponse("Flow control disabled");
    } else {
        sendResponse("Usage: FLOW [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
