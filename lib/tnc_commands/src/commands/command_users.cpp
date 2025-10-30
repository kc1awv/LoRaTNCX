#include "CommandContext.h"

TNCCommandResult TNCCommands::handleUSERS(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Max users: " + String(config.maxUsers));
        return TNCCommandResult::SUCCESS;
    }
    
    int users = args[0].toInt();
    if (users < 1 || users > 10) {
        sendResponse("ERROR: Max users must be 1-10");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.maxUsers = users;
    sendResponse("Max users set to " + String(users));
    return TNCCommandResult::SUCCESS;
}
