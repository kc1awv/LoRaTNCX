/**
 * @file TNCManager.h
 * @brief TNC Manager that coordinates KISS protocol, command parser, and LoRa radio
 * @author LoRaTNCX Project
 * @date October 28, 2025
 * 
 * Manages the overall TNC operation, switching between command mode and KISS mode,
 * and coordinating packet transmission/reception with the LoRa radio.
 */

#ifndef TNC_MANAGER_H
#define TNC_MANAGER_H

#include <Arduino.h>
#include "KISSProtocol.h"
#include "TNCCommandParser.h"
#include "LoRaRadio.h"
#include "HardwareConfig.h"

// Buffer sizes for packet handling
#undef TNC_MAX_PACKET_SIZE  // Remove previous definition
#define TNC_MAX_PACKET_SIZE     512
#define TNC_TX_QUEUE_SIZE       8
#define TNC_RX_BUFFER_SIZE      1024

/**
 * @brief Packet structure for queuing
 */
struct TNCPacket {
    uint8_t data[TNC_MAX_PACKET_SIZE];
    size_t length;
    uint8_t port;
    unsigned long timestamp;
};

/**
 * @brief TNC Manager Class
 * 
 * Coordinates all TNC operations including:
 * - Mode switching (Command <-> KISS)
 * - Packet transmission and reception
 * - KISS frame processing
 * - Command interface
 * - LoRa radio control
 */
class TNCManager {
public:
    TNCManager();
    
    // Initialization
    bool begin();
    void setLoRaRadio(LoRaRadio* radio) { loraRadio = radio; }
    
    // Main processing loop
    void update();
    
    // Mode management
    TNCMode getCurrentMode() const;
    void enterKissMode();
    void exitKissMode();
    void restart();
    
    // Packet operations
    bool transmitPacket(const uint8_t* data, size_t length, uint8_t port = 0);
    bool hasReceivedPacket();
    bool getReceivedPacket(uint8_t* data, size_t* length, uint8_t* port);
    
    // Statistics
    void printStatistics();
    
private:
    // Component instances
    KISSProtocol kissProtocol;
    TNCCommandParser commandParser;
    LoRaRadio* loraRadio;  // Reference to external radio instance
    
    // State management
    TNCMode currentMode;
    bool initialized;
    
    // Packet queues
    TNCPacket txQueue[TNC_TX_QUEUE_SIZE];
    uint8_t txQueueHead;
    uint8_t txQueueTail;
    uint8_t txQueueCount;
    
    TNCPacket rxQueue[TNC_TX_QUEUE_SIZE];  // Reuse same size for RX
    uint8_t rxQueueHead;
    uint8_t rxQueueTail;
    uint8_t rxQueueCount;
    
    // Statistics
    unsigned long packetsTransmitted;
    unsigned long packetsReceived;
    unsigned long kissFramesProcessed;
    unsigned long commandsProcessed;
    unsigned long lastStatsTime;
    
    // Buffers for packet processing
    uint8_t processBuffer[TNC_MAX_PACKET_SIZE];
    uint8_t kissBuffer[TNC_MAX_PACKET_SIZE + 16];  // Extra space for KISS framing
    
    // Internal methods
    void processCommandMode();
    void processKissMode();
    void processSerialInput();
    void processRadioPackets();
    void processTransmitQueue();
    
    // Queue management
    bool enqueueTxPacket(const TNCPacket& packet);
    bool dequeueTxPacket(TNCPacket& packet);
    bool enqueueRxPacket(const TNCPacket& packet);
    bool dequeueRxPacket(TNCPacket& packet);
    
    // KISS processing helpers
    void processKissFrame(const uint8_t* data, size_t length, uint8_t port, uint8_t command);
    void sendKissDataFrame(const uint8_t* data, size_t length, uint8_t port = 0);
    
    // Command callbacks (called by TNCCommandParser)
    static void onKissModeEnterCallback();
    static void onKissModeExitCallback();
    static void onRestartCallback();
    
    // Static reference for callbacks
    static TNCManager* instance;
    
    // Timing helpers
    unsigned long lastTxTime;
    unsigned long lastRxCheck;
    bool channelBusy;
    
    // LED flashing for KISS mode entry
    void flashKissLEDs();
    
    // P-persistence implementation
    bool shouldTransmit();
    void waitSlotTime();
};

#endif // TNC_MANAGER_H