#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSF(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Spreading Factor: " + String(config.spreadingFactor));
        return TNCCommandResult::SUCCESS;
    }
    
    int sf = args[0].toInt();
    if (sf < 6 || sf > 12) {
        sendResponse("ERROR: Spreading factor must be 6-12");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.spreadingFactor = sf;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setSpreadingFactor(sf)) {
        sendResponse("Spreading Factor set to " + String(sf));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set spreading factor on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
