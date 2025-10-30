#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMAXFRAME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Max Frame: " + String(config.maxFrame));
        return TNCCommandResult::SUCCESS;
    }
    
    int max = args[0].toInt();
    if (max < 1 || max > 15) {
        sendResponse("ERROR: Max frame must be 1-15");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.maxFrame = max;
    sendResponse("Max Frame set to " + String(max));
    return TNCCommandResult::SUCCESS;
}
