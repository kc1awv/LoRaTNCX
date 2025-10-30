#include "CommandContext.h"

TNCCommandResult TNCCommands::handleFACTORY(const String args[], int argCount) {
    sendResponse("Factory reset - restoring default settings...");
    
    // Reset to defaults
    config.myCall = "NOCALL";
    config.frequency = 915.0;
    config.txPower = 10;
    config.spreadingFactor = 7;
    config.bandwidth = 125.0;
    config.codingRate = 5;
    config.syncWord = 0x12;
    config.beaconEnabled = false;
    config.digiEnabled = false;
    config.lineEndingCR = true;
    config.lineEndingLF = true;
    config.echoEnabled = true;
    echoEnabled = true;

    sendResponse("Factory reset complete");
    return TNCCommandResult::SUCCESS;
}
