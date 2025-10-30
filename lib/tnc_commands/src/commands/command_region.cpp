#include "CommandContext.h"

TNCCommandResult TNCCommands::handleREGION(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Region: " + config.region);
        return TNCCommandResult::SUCCESS;
    }
    
    String region = toUpperCase(args[0]);
    config.region = region;
    sendResponse("Region set to " + region);
    return TNCCommandResult::SUCCESS;
}
