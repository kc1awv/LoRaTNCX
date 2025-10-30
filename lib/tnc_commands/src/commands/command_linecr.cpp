#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLINECR(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("LINECR: " + String(config.lineEndingCR ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }

    String option = toUpperCase(args[0]);
    if (option == "ON" || option == "1") {
        config.lineEndingCR = true;
        sendResponse("Carriage return enabled in responses");
        return TNCCommandResult::SUCCESS;
    } else if (option == "OFF" || option == "0") {
        config.lineEndingCR = false;
        sendResponse("Carriage return disabled in responses");
        return TNCCommandResult::SUCCESS;
    }

    sendResponse("Usage: LINECR [ON|OFF]");
    return TNCCommandResult::ERROR_INVALID_PARAMETER;
}
