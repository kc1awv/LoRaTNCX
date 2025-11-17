#ifndef TCP_KISS_H
#define TCP_KISS_H

#include <Arduino.h>
#include <WiFi.h>
#include "kiss.h"
#include "error_handling.h"
#include "base_tcp_server.h"

#define MAX_TCP_CLIENTS 4
#define TCP_KISS_DEFAULT_PORT 8001

class TCPKISSServer : public BaseTCPServer {
public:
    TCPKISSServer();
    ~TCPKISSServer();
    
    // Initialize and start the TCP server
    Result<void> begin(uint16_t port = TCP_KISS_DEFAULT_PORT);
    
    // Send KISS frame to all connected clients
    void sendKISSFrame(const uint8_t* frame, size_t length);
    
private:
    KISSProtocol clientKISS[MAX_TCP_CLIENTS];
    
    // Override base class methods
    void onClientConnected(uint8_t clientIndex) override;
    void onClientDisconnected(uint8_t clientIndex) override;
    void processClientData(uint8_t clientIndex) override;
    void sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) override;
};

#endif // TCP_KISS_H
