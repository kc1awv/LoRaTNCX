#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCAL(const String args[], int argCount) {
    sendResponse("Calibration functions:");
    sendResponse("======================");
    sendResponse("Note: Calibration not yet implemented");
    sendResponse("Radio uses factory calibration values");
    return TNCCommandResult::SUCCESS;
}
