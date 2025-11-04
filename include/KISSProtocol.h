// KISSProtocol.h
// KISS (Keep It Simple Stupid) protocol implementation for TNC
#pragma once

#include <Arduino.h>
#include <functional>

/**
 * @brief KISS Protocol handler for binary frame communication
 * 
 * Implements KISS protocol for interfacing with packet radio applications.
 * Handles frame assembly/disassembly, byte escaping, and exit detection.
 * 
 * Protocol constants:
 * - FEND (0xC0): Frame End marker
 * - FESC (0xDB): Frame Escape
 * - TFEND (0xDC): Transposed Frame End
 * - TFESC (0xDD): Transposed Frame Escape
 * 
 * Commands:
 * - CMD_DATA (0x00): Data frame
 * - CMD_TXDELAY (0x01): TX delay parameter
 * - CMD_P (0x02): Persistence parameter
 * - CMD_SLOTTIME (0x03): Slot time
 * - CMD_TXTAIL (0x04): TX tail
 * - CMD_FULLDUPLEX (0x05): Full duplex mode
 * - CMD_SETHARDWARE (0x06): Hardware-specific
 * - CMD_RETURN (0xFF): Exit KISS mode
 * 
 * Exit mechanisms:
 * - ESC character (0x1B): TAPR TNC2 standard
 * - CMD_RETURN frame (0xFF): KISS protocol standard
 */
class KISSProtocol
{
public:
  /**
   * @brief Construct a new KISS Protocol handler
   * @param io Stream for serial communication
   */
  KISSProtocol(Stream &io);

  /**
   * @brief Process a single byte for KISS frame assembly
   * @param byte Byte to process
   * @return true if a complete frame was received
   */
  bool processByte(uint8_t byte);

  /**
   * @brief Check if a complete frame is available
   * @return true if frame is ready to be retrieved
   */
  bool hasFrame() const;

  /**
   * @brief Get the received frame data
   * @param buffer Buffer to store frame data
   * @param maxLen Maximum buffer length
   * @return Number of bytes copied (0 if no frame available)
   */
  size_t getFrame(uint8_t *buffer, size_t maxLen);

  /**
   * @brief Send a data frame with proper KISS encoding
   * @param data Data to send
   * @param len Length of data
   */
  void sendFrame(const uint8_t *data, size_t len);

  /**
   * @brief Check if exit from KISS mode was requested
   * @return true if ESC or CMD_RETURN was received
   */
  bool isExitRequested() const;

  /**
   * @brief Clear the exit request flag
   */
  void clearExitRequest();

  /**
   * @brief Set callback for received data frames
   * @param handler Function to call when data frame is received
   */
  using FrameHandler = std::function<void(const uint8_t *data, size_t len)>;
  void setFrameHandler(FrameHandler handler);

private:
  Stream &_io;
  
  // Frame reception state
  static const size_t RX_BUFFER_SIZE = 512;
  uint8_t _rxBuffer[RX_BUFFER_SIZE];
  size_t _rxIndex;
  bool _frameInProgress;
  bool _escapeNext;
  
  // Completed frame storage
  uint8_t _lastFrame[RX_BUFFER_SIZE];
  size_t _lastFrameLen;
  bool _frameAvailable;
  
  // Exit detection
  bool _exitRequested;
  
  // Optional callback for immediate frame handling
  FrameHandler _frameHandler;
  
  // Internal methods
  void processFrame(const uint8_t *frame, size_t len);
  void sendEscaped(uint8_t byte);
  void resetRxBuffer();
  
  // KISS protocol constants
  static const uint8_t FEND = 0xC0;
  static const uint8_t FESC = 0xDB;
  static const uint8_t TFEND = 0xDC;
  static const uint8_t TFESC = 0xDD;
  
  // KISS command codes
  static const uint8_t CMD_DATA = 0x00;
  static const uint8_t CMD_TXDELAY = 0x01;
  static const uint8_t CMD_P = 0x02;
  static const uint8_t CMD_SLOTTIME = 0x03;
  static const uint8_t CMD_TXTAIL = 0x04;
  static const uint8_t CMD_FULLDUPLEX = 0x05;
  static const uint8_t CMD_SETHARDWARE = 0x06;
  static const uint8_t CMD_RETURN = 0xFF;
  
  // ESC character for exit
  static const uint8_t ESC = 0x1B;
};
