#include "CommandContext.h"

TNCCommandResult TNCCommands::handleDIGI(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Digipeater Configuration:");
        sendResponse("Status: " + String(config.digiEnabled ? "ON" : "OFF"));
        if (config.digiEnabled) {
            sendResponse("Max hops: " + String(config.digiPath));
            sendResponse("Callsign: " + config.myCall + (config.mySSID > 0 ? "-" + String(config.mySSID) : ""));
        }
        sendResponse("");
        sendResponse("Digipeater aliases supported:");
        sendResponse("• WIDE1-1, WIDE2-1, WIDE2-2");
        sendResponse("• Direct callsign addressing");
        sendResponse("• WIDEn-N flood algorithm");
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    if (cmd == "ON" || cmd == "1") {
        if (config.myCall == "NOCALL") {
            sendResponse("ERROR: Set station callsign first (MYCALL command)");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        config.digiEnabled = true;
        sendResponse("Digipeater enabled");
        sendResponse("Using callsign: " + config.myCall + (config.mySSID > 0 ? "-" + String(config.mySSID) : ""));
    } else if (cmd == "OFF" || cmd == "0") {
        config.digiEnabled = false;
        sendResponse("Digipeater disabled");
    } else if (cmd == "HOPS" && argCount > 1) {
        int hops = args[1].toInt();
        if (hops < 1 || hops > 7) {
            sendResponse("ERROR: Max hops must be 1-7");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        config.digiPath = hops;
        sendResponse("Max hops set to " + String(hops));
    } else if (cmd == "TEST" && argCount >= 2) {
        // Test digipeater path processing
        String testPath = args[1];
        sendResponse("Testing path: " + testPath);
        
        // Simulate path processing
        if (shouldDigipeat(testPath)) {
            String newPath = processDigipeatPath(testPath);
            sendResponse("Would digipeat with path: " + newPath);
        } else {
            sendResponse("Would NOT digipeat this path");
        }
    } else if (cmd == "STATS") {
        // Show digipeater statistics (placeholder for now)
        sendResponse("Digipeater Statistics:");
        sendResponse("Packets digipeated: 0");  // TODO: implement stats
        sendResponse("Packets dropped: 0");
        sendResponse("Current load: 0%");
    } else {
        sendResponse("Usage: DIGI [ON|OFF|HOPS <1-7>|TEST <path>|STATS]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
