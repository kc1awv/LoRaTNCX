#ifndef NMEA_SERVER_H
#define NMEA_SERVER_H

#include <Arduino.h>
#include <WiFi.h>

#define MAX_NMEA_CLIENTS 4
#define NMEA_DEFAULT_PORT 10110  // Standard NMEA-over-TCP port

class NMEAServer {
public:
    NMEAServer();
    ~NMEAServer();
    
    // Initialize and start the NMEA TCP server
    bool begin(uint16_t port = NMEA_DEFAULT_PORT);
    
    // Stop the server
    void stop();
    
    // Check if server is running
    bool isRunning() const { return serverRunning; }
    
    // Get current port
    uint16_t getPort() const { return serverPort; }
    
    // Update - handle client connections
    void update();
    
    // Send NMEA sentence to all connected clients
    void sendNMEA(const char* sentence);
    
    // Check if any clients are connected
    bool hasClients();
    
    // Get number of connected clients
    uint8_t getClientCount();
    
private:
    WiFiServer* server;
    WiFiClient clients[MAX_NMEA_CLIENTS];
    uint16_t serverPort;
    bool serverRunning;
    
    // Handle new client connections
    void acceptNewClients();
    
    // Clean up disconnected clients
    void cleanupClients();
    
    // Send data to a specific client
    void sendToClient(uint8_t clientIndex, const char* data);
};

#endif // NMEA_SERVER_H
