#include "nmea_server.h"
#include "debug.h"
#include "error_handling.h"

NMEAServer::NMEAServer() : BaseTCPServer(MAX_NMEA_CLIENTS) {
    // Base class handles initialization
}

NMEAServer::~NMEAServer() {
    // Base class destructor handles cleanup
}

Result<void> NMEAServer::begin(uint16_t port) {
    return BaseTCPServer::begin(port);
}

void NMEAServer::sendNMEA(const char* sentence) {
    if (!isRunning() || !sentence) {
        return;
    }
    
    // Send to all connected clients
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && clients[i].connected()) {
            sendToClient(i, (const uint8_t*)sentence, strlen(sentence));
        }
    }
}

void NMEAServer::sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) {
    if (clientIndex >= maxClients || !clients[clientIndex] || !clients[clientIndex].connected()) {
        return;
    }
    
    // Send NMEA sentence with CRLF termination
    clients[clientIndex].write(data, length);
    clients[clientIndex].print("\r\n");
}
