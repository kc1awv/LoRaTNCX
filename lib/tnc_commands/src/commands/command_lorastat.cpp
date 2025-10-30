#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLORASTAT(const String args[], int argCount) {
    sendResponse("LoRa Statistics:");
    sendResponse("================");
    sendResponse("Frequency: " + String(config.frequency, 1) + " MHz");
    sendResponse("Power: " + String(config.txPower) + " dBm");
    sendResponse("SF: " + String(config.spreadingFactor));
    sendResponse("BW: " + String(config.bandwidth, 1) + " kHz");
    sendResponse("CR: 4/" + String(config.codingRate));
    return TNCCommandResult::SUCCESS;
}
