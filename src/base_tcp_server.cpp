#include "base_tcp_server.h"
#include "debug.h"

BaseTCPServer::BaseTCPServer(uint8_t maxClients)
    : server(nullptr), clients(nullptr), maxClients(maxClients),
      serverPort(0), serverRunning(false) {
    // Allocate dynamic array for clients
    clients = new WiFiClient[maxClients];
}

BaseTCPServer::~BaseTCPServer() {
    stop();
    if (clients) {
        delete[] clients;
        clients = nullptr;
    }
}

Result<void> BaseTCPServer::begin(uint16_t port) {
    if (serverRunning) {
        DEBUG_PRINTLN("TCP server already running");
        return Result<void>();
    }

    serverPort = port;
    server = new WiFiServer(serverPort);

    if (!server) {
        DEBUG_PRINTLN("Failed to create TCP server");
        return Result<void>(ErrorCode::TCP_SERVER_INIT_FAILED);
    }

    server->begin();
    serverRunning = true;

    DEBUG_PRINT("TCP server started on port ");
    DEBUG_PRINTLN(serverPort);

    return Result<void>();
}

void BaseTCPServer::stop() {
    if (!serverRunning) {
        return;
    }

    // Disconnect all clients
    for (uint8_t i = 0; i < maxClients; i++) {
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
    DEBUG_PRINTLN("TCP server stopped");
}

void BaseTCPServer::update() {
    if (!serverRunning || !server) {
        return;
    }

    acceptNewClients();

    // Process data from each connected client
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && clients[i].connected()) {
            processClientData(i);
        }
    }

    cleanupClients();
}

void BaseTCPServer::acceptNewClients() {
    // Check for new connections
    WiFiClient newClient = server->accept();
    if (newClient) {
        // Find an empty slot
        for (uint8_t i = 0; i < maxClients; i++) {
            if (!clients[i] || !clients[i].connected()) {
                clients[i] = newClient;
                DEBUG_PRINT("New TCP client connected: ");
                DEBUG_PRINTLN(i);
                onClientConnected(i);
                break;
            }
        }
    }
}

void BaseTCPServer::cleanupClients() {
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && !clients[i].connected()) {
            DEBUG_PRINT("TCP client disconnected: ");
            DEBUG_PRINTLN(i);
            clients[i].stop();
            // Properly reset the WiFiClient object to prevent memory leaks
            clients[i] = WiFiClient();
            onClientDisconnected(i);
        }
    }
}

bool BaseTCPServer::hasClients() {
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && clients[i].connected()) {
            return true;
        }
    }
    return false;
}

uint8_t BaseTCPServer::getClientCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < maxClients; i++) {
        if (clients[i] && clients[i].connected()) {
            count++;
        }
    }
    return count;
}