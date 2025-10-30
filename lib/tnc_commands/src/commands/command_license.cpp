#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLICENSE(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("License Class: " + config.licenseClass);
        return TNCCommandResult::SUCCESS;
    }
    
    String license = toUpperCase(args[0]);
    if (license != "NOVICE" && license != "TECHNICIAN" && license != "GENERAL" && 
        license != "ADVANCED" && license != "EXTRA") {
        sendResponse("ERROR: Invalid license class. Valid: NOVICE, TECHNICIAN, GENERAL, ADVANCED, EXTRA");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    config.licenseClass = license;
    sendResponse("License class set to: " + license);
    return TNCCommandResult::SUCCESS;
}
