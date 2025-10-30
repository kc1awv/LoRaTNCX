#include "CommandContext.h"

TNCCommandResult TNCCommands::handleBEACON(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Beacon: " + String(config.beaconEnabled ? "ON" : "OFF"));
        if (config.beaconEnabled) {
            sendResponse("Interval: " + String(config.beaconInterval) + " seconds");
        }
        sendResponse("Text: " + config.beaconText);
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        config.beaconEnabled = true;
        sendResponse("Beacon enabled");
    } else if (cmd == "OFF" || cmd == "0") {
        config.beaconEnabled = false;
        sendResponse("Beacon disabled");
    } else if (cmd == "INTERVAL" && argCount > 1) {
        int interval = args[1].toInt();
        if (interval < 30 || interval > 3600) {
            sendResponse("ERROR: Beacon interval must be 30-3600 seconds");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.beaconInterval = interval;
        sendResponse("Beacon interval set to " + String(interval) + " seconds");
    } else if (cmd == "TEXT" && argCount > 1) {
        // Combine remaining arguments into beacon text
        String newText = "";
        for (int i = 1; i < argCount; i++) {
            if (i > 1) newText += " ";
            newText += args[i];
        }
        if (newText.length() > 200) {
            sendResponse("ERROR: Beacon text too long (max 200 characters)");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.beaconText = newText;
        sendResponse("Beacon text set to: " + config.beaconText);
    } else if (cmd == "NOW") {
        // Transmit beacon immediately
        if (!radio) {
            sendResponse("ERROR: Radio not available");
            return TNCCommandResult::ERROR_HARDWARE_ERROR;
        }
        
        return transmitBeacon();
    } else if (cmd == "POSITION" && argCount >= 3) {
        // Set position for APRS-style beacon
        config.latitude = args[1].toFloat();
        config.longitude = args[2].toFloat();
        if (argCount >= 4) {
            config.altitude = args[3].toInt();
        }
        sendResponse("Position set: " + String(config.latitude, 6) + ", " + String(config.longitude, 6));
        if (argCount >= 4) {
            sendResponse("Altitude: " + String(config.altitude) + "m");
        }
    } else {
        sendResponse("Usage: BEACON [ON|OFF|INTERVAL <seconds>|TEXT <text>|NOW|POSITION <lat> <lon> [alt]]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
