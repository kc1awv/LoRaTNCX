#include "CommandContext.h"

TNCCommandResult TNCCommands::handleDEFAULT(const String args[], int argCount) {
    sendResponse("Restoring factory defaults...");
    // Reset configuration to defaults (same as constructor)
    handleFACTORY(args, argCount);
    return TNCCommandResult::SUCCESS;
}
