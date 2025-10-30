#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTXDELAY(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Delay: " + String(config.txDelay) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int delay = args[0].toInt();
    if (delay < 0 || delay > 2000) {
        sendResponse("ERROR: TX delay must be 0-2000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txDelay = delay;
    sendResponse("TX Delay set to " + String(delay) + " ms");
    return TNCCommandResult::SUCCESS;
}
