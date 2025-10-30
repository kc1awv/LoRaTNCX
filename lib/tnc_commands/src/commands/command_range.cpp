#include "CommandContext.h"

TNCCommandResult TNCCommands::handleRANGE(const String args[], int argCount) {
    sendResponse("Estimated range: 5-15 km (depending on conditions)");
    return TNCCommandResult::SUCCESS;
}
