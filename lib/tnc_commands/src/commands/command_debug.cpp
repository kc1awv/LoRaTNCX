#include "CommandContext.h"

TNCCommandResult TNCCommands::handleDEBUG(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Debug level: " + String(config.debugLevel));
        return TNCCommandResult::SUCCESS;
    }
    
    int level = args[0].toInt();
    if (level < 0 || level > 3) {
        sendResponse("ERROR: Debug level must be 0-3");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.debugLevel = level;
    sendResponse("Debug level set to " + String(level));
    return TNCCommandResult::SUCCESS;
}
