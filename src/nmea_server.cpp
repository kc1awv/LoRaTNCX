#include "nmea_server.h"
#include "debug.h"

NMEAServer::NMEAServer() 
    : server(nullptr), serverPort(0), serverRunning(false) {
    // Initialize clients array
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        clients[i] = WiFiClient();
    }
}

NMEAServer::~NMEAServer() {
    stop();
}

bool NMEAServer::begin(uint16_t port) {
    if (serverRunning) {
        DEBUG_PRINTLN("NMEA server already running");
        return true;
    }
    
    serverPort = port;
    server = new WiFiServer(serverPort);
    
    if (!server) {
        DEBUG_PRINTLN("Failed to create NMEA server");
        return false;
    }
    
    server->begin();
    serverRunning = true;
    
    DEBUG_PRINT("NMEA server started on port ");
    DEBUG_PRINTLN(serverPort);
    
    return true;
}

void NMEAServer::stop() {
    if (!serverRunning) {
        return;
    }
    
    // Disconnect all clients
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        if (clients[i]) {
            clients[i].stop();
        }
    }
    
    // Stop server
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    
    serverRunning = false;
    DEBUG_PRINTLN("NMEA server stopped");
}

void NMEAServer::update() {
    if (!serverRunning || !server) {
        return;
    }
    
    acceptNewClients();
    cleanupClients();
}

void NMEAServer::acceptNewClients() {
    // Check for new connections
    WiFiClient newClient = server->accept();
    if (newClient) {
        // Find an empty slot
        for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
            if (!clients[i] || !clients[i].connected()) {
                clients[i] = newClient;
                DEBUG_PRINT("New NMEA client connected: ");
                DEBUG_PRINTLN(i);
                break;
            }
        }
    }
}

void NMEAServer::cleanupClients() {
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        if (clients[i] && !clients[i].connected()) {
            DEBUG_PRINT("NMEA client disconnected: ");
            DEBUG_PRINTLN(i);
            clients[i].stop();
        }
    }
}

void NMEAServer::sendNMEA(const char* sentence) {
    if (!serverRunning || !sentence) {
        return;
    }
    
    // Send to all connected clients
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            sendToClient(i, sentence);
        }
    }
}

void NMEAServer::sendToClient(uint8_t clientIndex, const char* data) {
    if (clientIndex >= MAX_NMEA_CLIENTS || !clients[clientIndex] || !clients[clientIndex].connected()) {
        return;
    }
    
    // Send NMEA sentence with CRLF termination
    clients[clientIndex].print(data);
    clients[clientIndex].print("\r\n");
}

bool NMEAServer::hasClients() {
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            return true;
        }
    }
    return false;
}

uint8_t NMEAServer::getClientCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_NMEA_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            count++;
        }
    }
    return count;
}
