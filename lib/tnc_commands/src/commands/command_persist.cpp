#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePERSIST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Persist: " + String(config.persist));
        return TNCCommandResult::SUCCESS;
    }
    
    int persist = args[0].toInt();
    if (persist < 0 || persist > 255) {
        sendResponse("ERROR: Persist must be 0-255");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.persist = persist;
    sendResponse("Persist set to " + String(persist));
    return TNCCommandResult::SUCCESS;
}
