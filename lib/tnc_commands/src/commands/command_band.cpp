#include "CommandContext.h"

TNCCommandResult TNCCommands::handleBAND(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Band: " + config.band);
        return TNCCommandResult::SUCCESS;
    }
    
    String band = toUpperCase(args[0]);
    if (band != "70CM" && band != "33CM" && band != "23CM") {
        sendResponse("ERROR: Invalid band. Valid: 70CM, 33CM, 23CM");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.band = band;
    sendResponse("Band set to " + band);
    return TNCCommandResult::SUCCESS;
}
