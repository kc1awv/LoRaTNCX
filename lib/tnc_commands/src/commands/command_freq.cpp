#include "CommandContext.h"

TNCCommandResult TNCCommands::handleFREQ(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Frequency: " + String(config.frequency, 1) + " MHz");
        return TNCCommandResult::SUCCESS;
    }
    
    float freq = args[0].toFloat();
    if (freq < 902.0 || freq > 928.0) {
        sendResponse("ERROR: Frequency must be 902.0-928.0 MHz");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.frequency = freq;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setFrequency(freq)) {
        sendResponse("Frequency set to " + String(freq, 1) + " MHz");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set frequency on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
