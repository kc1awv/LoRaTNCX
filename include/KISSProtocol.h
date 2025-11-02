#pragma once

#include <Arduino.h>

// KISS Protocol Constants
#define KISS_FEND    0xC0  // Frame End
#define KISS_FESC    0xDB  // Frame Escape
#define KISS_TFEND   0xDC  // Transposed Frame End
#define KISS_TFESC   0xDD  // Transposed Frame Escape

// KISS Command Types
#define KISS_CMD_DATA       0x00  // Data frame
#define KISS_CMD_TXDELAY    0x01  // TX Delay
#define KISS_CMD_P          0x02  // P parameter (persistence)
#define KISS_CMD_SLOTTIME   0x03  // Slot time
#define KISS_CMD_TXTAIL     0x04  // TX tail time
#define KISS_CMD_FULLDUPLEX 0x05  // Full duplex mode
#define KISS_CMD_SETHARDWARE 0x06 // Set hardware
#define KISS_CMD_RETURN     0xFF  // Exit KISS mode

// KISS TNC Parameters
// Note: KISS frames add overhead (FEND+CMD+FEND = 3 bytes) plus potential escaping
// Safe payload size = 255 - 3 - worst_case_escaping_overhead
#define KISS_MAX_FRAME_SIZE     255  // Maximum KISS frame size (matches LoRa limit)
#define KISS_MAX_DATA_PAYLOAD   250  // Maximum data payload (leaves room for framing + escaping)
#define KISS_DEFAULT_TXDELAY 50      // Default TX delay in 10ms units (500ms)
#define KISS_DEFAULT_P       63   // Default persistence (63/255 ≈ 25%)
#define KISS_DEFAULT_SLOTTIME 10  // Default slot time in 10ms units (100ms)
#define KISS_DEFAULT_TXTAIL  2    // Default TX tail in 10ms units (20ms)

typedef enum {
    KISS_STATE_IDLE,
    KISS_STATE_IN_FRAME,
    KISS_STATE_ESCAPED
} kiss_state_t;

typedef struct {
    uint8_t txDelay;      // TX delay in 10ms units
    uint8_t persistence;  // Persistence parameter (0-255)
    uint8_t slotTime;     // Slot time in 10ms units
    uint8_t txTail;       // TX tail time in 10ms units
    bool fullDuplex;      // Full duplex mode flag
} kiss_config_t;

class KissProtocol {
private:
    // KISS protocol state
    kiss_state_t state;
    kiss_config_t config;
    
    // Frame buffers
    uint8_t rxBuffer[KISS_MAX_FRAME_SIZE];
    uint16_t rxBufferPos;
    uint8_t txBuffer[KISS_MAX_FRAME_SIZE];
    
    // Statistics
    uint32_t framesReceived;
    uint32_t framesSent;
    uint32_t frameErrors;
    
    // Callback functions
    void (*onDataFrameCallback)(uint8_t* data, uint16_t length);
    void (*onCommandCallback)(uint8_t command, uint8_t parameter);
    
    // Internal methods
    void processCommand(uint8_t command, uint8_t parameter);
    void sendFrame(uint8_t command, uint8_t* data, uint16_t length);
    uint16_t escapeData(uint8_t* input, uint16_t inputLength, uint8_t* output);
    uint16_t unescapeData(uint8_t* input, uint16_t inputLength, uint8_t* output);
    
public:
    KissProtocol();
    
    // Initialization
    void begin();
    void reset();
    
    // Configuration
    void setTxDelay(uint8_t delay);
    void setPersistence(uint8_t p);
    void setSlotTime(uint8_t slotTime);
    void setTxTail(uint8_t tail);
    void setFullDuplex(bool enable);
    
    // Get configuration
    kiss_config_t getConfig() const { return config; }
    uint8_t getTxDelay() const { return config.txDelay; }
    uint8_t getPersistence() const { return config.persistence; }
    uint8_t getSlotTime() const { return config.slotTime; }
    uint8_t getTxTail() const { return config.txTail; }
    bool isFullDuplex() const { return config.fullDuplex; }
    
    // Frame processing
    void processByte(uint8_t byte);
    void sendDataFrame(uint8_t* data, uint16_t length);
    void sendCommand(uint8_t command, uint8_t parameter);
    
    // Callback registration
    void onDataFrame(void (*callback)(uint8_t* data, uint16_t length));
    void onCommand(void (*callback)(uint8_t command, uint8_t parameter));
    
    // Statistics
    uint32_t getFramesReceived() const { return framesReceived; }
    uint32_t getFramesSent() const { return framesSent; }
    uint32_t getFrameErrors() const { return frameErrors; }
    void printStatistics();
    void printConfiguration();
    
    // Utility
    bool isKissFrame(uint8_t* data, uint16_t length);
    static String commandToString(uint8_t command);
    
    // Direct serial interface helpers
    void handleSerialInput();
    void sendToSerial(uint8_t* data, uint16_t length);
};