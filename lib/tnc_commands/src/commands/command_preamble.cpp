#include "CommandContext.h"

TNCCommandResult TNCCommands::handlePREAMBLE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Preamble Length: " + String(config.preambleLength) + " symbols");
        return TNCCommandResult::SUCCESS;
    }
    
    int length = args[0].toInt();
    if (length < 6 || length > 65535) {
        sendResponse("ERROR: Preamble length must be 6-65535 symbols");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.preambleLength = length;
    sendResponse("Preamble length set to " + String(length) + " symbols");
    return TNCCommandResult::SUCCESS;
}
