#ifndef TCP_KISS_H
#define TCP_KISS_H

#include <Arduino.h>
#include <WiFi.h>
#include "kiss.h"

#define MAX_TCP_CLIENTS 4
#define TCP_KISS_DEFAULT_PORT 8001

class TCPKISSServer {
public:
    TCPKISSServer();
    ~TCPKISSServer();
    
    // Initialize and start the TCP server
    bool begin(uint16_t port = TCP_KISS_DEFAULT_PORT);
    
    // Stop the server
    void stop();
    
    // Check if server is running
    bool isRunning() const { return serverRunning; }
    
    // Get current port
    uint16_t getPort() const { return serverPort; }
    
    // Update - handle client connections and data
    void update();
    
    // Send KISS frame to all connected clients
    void sendKISSFrame(const uint8_t* frame, size_t length);
    
    // Check if any clients are connected
    bool hasClients();
    
    // Get number of connected clients
    uint8_t getClientCount();
    
private:
    WiFiServer* server;
    WiFiClient clients[MAX_TCP_CLIENTS];
    KISSProtocol clientKISS[MAX_TCP_CLIENTS];
    uint16_t serverPort;
    bool serverRunning;
    
    // Handle new client connections
    void acceptNewClients();
    
    // Process data from connected clients
    void processClientData();
    
    // Clean up disconnected clients
    void cleanupClients();
    
    // Send data to a specific client
    void sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length);
};

#endif // TCP_KISS_H
