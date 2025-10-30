#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePROMPT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Prompt: " + String(config.promptEnabled ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.promptEnabled = true;
        promptEnabled = true;
        sendResponse("Prompt enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.promptEnabled = false;
        promptEnabled = false;
        sendResponse("Prompt disabled");
    } else {
        sendResponse("Usage: PROMPT [ON|OFF]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
