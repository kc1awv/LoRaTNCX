#ifndef KISS_H
#define KISS_H

#include <Arduino.h>
#include "config.h"

/**
 * @brief KISS (Keep It Simple, Stupid) Protocol Implementation for LoRa TNC
 *
 * This class implements the KISS protocol as defined in RFC 1226, adapted for LoRa radio operation.
 * KISS provides a simple framing mechanism for packet radio data over serial connections.
 *
 * Key adaptations for LoRa:
 * - Traditional VHF/UHF timing parameters (TxDelay, Persistence, etc.) are accepted for compatibility
 *   but don't affect LoRa operation since LoRa uses different timing characteristics
 * - Hardware-specific commands (SETHARDWARE/GETHARDWARE) allow remote configuration of LoRa parameters
 * - Frame boundaries use byte stuffing to embed control characters in data streams
 */
class KISSProtocol {
public:
    /**
     * @brief Construct a new KISS Protocol handler
     */
    KISSProtocol();
    
    /**
     * @brief Process a single byte of incoming serial data
     *
     * This method implements the KISS frame parsing state machine, handling:
     * - Frame boundary detection (FEND characters)
     * - Escape sequence processing (FESC + TFEND/TFESC)
     * - Frame assembly and validation
     *
     * @param byte The incoming serial byte to process
     */
    void processSerialByte(uint8_t byte);
    
    /**
     * @brief Send a data frame using KISS protocol framing
     *
     * Transmits user data with proper KISS framing for delivery to the host application.
     * Frame format: FEND + CMD_DATA + [escaped data] + FEND
     *
     * @param data Pointer to the data buffer to send
     * @param length Number of bytes in the data buffer
     */
    void sendFrame(const uint8_t* data, size_t length);
    
    /**
     * @brief Send a command/response frame using KISS protocol framing
     *
     * Used for hardware configuration responses and status queries.
     * Frame format: FEND + command + [escaped data] + FEND
     *
     * @param cmd The KISS command byte (e.g., CMD_SETHARDWARE, CMD_GETHARDWARE)
     * @param data Pointer to the response data buffer
     * @param length Number of bytes in the response data
     */
    void sendCommand(uint8_t cmd, const uint8_t* data, size_t length);
    
    /**
     * @brief Check if a complete KISS frame is available for processing
     *
     * @return true if a complete frame has been received and is ready to process
     * @return false if no complete frame is available
     */
    bool hasFrame();
    
    /**
     * @brief Get a pointer to the received frame data
     *
     * The frame includes the command byte at index 0, followed by frame data.
     * Call hasFrame() first to ensure a frame is available.
     *
     * @return Pointer to the internal frame buffer
     */
    uint8_t* getFrame();
    
    /**
     * @brief Get the length of the received frame in bytes
     *
     * @return Total frame length including the command byte
     */
    size_t getFrameLength();
    
    /**
     * @brief Clear the current frame and prepare for the next one
     *
     * Must be called after processing a frame to free the buffer for new data.
     */
    void clearFrame();
    
    /**
     * @brief Handle a hardware configuration command
     *
     * Processes SETHARDWARE commands that configure LoRa radio parameters.
     * This method validates the command and prepares data for processing in main.cpp.
     *
     * @param data Pointer to the command data (starting after the command byte)
     * @param length Length of the command data in bytes
     * @return true if the command was recognized and radio reconfiguration may be needed
     * @return false if the command was invalid or unrecognized
     */
    bool handleHardwareCommand(const uint8_t* data, size_t length);
    
    /**
     * @brief Handle a hardware status query command
     *
     * Processes GETHARDWARE commands that request system status information.
     * This method validates the query and prepares the response format.
     *
     * @param data Pointer to the query data (starting after the command byte)
     * @param length Length of the query data in bytes
     * @return true if the query was recognized
     * @return false if the query was invalid or unrecognized
     */
    bool handleHardwareQuery(const uint8_t* data, size_t length);
    
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
