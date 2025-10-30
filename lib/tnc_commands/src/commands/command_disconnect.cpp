#include "CommandContext.h"

TNCCommandResult TNCCommands::handleDISCONNECT(const String args[], int argCount) {
    if (argCount == 0) {
        // Disconnect all connections
        int disconnected = 0;
        
        for (int i = 0; i < activeConnections; i++) {
            if (connections[i].state == CONNECTED || connections[i].state == CONNECTING) {
                String displayCall = connections[i].remoteCall;
                if (connections[i].remoteSSID > 0) {
                    displayCall += "-" + String(connections[i].remoteSSID);
                }
                
                if (sendDisconnectFrame(i)) {
                    sendResponse("Disconnected from " + displayCall);
                    connections[i].state = DISCONNECTED;
                    disconnected++;
                } else {
                    sendResponse("Failed to disconnect from " + displayCall);
                }
            }
        }
        
        if (disconnected == 0) {
            sendResponse("No active connections to disconnect");
        } else {
            sendResponse("Disconnected " + String(disconnected) + " connection(s)");
        }
        
        return TNCCommandResult::SUCCESS;
    }
    
    // Disconnect specific station
    String targetCall = toUpperCase(args[0]);
    uint8_t targetSSID = 0;
    
    if (argCount >= 2) {
        targetSSID = args[1].toInt();
        if (targetSSID > 15) {
            sendResponse("ERROR: SSID must be 0-15");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    // Find the connection
    int connectionIndex = -1;
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == targetCall && 
            connections[i].remoteSSID == targetSSID &&
            (connections[i].state == CONNECTED || connections[i].state == CONNECTING)) {
            connectionIndex = i;
            break;
        }
    }
    
    if (connectionIndex == -1) {
        String displayCall = targetCall + (targetSSID > 0 ? "-" + String(targetSSID) : "");
        sendResponse("ERROR: No active connection to " + displayCall);
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    String displayCall = connections[connectionIndex].remoteCall;
    if (connections[connectionIndex].remoteSSID > 0) {
        displayCall += "-" + String(connections[connectionIndex].remoteSSID);
    }
    
    if (sendDisconnectFrame(connectionIndex)) {
        sendResponse("Disconnected from " + displayCall);
        
        // Show connection statistics
        if (connections[connectionIndex].state == CONNECTED) {
            unsigned long connectedTime = (millis() - connections[connectionIndex].connectTime) / 1000;
            sendResponse("Connection duration: " + String(connectedTime) + " seconds");
        }
        
        connections[connectionIndex].state = DISCONNECTED;
        return TNCCommandResult::SUCCESS;
    } else {
        sendResponse("Failed to send disconnect to " + displayCall);
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
}
