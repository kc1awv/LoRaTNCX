#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLINELF(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("LINELF: " + String(config.lineEndingLF ? "ON" : "OFF"));
        return TNCCommandResult::SUCCESS;
    }

    String option = toUpperCase(args[0]);
    if (option == "ON" || option == "1") {
        config.lineEndingLF = true;
        sendResponse("Line feed enabled in responses");
        return TNCCommandResult::SUCCESS;
    } else if (option == "OFF" || option == "0") {
        config.lineEndingLF = false;
        sendResponse("Line feed disabled in responses");
        return TNCCommandResult::SUCCESS;
    }

    sendResponse("Usage: LINELF [ON|OFF]");
    return TNCCommandResult::ERROR_INVALID_PARAMETER;
}
