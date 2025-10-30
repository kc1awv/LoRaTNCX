#include "CommandContext.h"

TNCCommandResult TNCCommands::handleBTEXT(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Beacon text: \"" + config.beaconText + "\"");
        return TNCCommandResult::SUCCESS;
    }
    
    // Reconstruct the full text from all arguments
    String text = "";
    for (int i = 0; i < argCount; i++) {
        if (i > 0) text += " ";
        text += args[i];
    }
    
    // Remove quotes if present
    if (text.startsWith("\"") && text.endsWith("\"")) {
        text = text.substring(1, text.length() - 1);
    }
    
    if (text.length() > 128) {
        sendResponse("ERROR: Beacon text too long (max 128 characters)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.beaconText = text;
    sendResponse("Beacon text set to: \"" + text + "\"");
    return TNCCommandResult::SUCCESS;
}
