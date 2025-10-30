#include "CommandContext.h"

TNCCommandResult TNCCommands::handleUPTIME(const String args[], int argCount) {
    sendResponse("Uptime: " + formatTime(millis()));
    return TNCCommandResult::SUCCESS;
}
