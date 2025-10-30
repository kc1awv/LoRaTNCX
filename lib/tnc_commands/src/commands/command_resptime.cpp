#include "CommandContext.h"

TNCCommandResult TNCCommands::handleRESPTIME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Response Time: " + String(config.respTime) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int resp = args[0].toInt();
    if (resp < 100 || resp > 10000) {
        sendResponse("ERROR: Response time must be 100-10000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.respTime = resp;
    sendResponse("Response Time set to " + String(resp) + " ms");
    return TNCCommandResult::SUCCESS;
}
