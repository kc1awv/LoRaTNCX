#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSNR(const String args[], int argCount) {
    if (radio != nullptr) {
        float snr = radio->getSNR();
        sendResponse("Last SNR: " + String(snr, 1) + " dB");
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
