#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePOWER(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Power: " + String(config.txPower) + " dBm");
        return TNCCommandResult::SUCCESS;
    }
    
    int power = args[0].toInt();
    if (power < -9 || power > 22) {
        sendResponse("ERROR: Power must be -9 to 22 dBm");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txPower = power;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setTxPower(power)) {
        sendResponse("TX Power set to " + String(power) + " dBm");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set TX power on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
