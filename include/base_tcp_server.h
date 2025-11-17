#ifndef BASE_TCP_SERVER_H
#define BASE_TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "error_handling.h"

// Base class for TCP servers with common functionality
class BaseTCPServer {
public:
    BaseTCPServer(uint8_t maxClients);
    virtual ~BaseTCPServer();

    // Initialize and start the TCP server
    Result<void> begin(uint16_t port);

    // Stop the server
    void stop();

    // Check if server is running
    bool isRunning() const { return serverRunning; }

    // Get current port
    uint16_t getPort() const { return serverPort; }

    // Update - handle client connections (must be called regularly)
    void update();

    // Check if any clients are connected
    bool hasClients();

    // Get number of connected clients
    uint8_t getClientCount();

protected:
    WiFiServer* server;
    WiFiClient* clients;  // Dynamic array for clients
    uint8_t maxClients;
    uint16_t serverPort;
    bool serverRunning;

    // Virtual methods that derived classes can override
    virtual void onClientConnected(uint8_t clientIndex) {}
    virtual void onClientDisconnected(uint8_t clientIndex) {}
    virtual void processClientData(uint8_t clientIndex) {}

    // Send data to a specific client (must be implemented by derived class)
    virtual void sendToClient(uint8_t clientIndex, const uint8_t* data, size_t length) = 0;

private:
    // Handle new client connections
    void acceptNewClients();

    // Clean up disconnected clients
    void cleanupClients();
};

#endif // BASE_TCP_SERVER_H