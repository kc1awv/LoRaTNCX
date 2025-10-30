#include "CommandContext.h"

TNCCommandResult TNCCommands::handleBCON(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Beacon: " + String(config.beaconEnabled ? "ON" : "OFF"));
        if (config.beaconEnabled) {
            sendResponse("Interval: " + String(config.beaconInterval) + " seconds");
        }
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.beaconEnabled = true;
        if (argCount > 1) {
            int interval = args[1].toInt();
            if (interval < 30 || interval > 3600) {
                sendResponse("ERROR: Beacon interval must be 30-3600 seconds");
                return TNCCommandResult::ERROR_INVALID_VALUE;
            }
            config.beaconInterval = interval;
        }
        sendResponse("Beacon enabled, interval: " + String(config.beaconInterval) + " seconds");
    } else if (cmd == "OFF" || cmd == "0") {
        config.beaconEnabled = false;
        sendResponse("Beacon disabled");
    } else {
        sendResponse("Usage: BCON [ON|OFF] [interval_seconds]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
