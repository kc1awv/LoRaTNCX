#include "CommandContext.h"

TNCCommandResult TNCCommands::handleRESET(const String args[], int argCount) {
    sendResponse("Resetting TNC...");
    delay(1000);
    ESP.restart();
    return TNCCommandResult::SUCCESS;
}
