#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMEMORY(const String args[], int argCount) {
    sendResponse("Memory usage:");
    sendResponse("=============");
    sendResponse("Free heap: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Flash size: " + formatBytes(ESP.getFlashChipSize()));
    return TNCCommandResult::SUCCESS;
}
