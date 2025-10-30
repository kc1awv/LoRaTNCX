#include "CommandContext.h"

TNCCommandResult TNCCommands::handleMHEARD(const String args[], int argCount) {
    sendResponse("Stations heard:");
    sendResponse("===============");
    sendResponse("(No stations heard yet)");
    return TNCCommandResult::SUCCESS;
}
