#include "CommandContext.h"

TNCCommandResult TNCCommands::handleVOLTAGE(const String args[], int argCount) {
    sendResponse("Supply voltage: 5.0V (simulated)");
    return TNCCommandResult::SUCCESS;
}
