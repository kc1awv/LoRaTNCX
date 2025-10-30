#include "CommandContext.h"

TNCCommandResult TNCCommands::handleRETRY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Retry count: " + String(config.retry));
        return TNCCommandResult::SUCCESS;
    }
    
    int retry = args[0].toInt();
    if (retry < 0 || retry > 15) {
        sendResponse("ERROR: Retry count must be 0-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.retry = retry;
    sendResponse("Retry count set to " + String(retry));
    return TNCCommandResult::SUCCESS;
}
