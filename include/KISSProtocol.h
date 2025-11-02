#pragma once

#include <Arduino.h>

// KISS Protocol Constants
#define KISS_FEND 0xC0  // Frame End
#define KISS_FESC 0xDB  // Frame Escape
#define KISS_TFEND 0xDC // Transposed Frame End
#define KISS_TFESC 0xDD // Transposed Frame Escape

// KISS Command Types
#define KISS_CMD_DATA 0x00        // Data frame
#define KISS_CMD_TXDELAY 0x01     // TX Delay
#define KISS_CMD_P 0x02           // P parameter (persistence)
#define KISS_CMD_SLOTTIME 0x03    // Slot time
#define KISS_CMD_TXTAIL 0x04      // TX tail time
#define KISS_CMD_FULLDUPLEX 0x05  // Full duplex mode
#define KISS_CMD_SETHARDWARE 0x06 // Set hardware
#define KISS_CMD_RETURN 0xFF      // Exit KISS mode

// Extended KISS Commands for LoRa Radio Configuration (Hardware-specific)
// These use the SETHARDWARE command with sub-commands in the parameter
#define KISS_LORA_SET_FREQ_LOW 0x10     // Set frequency (low byte) - followed by 4-byte frequency in Hz
#define KISS_LORA_SET_FREQ_HIGH 0x11    // Set frequency (high byte)
#define KISS_LORA_SET_TXPOWER 0x12      // Set TX power in dBm (-17 to +22)
#define KISS_LORA_SET_BANDWIDTH 0x13    // Set bandwidth (0=7.8, 1=10.4, 2=15.6, 3=20.8, 4=31.25, 5=41.7, 6=62.5, 7=125, 8=250, 9=500 kHz)
#define KISS_LORA_SET_SF 0x14           // Set spreading factor (6-12)
#define KISS_LORA_SET_CR 0x15           // Set coding rate (5=4/5, 6=4/6, 7=4/7, 8=4/8)
#define KISS_LORA_SET_PREAMBLE 0x16     // Set preamble length (6-65535) - uses 2-byte parameter
#define KISS_LORA_SET_SYNCWORD 0x17     // Set sync word (0x12=LoRaWAN public, 0x34=private)
#define KISS_LORA_SET_CRC 0x18          // Enable/disable CRC (0=disable, 1=enable)
#define KISS_LORA_SELECT_BAND 0x19      // Select predefined frequency band - parameter is band index
#define KISS_LORA_GET_CONFIG 0x1A       // Get current configuration (returns multiple response frames)
#define KISS_LORA_SAVE_CONFIG 0x1B      // Save current configuration to NVS
#define KISS_LORA_RESET_CONFIG 0x1C     // Reset to default configuration

// KISS TNC Parameters
// Note: KISS frames add overhead (FEND+CMD+FEND = 3 bytes) plus potential escaping
// Safe payload size = 255 - 3 - worst_case_escaping_overhead
#define KISS_MAX_FRAME_SIZE 255   // Maximum KISS frame size (matches LoRa limit)
#define KISS_MAX_DATA_PAYLOAD 250 // Maximum data payload (leaves room for framing + escaping)
#define KISS_DEFAULT_TXDELAY 50   // Default TX delay in 10ms units (500ms)
#define KISS_DEFAULT_P 63         // Default persistence (63/255 ≈ 25%)
#define KISS_DEFAULT_SLOTTIME 10  // Default slot time in 10ms units (100ms)
#define KISS_DEFAULT_TXTAIL 2     // Default TX tail in 10ms units (20ms)

// LoRa-specific parameter encodings
#define KISS_LORA_BW_7_8    0    // 7.8 kHz
#define KISS_LORA_BW_10_4   1    // 10.4 kHz
#define KISS_LORA_BW_15_6   2    // 15.6 kHz
#define KISS_LORA_BW_20_8   3    // 20.8 kHz
#define KISS_LORA_BW_31_25  4    // 31.25 kHz
#define KISS_LORA_BW_41_7   5    // 41.7 kHz
#define KISS_LORA_BW_62_5   6    // 62.5 kHz
#define KISS_LORA_BW_125    7    // 125 kHz
#define KISS_LORA_BW_250    8    // 250 kHz
#define KISS_LORA_BW_500    9    // 500 kHz

typedef enum
{
    KISS_STATE_IDLE,
    KISS_STATE_IN_FRAME,
    KISS_STATE_ESCAPED
} kiss_state_t;

typedef struct
{
    uint8_t txDelay;     // TX delay in 10ms units
    uint8_t persistence; // Persistence parameter (0-255)
    uint8_t slotTime;    // Slot time in 10ms units
    uint8_t txTail;      // TX tail time in 10ms units
    bool fullDuplex;     // Full duplex mode flag
} kiss_config_t;

class KissProtocol
{
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
    void (*onDataFrameCallback)(uint8_t *data, uint16_t length);
    void (*onCommandCallback)(uint8_t command, uint8_t parameter);
    void (*onLoRaCommandCallback)(uint8_t command, uint8_t *data, uint16_t length);

    // Multi-byte parameter handling
    bool expectingMultiByteParam;
    uint8_t multiByteCommand;
    uint8_t multiByteBuffer[8];  // Support up to 8-byte parameters
    uint8_t multiBytePos;
    uint8_t multiByteExpected;

    // Internal methods
    void processReceivedFrame();
    void processCommand(uint8_t command, uint8_t parameter);
    void processHardwareCommand(uint8_t *data, uint16_t length);
    void sendFrame(uint8_t command, uint8_t *data, uint16_t length);
    uint16_t escapeData(uint8_t *input, uint16_t inputLength, uint8_t *output);
    uint16_t unescapeData(uint8_t *input, uint16_t inputLength, uint8_t *output);

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
    void sendDataFrame(uint8_t *data, uint16_t length);
    void sendCommand(uint8_t command, uint8_t parameter);

    // Callback registration
    void onDataFrame(void (*callback)(uint8_t *data, uint16_t length));
    void onCommand(void (*callback)(uint8_t command, uint8_t parameter));
    void onLoRaCommand(void (*callback)(uint8_t command, uint8_t *data, uint16_t length));

    // Statistics
    uint32_t getFramesReceived() const { return framesReceived; }
    uint32_t getFramesSent() const { return framesSent; }
    uint32_t getFrameErrors() const { return frameErrors; }
    void printStatistics();
    void clearStatistics();
    void printConfiguration();

    // Utility
    bool isKissFrame(uint8_t *data, uint16_t length);
    static String commandToString(uint8_t command);
    static String loraCommandToString(uint8_t command);
    static float bandwidthIndexToValue(uint8_t index);
    static uint8_t bandwidthValueToIndex(float bandwidth);

    // Direct serial interface helpers
    void handleSerialInput();
    void sendToSerial(uint8_t *data, uint16_t length);
    
    // LoRa-specific command helpers
    void sendLoRaCommand(uint8_t command, uint8_t *data, uint16_t length);
    void sendLoRaResponse(uint8_t command, uint8_t *data, uint16_t length);
    void sendFrequencyBytes(uint32_t frequencyHz);
    void sendConfigResponse();
};