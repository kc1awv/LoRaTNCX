#include "CommandContext.h"

TNCCommandResult TNCCommands::handleRSSI(const String args[], int argCount) {
    if (radio != nullptr) {
        float rssi = radio->getRSSI();
        sendResponse("Last RSSI: " + String(rssi, 1) + " dBm");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
