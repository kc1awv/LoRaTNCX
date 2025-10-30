#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTXTAIL(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("TX Tail: " + String(config.txTail) + " ms");
        return TNCCommandResult::SUCCESS;
    }
    
    int tail = args[0].toInt();
    if (tail < 0 || tail > 2000) {
        sendResponse("ERROR: TX tail must be 0-2000 ms");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.txTail = tail;
    sendResponse("TX Tail set to " + String(tail) + " ms");
    return TNCCommandResult::SUCCESS;
}
