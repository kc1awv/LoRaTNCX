#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLINKTEST(const String args[], int argCount) {
    if (argCount == 0) {
        sendResponse("Usage: LINKTEST <callsign> [count]");
        return TNCCommandResult::ERROR_INSUFFICIENT_ARGS;
    }
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    String targetCall = args[0];
    int testCount = 3; // Default to 3 ping packets
    
    if (argCount >= 2) {
        testCount = args[1].toInt();
        if (testCount < 1 || testCount > 10) {
            sendResponse("ERROR: Count must be between 1 and 10");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    sendResponse("Link test to " + targetCall + " (" + String(testCount) + " packets):");
    
    int successCount = 0;
    unsigned long totalTime = 0;
    
    for (int i = 1; i <= testCount; i++) {
        // Construct ping packet format: "PING:sourceCall>targetCall:sequenceNumber:timestamp"
        String pingPacket = "PING:" + config.myCall + ">" + targetCall + ":" + String(i) + ":" + String(millis());
        
        sendResponse("Ping " + String(i) + "...");
        
        // Send ping packet
        unsigned long startTime = millis();
        if (!radio->transmit(pingPacket)) {
            sendResponse("  TX FAILED");
            continue;
        }
        
        // Wait for response (timeout after 5 seconds)
        bool responseReceived = false;
        unsigned long timeout = startTime + 5000;
        String response;
        
        while (millis() < timeout && !responseReceived) {
            if (radio->available()) {
                if (radio->receive(response)) {
                    // Check if this is a PONG response to our PING
                    if (response.startsWith("PONG:" + targetCall + ">" + config.myCall + ":" + String(i))) {
                        responseReceived = true;
                        unsigned long roundTripTime = millis() - startTime;
                        totalTime += roundTripTime;
                        successCount++;
                        
                        // Get signal quality from last received packet
                        float rssi = radio->getRSSI();
                        float snr = radio->getSNR();
                        
                        sendResponse("  PONG received: " + String(roundTripTime) + "ms, RSSI=" + String(rssi, 1) + "dBm, SNR=" + String(snr, 1) + "dB");
                    }
                }
            }
            delay(10); // Small delay to prevent busy-waiting
        }
        
        if (!responseReceived) {
            sendResponse("  TIMEOUT (no response)");
        }
        
        // Small delay between pings
        if (i < testCount) {
            delay(500);
        }
    }
    
    // Summary
    sendResponse("--- Link test complete ---");
    sendResponse("Packets sent: " + String(testCount));
    sendResponse("Packets received: " + String(successCount));
    sendResponse("Packet loss: " + String(((testCount - successCount) * 100) / testCount) + "%");
    
    if (successCount > 0) {
        unsigned long avgTime = totalTime / successCount;
        sendResponse("Average round-trip time: " + String(avgTime) + "ms");
    }
    
    return TNCCommandResult::SUCCESS;
}
