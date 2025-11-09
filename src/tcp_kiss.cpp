#include "tcp_kiss.h"
#include "debug.h"
#include "kiss.h"

extern KISSProtocol kiss;  // Reference to main KISS protocol handler
extern void handleHardwareConfig(uint8_t* frame, size_t length);  // Forward declaration

TCPKISSServer::TCPKISSServer() 
    : server(nullptr), serverPort(0), serverRunning(false) {
    // Initialize clients array
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        clients[i] = WiFiClient();
    }
}

TCPKISSServer::~TCPKISSServer() {
    stop();
}

bool TCPKISSServer::begin(uint16_t port) {
    if (serverRunning) {
        DEBUG_PRINTLN("TCP KISS server already running");
        return true;
    }
    
    serverPort = port;
    server = new WiFiServer(serverPort);
    
    if (!server) {
        DEBUG_PRINTLN("Failed to create TCP server");
        return false;
    }
    
    server->begin();
    serverRunning = true;
    
    DEBUG_PRINT("TCP KISS server started on port ");
    DEBUG_PRINTLN(serverPort);
    
    return true;
}

void TCPKISSServer::stop() {
    if (!serverRunning) {
        return;
    }
    
    // Disconnect all clients
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
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
    DEBUG_PRINTLN("TCP KISS server stopped");
}

void TCPKISSServer::update() {
    if (!serverRunning || !server) {
        return;
    }
    
    acceptNewClients();
    processClientData();
    cleanupClients();
}

void TCPKISSServer::acceptNewClients() {
    // Check for new connections
    WiFiClient newClient = server->accept();
    if (newClient) {
        // Find an empty slot
        for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
            if (!clients[i] || !clients[i].connected()) {
                clients[i] = newClient;
                DEBUG_PRINT("New TCP KISS client connected: ");
                DEBUG_PRINTLN(i);
                break;
            }
        }
    }
}

void TCPKISSServer::processClientData() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            // Process incoming data from this client
            while (clients[i].available()) {
                uint8_t byte = clients[i].read();
                clientKISS[i].processSerialByte(byte);
            }
            
            // Check if we have a complete KISS frame from this client
            if (clientKISS[i].hasFrame()) {
                uint8_t* frame = clientKISS[i].getFrame();
                size_t frameLen = clientKISS[i].getFrameLength();
                
                if (frameLen > 0) {
                    uint8_t cmd = frame[0] & 0x0F;
                    
                    DEBUG_PRINT("TCP client ");
                    DEBUG_PRINT(i);
                    DEBUG_PRINT(" KISS frame, cmd=");
                    DEBUG_PRINT(cmd);
                    DEBUG_PRINT(", len=");
                    DEBUG_PRINTLN(frameLen);
                    
                    // Handle hardware config commands locally
                    if (cmd == CMD_SETHARDWARE && frameLen > 1) {
                        handleHardwareConfig(frame, frameLen);
                    } else if (cmd == CMD_DATA && frameLen > 1) {
                        // Forward data frame to main KISS handler for radio transmission
                        // The main loop will process this through the radio
                        kiss.processSerialByte(FEND);
                        for (size_t j = 0; j < frameLen; j++) {
                            kiss.processSerialByte(frame[j]);
                        }
                        kiss.processSerialByte(FEND);
                    }
                }
                
                clientKISS[i].clearFrame();
            }
        }
    }
}

void TCPKISSServer::cleanupClients() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clients[i] && !clients[i].connected()) {
            DEBUG_PRINT("TCP KISS client disconnected: ");
            DEBUG_PRINTLN(i);
            clients[i].stop();
            clientKISS[i].clearFrame();
        }
    }
}

void TCPKISSServer::sendKISSFrame(const uint8_t* frame, size_t length) {
    if (!serverRunning || length == 0) {
        return;
    }
    
    // Send to all connected clients
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            sendToClient(i, frame, length);
        }
    }
}

void TCPKISSServer::sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) {
    if (clientIndex >= MAX_TCP_CLIENTS || !clients[clientIndex] || !clients[clientIndex].connected()) {
        return;
    }
    
    // Use the KISS protocol's sendFrame which handles FEND framing
    size_t written = clients[clientIndex].write(data, length);
    
    if (written != length) {
        DEBUG_PRINT("Warning: TCP client ");
        DEBUG_PRINT(clientIndex);
        DEBUG_PRINT(" partial write: ");
        DEBUG_PRINT(written);
        DEBUG_PRINT("/");
        DEBUG_PRINTLN(length);
    }
}

bool TCPKISSServer::hasClients() {
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            return true;
        }
    }
    return false;
}

uint8_t TCPKISSServer::getClientCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            count++;
        }
    }
    return count;
}
