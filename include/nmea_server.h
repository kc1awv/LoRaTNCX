#ifndef NMEA_SERVER_H
#define NMEA_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "error_handling.h"
#include "base_tcp_server.h"

#define MAX_NMEA_CLIENTS 4
#define NMEA_DEFAULT_PORT 10110  // Standard NMEA-over-TCP port

class NMEAServer : public BaseTCPServer {
public:
    NMEAServer();
    ~NMEAServer();
    
    // Initialize and start the NMEA TCP server
    Result<void> begin(uint16_t port = NMEA_DEFAULT_PORT);
    
    // Send NMEA sentence to all connected clients
    void sendNMEA(const char* sentence);
    
private:
    // Override base class methods
    void sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) override;
};

#endif // NMEA_SERVER_H
