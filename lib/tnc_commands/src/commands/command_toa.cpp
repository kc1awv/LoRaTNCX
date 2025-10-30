#include "CommandContext.h"

TNCCommandResult TNCCommands::handleTOA(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: TOA <packet_size_bytes>");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    int packetSize = args[0].toInt();
    if (packetSize <= 0 || packetSize > 255) {
        sendResponse("ERROR: Packet size must be 1-255 bytes");
        return TNCCommandResult::ERROR_INVALID_VALUE;
    }
    
    // Simplified TOA calculation (not accurate)
    float toa = packetSize * 0.5; // Placeholder calculation
    sendResponse("Time-on-air for " + String(packetSize) + " bytes: " + String(toa, 1) + " ms");
    return TNCCommandResult::SUCCESS;
}
