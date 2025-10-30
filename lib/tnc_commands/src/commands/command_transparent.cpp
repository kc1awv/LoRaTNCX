#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTRANSPARENT(const String args[], int argCount) {
    setMode(TNCMode::TRANSPARENT_MODE);
    return TNCCommandResult::SUCCESS;
}
