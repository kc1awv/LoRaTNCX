#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSYNC(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Sync Word: 0x" + String(config.syncWord, HEX));
        return TNCCommandResult::SUCCESS;
    }
    
    String syncStr = args[0];
    uint16_t sync;
    if (syncStr.startsWith("0x") || syncStr.startsWith("0X")) {
        sync = strtol(syncStr.c_str(), NULL, 16);
    } else {
        sync = syncStr.toInt();
    }
    
    config.syncWord = sync;
    
    // Apply to radio hardware
    if (radio != nullptr && radio->setSyncWord(sync)) {
        sendResponse("Sync Word set to 0x" + String(sync, HEX));
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Failed to set sync word on radio hardware");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
