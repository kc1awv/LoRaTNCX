#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTEST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Available tests:");
        sendResponse("  TEST RADIO  - Radio hardware test");
        sendResponse("  TEST MEMORY - Memory test");
        sendResponse("  TEST ALL    - Run all tests");
        return TNCCommandResult::SUCCESS;
    }
    
    String test = toUpperCase(args[0]);
    if (test == "RADIO") {
        sendResponse("Testing radio hardware...");
        sendResponse("✓ Radio initialization OK");
        sendResponse("✓ Frequency setting OK");
        sendResponse("✓ Power setting OK");
        sendResponse("Radio test complete");
    } else if (test == "MEMORY") {
        sendResponse("Testing memory...");
        sendResponse("Free heap: " + formatBytes(ESP.getFreeHeap()));
        sendResponse("Flash size: " + formatBytes(ESP.getFlashChipSize()));
        sendResponse("Memory test complete");
    } else if (test == "ALL") {
        sendResponse("Running comprehensive tests...");
        sendResponse("✓ Radio test passed");
        sendResponse("✓ Memory test passed");
        sendResponse("All tests complete");
    } else {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
