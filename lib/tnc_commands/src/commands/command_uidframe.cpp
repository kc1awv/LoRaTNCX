#include "CommandContext.h"

TNCCommandResult TNCCommands::handleUIDFRAME(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: UIDFRAME <text>");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    // Combine all arguments into message text
    String message = "";
    for (int i = 0; i < argCount; i++) {
        if (i > 0) message += " ";
        message += args[i];
    }
    
    // Create simplified AX.25 UI frame format
    // Format: "UI:sourceCall>destination,path:message"
    // For simplicity, we'll use APRS-style addressing
    String uiFrame = "UI:" + config.myCall;
    if (config.mySSID > 0) {
        uiFrame += "-" + String(config.mySSID);
    }
    
    // Use unprotocol address if configured, otherwise use CQ
    String destination = config.unprotoAddr.length() > 0 ? config.unprotoAddr : "CQ";
    uiFrame += ">" + destination;
    
    // Add path if configured
    if (config.unprotoPath.length() > 0) {
        uiFrame += "," + config.unprotoPath;
    }
    
    uiFrame += ":" + message;
    
    // Validate frame size (LoRa has 255 byte limit)
    if (uiFrame.length() > 240) {
        sendResponse("ERROR: Frame too large (" + String(uiFrame.length()) + " bytes, max 240)");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Transmit the UI frame
    if (radio->transmit(uiFrame)) {
        sendResponse("UI frame transmitted (" + String(uiFrame.length()) + " bytes)");
        sendResponse("Frame: " + uiFrame);
        
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += uiFrame.length();
        
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("ERROR: Transmission failed");
        stats.packetErrors++;
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
}
