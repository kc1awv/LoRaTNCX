#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSTATS(const String args[], int argCount) {
    stats.uptime = millis();
    
    sendResponse("Packet Statistics:");
    sendResponse("==================");
    sendResponse("Transmitted: " + String(stats.packetsTransmitted) + " packets, " + 
                 String(stats.bytesTransmitted) + " bytes");
    sendResponse("Received: " + String(stats.packetsReceived) + " packets, " + 
                 String(stats.bytesReceived) + " bytes");
    sendResponse("Errors: " + String(stats.packetErrors));
    sendResponse("Uptime: " + formatTime(stats.uptime));
    sendResponse("Last RSSI: " + String(stats.lastRSSI, 1) + " dBm");
    sendResponse("Last SNR: " + String(stats.lastSNR, 1) + " dB");
    
    return TNCCommandResult::SUCCESS;
}
