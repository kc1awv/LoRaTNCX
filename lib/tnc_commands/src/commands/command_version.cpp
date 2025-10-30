#include "CommandContext.h"

TNCCommandResult TNCCommands::handleVERSION(const String args[], int argCount) {
    sendResponse("LoRaTNCX Terminal Node Controller");
    sendResponse("Version: 1.0.0 (Simplified)");
    sendResponse("Build Date: " + String(__DATE__) + " " + String(__TIME__));
    sendResponse("Hardware: Heltec WiFi LoRa 32 V4");
    sendResponse("");
    sendResponse("AI-Enhanced Amateur Radio TNC");
    
    return TNCCommandResult::SUCCESS;
}
