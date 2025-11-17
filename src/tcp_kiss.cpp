#include "tcp_kiss.h"
#include "debug.h"
#include "kiss.h"
#include "error_handling.h"

extern KISSProtocol kiss;  // Reference to main KISS protocol handler
extern void handleHardwareConfig(uint8_t* frame, size_t length);  // Forward declaration

TCPKISSServer::TCPKISSServer() : BaseTCPServer(MAX_TCP_CLIENTS) {
    // Initialize KISS protocol handlers for each client
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        // KISSProtocol default constructor should handle initialization
    }
}

TCPKISSServer::~TCPKISSServer() {
    // Base class destructor handles cleanup
}

Result<void> TCPKISSServer::begin(uint16_t port) {
    return BaseTCPServer::begin(port);
}

void TCPKISSServer::sendKISSFrame(const uint8_t* frame, size_t length) {
    if (!isRunning() || length == 0) {
        return;
    }
    
    // Send to all connected clients
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && clients[i].connected()) {
            sendToClient(i, frame, length);
        }
    }
}

void TCPKISSServer::onClientConnected(uint8_t clientIndex) {
    // Reset KISS state for new client
    clientKISS[clientIndex].clearFrame();
}

void TCPKISSServer::onClientDisconnected(uint8_t clientIndex) {
    // Clean up KISS state
    clientKISS[clientIndex].clearFrame();
}

void TCPKISSServer::processClientData(uint8_t clientIndex) {
    // Process incoming data from this client
    while (clients[clientIndex].available()) {
        uint8_t byte = clients[clientIndex].read();
        clientKISS[clientIndex].processSerialByte(byte);
    }
    
    // Check if we have a complete KISS frame from this client
    if (clientKISS[clientIndex].hasFrame()) {
        uint8_t* frame = clientKISS[clientIndex].getFrame();
        size_t frameLen = clientKISS[clientIndex].getFrameLength();
        
        if (frameLen > 0) {
            uint8_t cmd = frame[0] & 0x0F;
            
            DEBUG_PRINT("TCP client ");
            DEBUG_PRINT(clientIndex);
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
        
        clientKISS[clientIndex].clearFrame();
    }
}

void TCPKISSServer::sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) {
    if (clientIndex >= maxClients || !clients[clientIndex] || !clients[clientIndex].connected()) {
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
