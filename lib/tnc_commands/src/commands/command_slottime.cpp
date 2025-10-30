#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSLOTTIME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Slot Time: " + String(config.slotTime) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int slot = args[0].toInt();
    if (slot < 10 || slot > 1000) {
        sendResponse("ERROR: Slot time must be 10-1000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.slotTime = slot;
    sendResponse("Slot Time set to " + String(slot) + " ms");
    return TNCCommandResult::SUCCESS;
}
