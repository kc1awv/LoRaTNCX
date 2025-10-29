/**
 * @file KISSProtocol.h
 * @brief KISS protocol implementation for TNC communication
 * @author LoRaTNCX Project
 * @date October 28, 2025
 *
 * Implements the KISS (Keep It Simple Stupid) protocol for Terminal Node Controller
 * communication. This allows the TNC to interface with packet radio applications
 * like APRS, Winlink, and other amateur radio digital modes.
 *
 * KISS Protocol Reference:
 * - Frame start/end: 0xC0 (FEND)
 * - Frame escape: 0xDB (FESC)
 * - Transposed frame end: 0xDC (TFEND)
 * - Transposed frame escape: 0xDD (TFESC)
 */

#ifndef KISS_PROTOCOL_H
#define KISS_PROTOCOL_H

#include <Arduino.h>

// KISS protocol constants
#define FEND 0xC0  // Frame End
#define FESC 0xDB  // Frame Escape
#define TFEND 0xDC // Transposed Frame End
#define TFESC 0xDD // Transposed Frame Escape

// KISS command codes
#define CMD_DATA 0x00        // Data frame
#define CMD_TXDELAY 0x01     // TX delay
#define CMD_P 0x02           // Persistence parameter
#define CMD_SLOTTIME 0x03    // Slot time
#define CMD_TXTAIL 0x04      // TX tail
#define CMD_FULLDUPLEX 0x05  // Full duplex
#define CMD_SETHARDWARE 0x06 // Set hardware
#define CMD_RETURN 0xFF      // Return

// Buffer sizes
#define KISS_RX_BUFFER_SIZE 512
#define KISS_TX_BUFFER_SIZE 512

class KISSProtocol
{
public:
    /**
     * @brief Initialize KISS protocol handler
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Process incoming serial data for KISS frames
     * @return true if complete frame received
     */
    bool processIncoming();

    /**
     * @brief Process a single byte for KISS frame assembly
     * @param byte Byte to process
     * @return true if complete frame received
     */
    bool processByte(uint8_t byte);

    /**
     * @brief Check if a complete KISS frame is available
     * @return true if frame available
     */
    bool available();

    /**
     * @brief Get the last received KISS frame
     * @param buffer Buffer to store frame data
     * @param maxLength Maximum buffer length
     * @return Number of bytes in frame, 0 if no frame
     */
    size_t getFrame(uint8_t *buffer, size_t maxLength);

    /**
     * @brief Send data as KISS frame
     * @param data Data to send
     * @param length Length of data
     * @return true if sent successfully
     */
    bool sendFrame(const uint8_t *data, size_t length);

    /**
     * @brief Send data frame (command 0x00)
     * @param data Data to send
     * @param length Length of data
     * @return true if sent successfully
     */
    bool sendData(const uint8_t *data, size_t length);

    /**
     * @brief Process KISS command
     * @param command Command code
     * @param parameter Command parameter
     * @return true if command processed successfully
     */
    bool processCommand(uint8_t command, uint8_t parameter);

    /**
     * @brief Get KISS protocol statistics
     * @return Status string
     */
    String getStatus();

    /**
     * @brief Check if exit from KISS mode has been requested
     * @return true if exit requested
     */
    bool isExitRequested();

    /**
     * @brief Clear the exit request flag
     */
    void clearExitRequest();

private:
    uint8_t rxBuffer[KISS_RX_BUFFER_SIZE]; // Receive buffer
    size_t rxIndex;                        // Current receive index
    bool frameInProgress;                  // Frame reception in progress
    bool escapeNext;                       // Next byte is escaped

    uint8_t lastFrame[KISS_RX_BUFFER_SIZE]; // Last complete frame
    size_t lastFrameLength;                 // Length of last frame
    bool frameAvailable;                    // Frame available flag

    // Statistics
    unsigned long framesReceived; // Total frames received
    unsigned long framesSent;     // Total frames sent
    unsigned long errors;         // Protocol errors

    // TNC parameters (set via KISS commands)
    uint8_t txDelay;     // TX delay parameter
    uint8_t persistence; // Persistence parameter
    uint8_t slotTime;    // Slot time parameter
    uint8_t txTail;      // TX tail parameter
    bool fullDuplex;     // Full duplex mode
    bool exitRequested;  // Exit KISS mode requested

    /**
     * @brief Process a complete KISS frame
     * @param frame Frame data
     * @param length Frame length
     */
    void processFrame(const uint8_t *frame, size_t length);

    /**
     * @brief Send raw byte with KISS escaping
     * @param byte Byte to send
     */
    void sendEscaped(uint8_t byte);

    /**
     * @brief Reset receive buffer
     */
    void resetRxBuffer();
};

#endif // KISS_PROTOCOL_H