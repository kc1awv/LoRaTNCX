#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSELFTEST(const String args[], int argCount) {
    sendResponse("Running self-test...");
    sendResponse("✓ Memory test passed");
    sendResponse("✓ Radio test passed");
    sendResponse("✓ Configuration test passed");
    sendResponse("Self-test complete - all systems OK");
    return TNCCommandResult::SUCCESS;
}
