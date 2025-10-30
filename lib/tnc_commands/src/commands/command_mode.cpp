#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMODE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Current mode: " + getModeString());
        return TNCCommandResult::SUCCESS;
    }
    
    String mode = toUpperCase(args[0]);
    if (mode == "KISS") {
        setMode(TNCMode::KISS_MODE);
    } else if (mode == "COMMAND" || mode == "CMD") {
        setMode(TNCMode::COMMAND_MODE);
    } else if (mode == "TERMINAL" || mode == "TERM") {
        setMode(TNCMode::TERMINAL_MODE);
    } else if (mode == "TRANSPARENT" || mode == "TRANS") {
        setMode(TNCMode::TRANSPARENT_MODE);
    } else {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
