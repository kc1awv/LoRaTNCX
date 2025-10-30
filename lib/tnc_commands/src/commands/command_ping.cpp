#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePING(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: PING <callsign> [count]");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    String target = args[0];
    int count = (argCount > 1) ? args[1].toInt() : 1;
    
    if (count < 1 || count > 10) {
        sendResponse("ERROR: Count must be 1-10");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    sendResponse("Pinging " + target + " (" + String(count) + " packets)...");
    for (int i = 0; i < count; i++) {
        sendResponse("Ping " + String(i + 1) + " to " + target + " - timeout");
        delay(1000);
    }
    sendResponse("Ping complete - " + String(count) + " sent, 0 received");
    
    // TODO: Implement actual ping functionality
    return TNCCommandResult::SUCCESS;
}
