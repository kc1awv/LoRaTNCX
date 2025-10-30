#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCALIBRATE(const String args[], int argCount) {
    sendResponse("System calibration not yet implemented");
    return TNCCommandResult::ERROR_NOT_IMPLEMENTED;
}
