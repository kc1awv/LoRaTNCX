/**
 * @file KISSProtocol.h
 * @brief KISS (Keep It Simple, Stupid) protocol implementation for TNC operation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 * 
 * Implements the KISS protocol as specified by Mike Chepponis K3MC and Phil Karn KA9Q.
 * Provides frame encoding/decoding and command handling for TNC operations.
 */

#ifndef KISS_PROTOCOL_H
#define KISS_PROTOCOL_H

#include <Arduino.h>

// KISS Protocol Constants
#define KISS_FEND     0xC0    // Frame End
#define KISS_FESC     0xDB    // Frame Escape  
#define KISS_TFEND    0xDC    // Transposed Frame End
#define KISS_TFESC    0xDD    // Transposed Frame Escape

// KISS Command Types (lower 4 bits)
#define KISS_CMD_DATA         0x00    // Data frame
#define KISS_CMD_TXDELAY      0x01    // TX delay in 10ms units
#define KISS_CMD_P            0x02    // Persistence parameter (0-255)
#define KISS_CMD_SLOTTIME     0x03    // Slot time in 10ms units  
#define KISS_CMD_TXTAIL       0x04    // TX tail (obsolete but supported)
#define KISS_CMD_FULLDUPLEX   0x05    // Full duplex mode (0=half, 1=full)
#define KISS_CMD_SETHARDWARE  0x06    // Hardware specific settings
#define KISS_CMD_RETURN       0xFF    // Exit KISS mode

// KISS Port number is in upper 4 bits (0-15)
#define KISS_PORT_MASK        0xF0
#define KISS_CMD_MASK         0x0F

// Default Parameters (matching TAPR TNC-2 defaults)
#define KISS_DEFAULT_TXDELAY      50    // 500ms
#define KISS_DEFAULT_PERSISTENCE  63    // p = 0.25
#define KISS_DEFAULT_SLOTTIME     10    // 100ms
#define KISS_DEFAULT_TXTAIL       30    // 300ms (though obsolete)
#define KISS_DEFAULT_FULLDUPLEX   0     // Half duplex

// Buffer sizes
#define KISS_MAX_FRAME_SIZE   512
#define KISS_RX_BUFFER_SIZE   1024

/**
 * @brief KISS Protocol Handler Class
 * 
 * Handles encoding and decoding of KISS frames, command processing,
 * and maintains TNC parameters according to KISS specification.
 */
class KISSProtocol {
public:
    KISSProtocol();
    
    // Initialization
    bool begin();
    
    // Frame Processing
    bool encodeFrame(const uint8_t* data, size_t dataLen, uint8_t port, uint8_t command, uint8_t* output, size_t* outputLen);
    bool decodeFrame(const uint8_t* input, size_t inputLen, uint8_t* data, size_t* dataLen, uint8_t* port, uint8_t* command);
    
    // Data Frame Helpers
    bool encodeDataFrame(const uint8_t* data, size_t dataLen, uint8_t port, uint8_t* output, size_t* outputLen);
    bool decodeDataFrame(const uint8_t* input, size_t inputLen, uint8_t* data, size_t* dataLen, uint8_t* port);
    
    // Command Frame Helpers
    bool encodeCommandFrame(uint8_t command, uint8_t parameter, uint8_t port, uint8_t* output, size_t* outputLen);
    bool processCommand(uint8_t command, uint8_t parameter, uint8_t port);
    
    // Stream Processing (for serial interface)
    void processInputStream(uint8_t byte);
    bool hasCompleteFrame();
    bool getCompleteFrame(uint8_t* data, size_t* dataLen, uint8_t* port, uint8_t* command);
    
    // Parameter Access
    uint8_t getTxDelay() const { return txDelay; }
    uint8_t getPersistence() const { return persistence; }
    uint8_t getSlotTime() const { return slotTime; }
    uint8_t getTxTail() const { return txTail; }
    bool getFullDuplex() const { return fullDuplex; }
    
    void setTxDelay(uint8_t value) { txDelay = value; }
    void setPersistence(uint8_t value) { persistence = value; }
    void setSlotTime(uint8_t value) { slotTime = value; }
    void setTxTail(uint8_t value) { txTail = value; }
    void setFullDuplex(bool value) { fullDuplex = value; }
    
    // Status
    void printParameters();
    
private:
    // KISS Parameters
    uint8_t txDelay;        // Transmitter keyup delay (10ms units)
    uint8_t persistence;    // P-persistence parameter (0-255)
    uint8_t slotTime;       // Slot time (10ms units)
    uint8_t txTail;         // TX tail time (10ms units) - obsolete
    bool fullDuplex;        // Full duplex mode
    
    // Stream processing state
    uint8_t rxBuffer[KISS_RX_BUFFER_SIZE];
    size_t rxBufferIndex;
    bool inEscapeMode;
    bool frameComplete;
    size_t completeFrameLength;
    
    // Internal helpers
    void addByteToBuffer(uint8_t byte);
    void resetBuffer();
    size_t escapeData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t maxOutputLen);
    size_t unescapeData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t maxOutputLen);
};

#endif // KISS_PROTOCOL_H