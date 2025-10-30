#include "CommandContext.h"

TNCCommandResult TNCCommands::handleROUTE(const String args[], int argCount) {
    if (argCount == 0) {
        // Display routing table
        sendResponse("Routing Table:");
        sendResponse("==============");
        sendResponse("Dest      NextHop   Hops Quality LastUsed  LastUpd   Status");
        sendResponse("--------- --------- ---- ------- --------- --------- ------");
        
        bool hasRoutes = false;
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination.length() > 0) {
                hasRoutes = true;
                String line = "";
                
                // Destination (9 chars)
                line += routingTable[i].destination;
                for (int j = routingTable[i].destination.length(); j < 10; j++) line += " ";
                
                // Next hop (9 chars)
                line += routingTable[i].nextHop;
                for (int j = routingTable[i].nextHop.length(); j < 10; j++) line += " ";
                
                // Hops (4 chars)
                String hopsStr = String(routingTable[i].hops);
                line += hopsStr;
                for (int j = hopsStr.length(); j < 5; j++) line += " ";
                
                // Quality (7 chars)
                String qualStr = String(routingTable[i].quality, 2);
                line += qualStr;
                for (int j = qualStr.length(); j < 8; j++) line += " ";
                
                // Last used (9 chars) - show seconds ago
                unsigned long secondsAgo = (millis() - routingTable[i].lastUsed) / 1000;
                String lastUsedStr = String(secondsAgo) + "s";
                line += lastUsedStr;
                for (int j = lastUsedStr.length(); j < 10; j++) line += " ";
                
                // Last updated (9 chars) - show seconds ago
                secondsAgo = (millis() - routingTable[i].lastUpdated) / 1000;
                String lastUpdStr = String(secondsAgo) + "s";
                line += lastUpdStr;
                for (int j = lastUpdStr.length(); j < 10; j++) line += " ";
                
                // Status
                line += routingTable[i].isActive ? "ACTIVE" : "STALE";
                
                sendResponse(line);
            }
        }
        
        if (!hasRoutes) {
            sendResponse("(No routes configured)");
        }
        
        sendResponse("");
        sendResponse("Usage: ROUTE ADD <dest> <nexthop> [hops] [quality]");
        sendResponse("       ROUTE DEL <dest>");
        sendResponse("       ROUTE CLEAR");
        sendResponse("       ROUTE PURGE (remove stale routes)");
        
        return TNCCommandResult::SUCCESS;
    }
    
    String cmd = toUpperCase(args[0]);
    
    if (cmd == "ADD" && argCount >= 3) {
        // Add new route: ROUTE ADD destination nexthop [hops] [quality]
        String dest = toUpperCase(args[1]);
        String nextHop = toUpperCase(args[2]);
        uint8_t hops = argCount >= 4 ? args[3].toInt() : 1;
        float quality = argCount >= 5 ? args[4].toFloat() : 0.8;
        
        if (hops < 1 || hops > 7) {
            sendResponse("ERROR: Hops must be 1-7");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        
        if (quality < 0.0 || quality > 1.0) {
            sendResponse("ERROR: Quality must be 0.0-1.0");
            return TNCCommandResult::ERROR_INVALID_VALUE;
        }
        
        // Check if route already exists
        int existingIndex = -1;
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination == dest) {
                existingIndex = i;
                break;
            }
        }
        
        if (existingIndex >= 0) {
            // Update existing route
            routingTable[existingIndex].nextHop = nextHop;
            routingTable[existingIndex].hops = hops;
            routingTable[existingIndex].quality = quality;
            routingTable[existingIndex].lastUpdated = millis();
            routingTable[existingIndex].isActive = true;
            sendResponse("Updated route to " + dest + " via " + nextHop);
        } else {
            // Add new route
            if (routeCount >= MAX_ROUTES) {
                sendResponse("ERROR: Routing table full (max " + String(MAX_ROUTES) + " routes)");
                return TNCCommandResult::ERROR_SYSTEM_ERROR;
            }
            
            routingTable[routeCount].destination = dest;
            routingTable[routeCount].nextHop = nextHop;
            routingTable[routeCount].hops = hops;
            routingTable[routeCount].quality = quality;
            routingTable[routeCount].lastUsed = 0;
            routingTable[routeCount].lastUpdated = millis();
            routingTable[routeCount].isActive = true;
            routeCount++;
            
            sendResponse("Added route to " + dest + " via " + nextHop + " (" + String(hops) + " hops, Q=" + String(quality, 2) + ")");
        }
        
    } else if (cmd == "DEL" && argCount >= 2) {
        // Delete route: ROUTE DEL destination
        String dest = toUpperCase(args[1]);
        bool found = false;
        
        for (int i = 0; i < routeCount; i++) {
            if (routingTable[i].destination == dest) {
                // Shift remaining routes down
                for (int j = i; j < routeCount - 1; j++) {
                    routingTable[j] = routingTable[j + 1];
                }
                routeCount--;
                found = true;
                sendResponse("Deleted route to " + dest);
                break;
            }
        }
        
        if (!found) {
            sendResponse("Route to " + dest + " not found");
            return TNCCommandResult::ERROR_INVALID_PARAMETER;
        }
        
    } else if (cmd == "CLEAR") {
        // Clear all routes
        routeCount = 0;
        for (int i = 0; i < MAX_ROUTES; i++) {
            routingTable[i].destination = "";
            routingTable[i].isActive = false;
        }
        sendResponse("Routing table cleared");
        
    } else if (cmd == "PURGE") {
        // Remove stale routes (inactive or very old)
        int purged = 0;
        unsigned long now = millis();
        
        for (int i = routeCount - 1; i >= 0; i--) {
            bool shouldPurge = false;
            
            // Purge if inactive
            if (!routingTable[i].isActive) {
                shouldPurge = true;
            }
            // Purge if not updated in 30 minutes
            else if ((now - routingTable[i].lastUpdated) > 1800000) {
                shouldPurge = true;
            }
            
            if (shouldPurge) {
                // Shift remaining routes down
                for (int j = i; j < routeCount - 1; j++) {
                    routingTable[j] = routingTable[j + 1];
                }
                routeCount--;
                purged++;
            }
        }
        
        sendResponse("Purged " + String(purged) + " stale routes");
        
    } else {
        sendResponse("Usage: ROUTE [ADD <dest> <nexthop> [hops] [quality] | DEL <dest> | CLEAR | PURGE]");
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }
    
    return TNCCommandResult::SUCCESS;
}
