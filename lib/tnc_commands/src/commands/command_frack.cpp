#include "CommandContext.h"

TNCCommandResult TNCCommands::handleFRACK(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Frame ACK timeout: " + String(config.frack) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int frack = args[0].toInt();
    if (frack < 1000 || frack > 30000) {
        sendResponse("ERROR: FRACK must be 1000-30000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.frack = frack;
    sendResponse("Frame ACK timeout set to " + String(frack) + " ms");
    return TNCCommandResult::SUCCESS;
}
