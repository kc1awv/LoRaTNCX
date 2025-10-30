#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSENSITIVITY(const String args[], int argCount) {
    sendResponse("Receiver sensitivity: -137 dBm (typical)");
    return TNCCommandResult::SUCCESS;
}
