#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCR(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Coding Rate: 4/" + String(config.codingRate));
        return TNCCommandResult::SUCCESS;
    }
    
    int cr = args[0].toInt();
    if (cr < 5 || cr > 8) {
        sendResponse("ERROR: Coding rate must be 5-8 (for 4/5 to 4/8)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.codingRate = cr;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setCodingRate(cr)) {
        sendResponse("Coding Rate set to 4/" + String(cr));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set coding rate on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
