#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTEMPERATURE(const String args[], int argCount) {
    sendResponse("Temperature: 25.0°C (simulated)");
    return TNCCommandResult::SUCCESS;
}
