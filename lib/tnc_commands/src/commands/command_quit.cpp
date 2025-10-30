#include "CommandContext.h"

TNCCommandResult TNCCommands::handleQUIT(const String args[], int argCount) {
    sendResponse("Goodbye!");
    setMode(TNCMode::KISS_MODE);
    return TNCCommandResult::SUCCESS;
}
