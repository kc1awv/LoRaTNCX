#include "CommandContext.h"

TNCCommandResult TNCCommands::handleDIAG(const String args[], int argCount) {
    sendResponse("System Diagnostics:");
    sendResponse("===================");
    sendResponse("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    sendResponse("Flash Size: " + formatBytes(ESP.getFlashChipSize()));
    sendResponse("Free Heap: " + formatBytes(ESP.getFreeHeap()));
    sendResponse("Uptime: " + formatTime(millis()));
    sendResponse("Reset Reason: " + String((int)esp_reset_reason()));
    sendResponse("SDK Version: " + String(ESP.getSdkVersion()));
    return TNCCommandResult::SUCCESS;
}
