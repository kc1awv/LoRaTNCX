#include "CommandContext.h"

TNCCommandResult TNCCommands::handleGRID(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Grid Square: " + (config.gridSquare.length() > 0 ? config.gridSquare : "Not set"));
        return TNCCommandResult::SUCCESS;
    }
    
    String grid = toUpperCase(args[0]);
    if (grid.length() < 4 || grid.length() > 8) {
        sendResponse("ERROR: Grid square must be 4-8 characters (e.g., FN42ni)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.gridSquare = grid;
    sendResponse("Grid square set to: " + grid);
    return TNCCommandResult::SUCCESS;
}
