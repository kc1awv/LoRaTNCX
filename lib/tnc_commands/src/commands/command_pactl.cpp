#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePACTL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Power Amplifier Control: " + String(config.paControl ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.paControl = true;
        sendResponse("Power amplifier control enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.paControl = false;
        sendResponse("Power amplifier control disabled");
    } else {
        sendResponse("Usage: PACTL [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
