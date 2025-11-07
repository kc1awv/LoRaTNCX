#ifndef KISS_H
#define KISS_H

#include <Arduino.h>
#include "config.h"

class KISSProtocol {
public:
    KISSProtocol();
    
    // Process incoming serial data
    void processSerialByte(uint8_t byte);
    
    // Send a frame via KISS protocol
    void sendFrame(const uint8_t* data, size_t length);
    
    // Send a command response (like SETHARDWARE responses)
    void sendCommand(uint8_t cmd, const uint8_t* data, size_t length);
    
    // Get the decoded frame (if complete)
    bool hasFrame();
    uint8_t* getFrame();
    size_t getFrameLength();
    void clearFrame();
    
    // Note: Standard KISS parameters (TxDelay, Persistence, SlotTime, TxTail, FullDuplex)
    // are VHF/UHF FM specific and not applicable to LoRa operation.
    // Commands are accepted for compatibility but ignored.
    
    // Hardware configuration handler (returns true if radio needs reconfiguration)
    bool handleHardwareCommand(const uint8_t* data, size_t length);
    
private:
    uint8_t rxBuffer[SERIAL_BUFFER_SIZE];
    size_t rxBufferIndex;
    bool frameReady;
    bool escapeNext;
    bool inFrame;
    
    // KISS parameters (accepted for compatibility but not used with LoRa)
    uint8_t txDelay;      // Not used - LoRa doesn't need PTT delay
    uint8_t persistence;  // Not used - LoRa uses CAD, not CSMA
    uint8_t slotTime;     // Not used - LoRa uses CAD, not CSMA
    uint8_t txTail;       // Not used - LoRa has no squelch tail
    bool fullDuplex;      // Not used - SX1262 is half-duplex only
    
    void handleCommand(uint8_t cmd, uint8_t value);
    void resetRxBuffer();
};

#endif // KISS_H
