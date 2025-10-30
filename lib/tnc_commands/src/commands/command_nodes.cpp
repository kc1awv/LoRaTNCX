#include "CommandContext.h"

TNCCommandResult TNCCommands::handleNODES(const String args[], int argCount) {
    if (argCount > 0) {
        String cmd = toUpperCase(args[0]);
        
        if (cmd == "CLEAR") {
            // Clear node table
            nodeCount = 0;
            for (int i = 0; i < MAX_NODES; i++) {
                nodeTable[i].callsign = "";
                nodeTable[i].packetCount = 0;
            }
            sendResponse("Node table cleared");
            return TNCCommandResult::SUCCESS;
        } else if (cmd == "PURGE") {
            // Remove nodes not heard in 60 minutes
            int purged = 0;
            unsigned long now = millis();
            
            for (int i = nodeCount - 1; i >= 0; i--) {
                if ((now - nodeTable[i].lastHeard) > 3600000) { // 60 minutes
                    // Shift remaining nodes down
                    for (int j = i; j < nodeCount - 1; j++) {
                        nodeTable[j] = nodeTable[j + 1];
                    }
                    nodeCount--;
                    purged++;
                }
            }
            
            sendResponse("Purged " + String(purged) + " old nodes");
            return TNCCommandResult::SUCCESS;
        }
    }
    
    // Display node table
    sendResponse("Heard Stations:");
    sendResponse("===============");
    sendResponse("Callsign  SSID  RSSI   SNR   Count Last    First   Last Packet");
    sendResponse("--------- ---- ------ ----- ----- ------- ------- ------------");
    
    bool hasNodes = false;
    unsigned long now = millis();
    
    for (int i = 0; i < nodeCount; i++) {
        if (nodeTable[i].callsign.length() > 0) {
            hasNodes = true;
            String line = "";
            
            // Callsign (9 chars)
            line += nodeTable[i].callsign;
            for (int j = nodeTable[i].callsign.length(); j < 10; j++) line += " ";
            
            // SSID (4 chars)
            String ssidStr = nodeTable[i].ssid > 0 ? String(nodeTable[i].ssid) : "-";
            line += ssidStr;
            for (int j = ssidStr.length(); j < 5; j++) line += " ";
            
            // RSSI (6 chars)
            String rssiStr = String(nodeTable[i].lastRSSI, 1);
            line += rssiStr;
            for (int j = rssiStr.length(); j < 7; j++) line += " ";
            
            // SNR (5 chars)
            String snrStr = String(nodeTable[i].lastSNR, 1);
            line += snrStr;
            for (int j = snrStr.length(); j < 6; j++) line += " ";
            
            // Count (5 chars)
            String countStr = String(nodeTable[i].packetCount);
            line += countStr;
            for (int j = countStr.length(); j < 6; j++) line += " ";
            
            // Last heard (7 chars) - minutes ago
            unsigned long minutesAgo = (now - nodeTable[i].lastHeard) / 60000;
            String lastStr = String(minutesAgo) + "m";
            line += lastStr;
            for (int j = lastStr.length(); j < 8; j++) line += " ";
            
            // First heard (7 chars) - minutes ago
            minutesAgo = (now - nodeTable[i].firstHeard) / 60000;
            String firstStr = String(minutesAgo) + "m";
            line += firstStr;
            for (int j = firstStr.length(); j < 8; j++) line += " ";
            
            // Last packet (truncated to fit)
            String packet = nodeTable[i].lastPacket;
            if (packet.length() > 20) {
                packet = packet.substring(0, 17) + "...";
            }
            line += packet;
            
            sendResponse(line);
        }
    }
    
    if (!hasNodes) {
        sendResponse("(No stations heard yet)");
        sendResponse("");
        sendResponse("Stations will appear here as packets are received.");
        sendResponse("Node discovery requires incoming packet monitoring.");
    } else {
        sendResponse("");
        sendResponse("Total nodes: " + String(nodeCount));
    }
    
    sendResponse("");
    sendResponse("Usage: NODES [CLEAR | PURGE]");
    sendResponse("       CLEAR - Clear all node entries");
    sendResponse("       PURGE - Remove nodes not heard in 60 minutes");
    
    return TNCCommandResult::SUCCESS;
}
