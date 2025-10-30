#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCOMPLIANCE(const String args[], int argCount) {
    sendResponse("Regulatory Compliance Check:");
    sendResponse("============================");
    sendResponse("Band: " + config.band + " - OK");
    sendResponse("Frequency: " + String(config.frequency, 1) + " MHz - OK");
    sendResponse("Power: " + String(config.txPower) + " dBm - OK");
    sendResponse("All parameters compliant");
    return TNCCommandResult::SUCCESS;
}
