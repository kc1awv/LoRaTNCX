/**
 * Enhanced KISS Protocol Implementation
 * 
 * This extends standard KISS with additional features while maintaining
 * backward compatibility with existing KISS applications.
 * 
 * Key Enhancements:
 * - RX indication frames with RSSI/SNR metadata
 * - Statistics reporting
 * - Error reporting and diagnostics
 * - Flow control support
 * - Better debugging capabilities
 */

#pragma once
#include <Arduino.h>
#include <functional>

class EnhancedKISS {
public:
    using FrameCB = std::function<void(const uint8_t *, size_t)>;
    using CommandCB = std::function<void(uint8_t, const uint8_t *, size_t)>;
    using RxIndicationCB = std::function<void(const uint8_t *, size_t, int16_t, float, uint32_t)>;

    EnhancedKISS(Stream &serial, size_t rxCap, size_t txCap);
    ~EnhancedKISS();

    // Standard KISS compatibility
    void setOnFrame(FrameCB cb) { onFrame = cb; }
    void setOnCommand(CommandCB cb) { onCommand = cb; }
    
    // Enhanced features
    void setOnRxIndication(RxIndicationCB cb) { onRxIndication = cb; }

    void pushSerialByte(uint8_t b);
    void writeFrame(const uint8_t *data, size_t len);
    void writeFrameTo(Stream &s, const uint8_t *data, size_t len);
    void writeCommand(uint8_t cmd, const uint8_t *data = nullptr, size_t len = 0);
    
    // Enhanced methods
    void writeRxIndication(const uint8_t *data, size_t len, int16_t rssi, float snr, uint32_t timestamp = 0);
    void writeStatistics();
    void writeError(uint8_t errorCode, const char* description = nullptr);
    void writeStatus();

    // Standard KISS commands (backward compatible)
    enum FrameType : uint8_t {
        DATA_FRAME = 0x00,
        TX_DELAY = 0x01,
        PERSISTENCE = 0x02,
        SLOT_TIME = 0x03,
        TX_TAIL = 0x04,
        FULL_DUPLEX = 0x05,
        SET_HARDWARE = 0x06,
        RETURN = 0xFF
    };

    // Enhanced KISS commands
    enum EnhancedCommands : uint8_t {
        // Status and monitoring (0x10-0x17)
        STATUS_REQUEST = 0x10,      // Request TNC status
        STATISTICS = 0x11,          // Statistics report
        ERROR_REPORT = 0x12,        // Error notification
        RX_INDICATION = 0x13,       // Received packet with metadata
        
        // Flow control (0x18-0x1F)  
        FLOW_CONTROL = 0x18,        // Flow control commands
        BUFFER_STATUS = 0x19,       // Buffer status report
        
        // Configuration (0x20-0x2F)
        PROTOCOL_VERSION = 0x20,    // Protocol version info
        ENHANCED_CONFIG = 0x21,     // Enhanced configuration
    };

    // Error codes
    enum ErrorCodes : uint8_t {
        ERROR_NONE = 0x00,
        ERROR_BUFFER_OVERFLOW = 0x01,
        ERROR_INVALID_FRAME = 0x02,
        ERROR_RADIO_FAILURE = 0x03,
        ERROR_CONFIG_INVALID = 0x04,
        ERROR_TIMEOUT = 0x05,
        ERROR_CRC_FAILURE = 0x06,
    };

    struct Statistics {
        uint32_t framesRx = 0;
        uint32_t framesTx = 0;
        uint32_t commandsRx = 0;
        uint32_t bytesRx = 0;
        uint32_t bytesTx = 0;
        uint32_t errors = 0;
        uint32_t uptime = 0;
        
        // Enhanced statistics
        uint32_t rxIndicationsCount = 0;
        uint32_t bufferOverflows = 0;
        uint32_t crcErrors = 0;
        int16_t lastRssi = -999;
        float lastSnr = -99.9;
        uint32_t lastRxTime = 0;
        uint32_t lastTxTime = 0;
    };

    const Statistics &getStats() const { return stats; }
    void resetStats();

private:
    Stream &port;
    size_t rxCap;
    uint8_t *rx;
    size_t rxLen = 0;
    
    enum RxState { WAIT_FEND, IN_FRAME, ESCAPED };
    RxState rxState = WAIT_FEND;
    
    // Constants
    static const uint8_t FEND = 0xC0;
    static const uint8_t FESC = 0xDB;
    static const uint8_t TFEND = 0xDC;
    static const uint8_t TFESC = 0xDD;
    
    // Callbacks
    FrameCB onFrame;
    CommandCB onCommand;
    RxIndicationCB onRxIndication;
    
    // Statistics
    Statistics stats;
    unsigned long bootTime;
    
    // Internal methods
    void processFrameData();
    void resetFrame();
    void updateStats();
    void escapeAndWrite(Stream &s, uint8_t b);
    
    // Enhanced frame processing
    void processEnhancedCommand(uint8_t cmd, const uint8_t *data, size_t len);
    void sendProtocolVersion();
    void sendBufferStatus();
};

// Implementation of key methods
inline void EnhancedKISS::writeRxIndication(const uint8_t *data, size_t len, 
                                           int16_t rssi, float snr, uint32_t timestamp) {
    stats.rxIndicationsCount++;
    stats.lastRssi = rssi;
    stats.lastSnr = snr;
    stats.lastRxTime = timestamp ? timestamp : millis();
    updateStats();
    
    // Format: CMD RSSI_H RSSI_L SNR_INT SNR_FRAC TIMESTAMP_4BYTES DATA...
    port.write(FEND);
    escapeAndWrite(port, RX_INDICATION);
    
    // RSSI as 16-bit signed integer
    escapeAndWrite(port, (rssi >> 8) & 0xFF);
    escapeAndWrite(port, rssi & 0xFF);
    
    // SNR as integer and fractional parts
    int8_t snr_int = (int8_t)snr;
    uint8_t snr_frac = (uint8_t)((snr - snr_int) * 10);
    escapeAndWrite(port, snr_int);
    escapeAndWrite(port, snr_frac);
    
    // Timestamp (4 bytes, little-endian)
    uint32_t ts = stats.lastRxTime;
    escapeAndWrite(port, ts & 0xFF);
    escapeAndWrite(port, (ts >> 8) & 0xFF);
    escapeAndWrite(port, (ts >> 16) & 0xFF);
    escapeAndWrite(port, (ts >> 24) & 0xFF);
    
    // Data payload
    for (size_t i = 0; i < len; i++) {
        escapeAndWrite(port, data[i]);
    }
    
    port.write(FEND);
}

inline void EnhancedKISS::writeStatistics() {
    port.write(FEND);
    escapeAndWrite(port, STATISTICS);
    
    // Pack statistics into frame
    // Format: RX_4BYTES TX_4BYTES ERRORS_4BYTES UPTIME_4BYTES RSSI_2BYTES SNR_2BYTES
    uint32_t rx = stats.framesRx;
    escapeAndWrite(port, rx & 0xFF);
    escapeAndWrite(port, (rx >> 8) & 0xFF);
    escapeAndWrite(port, (rx >> 16) & 0xFF);
    escapeAndWrite(port, (rx >> 24) & 0xFF);
    
    uint32_t tx = stats.framesTx;
    escapeAndWrite(port, tx & 0xFF);
    escapeAndWrite(port, (tx >> 8) & 0xFF);
    escapeAndWrite(port, (tx >> 16) & 0xFF);
    escapeAndWrite(port, (tx >> 24) & 0xFF);
    
    uint32_t errors = stats.errors;
    escapeAndWrite(port, errors & 0xFF);
    escapeAndWrite(port, (errors >> 8) & 0xFF);
    escapeAndWrite(port, (errors >> 16) & 0xFF);
    escapeAndWrite(port, (errors >> 24) & 0xFF);
    
    uint32_t uptime = stats.uptime;
    escapeAndWrite(port, uptime & 0xFF);
    escapeAndWrite(port, (uptime >> 8) & 0xFF);
    escapeAndWrite(port, (uptime >> 16) & 0xFF);
    escapeAndWrite(port, (uptime >> 24) & 0xFF);
    
    // RSSI and SNR
    int16_t rssi = stats.lastRssi;
    escapeAndWrite(port, rssi & 0xFF);
    escapeAndWrite(port, (rssi >> 8) & 0xFF);
    
    int16_t snr = (int16_t)(stats.lastSnr * 10); // SNR * 10 for decimal precision
    escapeAndWrite(port, snr & 0xFF);
    escapeAndWrite(port, (snr >> 8) & 0xFF);
    
    port.write(FEND);
}

inline void EnhancedKISS::writeError(uint8_t errorCode, const char* description) {
    stats.errors++;
    updateStats();
    
    port.write(FEND);
    escapeAndWrite(port, ERROR_REPORT);
    escapeAndWrite(port, errorCode);
    
    // Optional description string
    if (description) {
        size_t len = strlen(description);
        for (size_t i = 0; i < len && i < 64; i++) { // Limit description length
            escapeAndWrite(port, description[i]);
        }
    }
    
    port.write(FEND);
}