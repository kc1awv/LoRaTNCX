#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSTATUS(const String args[], int argCount) {
    sendResponse("LoRaTNCX System Status");
    sendResponse("=====================");
    sendResponse("Mode: " + getModeString());
    sendResponse("Uptime: " + formatTime(millis()));
    sendResponse("Free Memory: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Flash Size: " + formatBytes(ESP.getFlashChipSize()));
    
    return TNCCommandResult::SUCCESS;
}
