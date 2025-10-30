#include "CommandContext.h"

TNCCommandResult TNCCommands::handleBW(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Bandwidth: " + String(config.bandwidth, 1) + " kHz");
        return TNCCommandResult::SUCCESS;
    }
    
    float bw = args[0].toFloat();
    if (bw != 7.8 && bw != 10.4 && bw != 15.6 && bw != 20.8 && bw != 31.25 && 
        bw != 41.7 && bw != 62.5 && bw != 125.0 && bw != 250.0 && bw != 500.0) {
        sendResponse("ERROR: Invalid bandwidth. Valid: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.bandwidth = bw;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setBandwidth(bw)) {
        sendResponse("Bandwidth set to " + String(bw, 1) + " kHz");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set bandwidth on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
