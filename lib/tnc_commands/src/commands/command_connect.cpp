#include "CommandContext.h"

TNCCommandResult TNCCommands::handleCONNECT(const String args[], int argCount) {
    if (argCount == 0) {
        // Show current connections
        sendResponse("Active Connections:");
        sendResponse("==================");
        
        bool hasConnections = false;
        for (int i = 0; i < activeConnections; i++) {
            if (connections[i].state != DISCONNECTED) {
                hasConnections = true;
                String stateStr;
                switch (connections[i].state) {
                    case CONNECTING: stateStr = "CONNECTING"; break;
                    case CONNECTED: stateStr = "CONNECTED"; break;
                    case DISCONNECTING: stateStr = "DISCONNECTING"; break;
                    default: stateStr = "UNKNOWN"; break;
                }
                
                String call = connections[i].remoteCall;
                if (connections[i].remoteSSID > 0) {
                    call += "-" + String(connections[i].remoteSSID);
                }
                
                sendResponse(String(i+1) + ". " + call + " [" + stateStr + "]");
                
                if (connections[i].state == CONNECTED) {
                    unsigned long connectedTime = (millis() - connections[i].connectTime) / 1000;
                    sendResponse("   Connected for " + String(connectedTime) + " seconds");
                }
            }
        }
        
        if (!hasConnections) {
            sendResponse("(No active connections)");
        }
        
        sendResponse("");
        sendResponse("Usage: CONNECT <callsign> [ssid]");
        return TNCCommandResult::SUCCESS;
    }
    
    if (!radio) {
        sendResponse("ERROR: Radio not available");
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
    
    if (config.myCall == "NOCALL") {
        sendResponse("ERROR: Set station callsign first (MYCALL command)");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    String targetCall = toUpperCase(args[0]);
    uint8_t targetSSID = 0;
    
    if (argCount >= 2) {
        targetSSID = args[1].toInt();
        if (targetSSID > 15) {
            sendResponse("ERROR: SSID must be 0-15");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
    }
    
    // Check if already connected to this station
    for (int i = 0; i < activeConnections; i++) {
        if (connections[i].remoteCall == targetCall && 
            connections[i].remoteSSID == targetSSID &&
            connections[i].state != DISCONNECTED) {
            sendResponse("ERROR: Already connected/connecting to " + targetCall + 
                        (targetSSID > 0 ? "-" + String(targetSSID) : ""));
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
    }
    
    // Find available connection slot
    int connectionIndex = -1;
    if (activeConnections < MAX_CONNECTIONS) {
        connectionIndex = activeConnections;
        activeConnections++;
    } else {
        // Look for a disconnected slot
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i].state == DISCONNECTED) {
                connectionIndex = i;
                break;
            }
        }
    }
    
    if (connectionIndex == -1) {
        sendResponse("ERROR: Maximum connections reached (" + String(MAX_CONNECTIONS) + ")");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    // Initialize connection
    connections[connectionIndex].remoteCall = targetCall;
    connections[connectionIndex].remoteSSID = targetSSID;
    connections[connectionIndex].state = CONNECTING;
    connections[connectionIndex].connectTime = millis();
    connections[connectionIndex].lastActivity = millis();
    connections[connectionIndex].vs = 0;
    connections[connectionIndex].vr = 0;
    connections[connectionIndex].va = 0;
    connections[connectionIndex].retryCount = 0;
    connections[connectionIndex].pollBit = true;
    
    // Send SABM (Set Asynchronous Balanced Mode) frame
    String connectFrame = "SABM:" + config.myCall;
    if (config.mySSID > 0) {
        connectFrame += "-" + String(config.mySSID);
    }
    connectFrame += ">" + targetCall;
    if (targetSSID > 0) {
        connectFrame += "-" + String(targetSSID);
    }
    connectFrame += ":CONNECT_REQUEST:" + String(millis());
    
    if (radio->transmit(connectFrame)) {
        String displayCall = targetCall + (targetSSID > 0 ? "-" + String(targetSSID) : "");
        sendResponse("Connecting to " + displayCall + "...");
        sendResponse("Sent SABM frame, waiting for UA response");
        
        // Update statistics
        stats.packetsTransmitted++;
        stats.bytesTransmitted += connectFrame.length();
        
        // Note: In a real implementation, we would wait for UA (Unnumbered Acknowledgment)
        // For this simplified version, we'll simulate the connection after a delay
        sendResponse("Connection request sent successfully");
        sendResponse("Use DISCONNECT to terminate the connection");
        
        return TNCCommandResult::SUCCESS;
    } else {
        // Connection failed, reset state
        connections[connectionIndex].state = DISCONNECTED;
        if (connectionIndex == activeConnections - 1) {
            activeConnections--;
        }
        
        sendResponse("ERROR: Failed to send connection request");
        stats.packetErrors++;
        return TNCCommandResult::ERROR_HARDWARE_ERROR;
    }
}
