#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTERMINAL(const String args[], int argCount) {
    setMode(TNCMode::TERMINAL_MODE);
    return TNCCommandResult::SUCCESS;
}
