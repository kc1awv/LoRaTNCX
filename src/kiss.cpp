#include "kiss.h"

KISSProtocol::KISSProtocol() 
    : rxBufferIndex(0), frameReady(false), escapeNext(false), inFrame(false),
      txDelay(50), persistence(63), slotTime(10), txTail(10), fullDuplex(false) {
}

void KISSProtocol::processSerialByte(uint8_t byte) {
    // KISS Protocol Frame Parsing State Machine:
    // KISS uses byte stuffing to embed control characters in data streams.
    // FEND (0xC0) marks frame boundaries, FESC (0xDB) escapes special bytes.
    // TFEND (0xDC) and TFESC (0xDD) are the escaped versions.
    //
    // State transitions:
    // - Outside frame: Wait for FEND to start new frame
    // - In frame: Accumulate bytes until FEND, handling escape sequences
    // - Escape pending: Next byte is escaped (FESC seen, waiting for TFEND/TFESC)
    
    // Handle FEND - Frame boundary marker
    if (byte == FEND) {
        if (inFrame && rxBufferIndex > 0) {
            // End of frame detected - we have a complete frame
            frameReady = true;
            inFrame = false;
        } else {
            // Start of new frame - reset buffer and enter frame state
            resetRxBuffer();
            inFrame = true;
        }
        return;
    }
    
    // Ignore data bytes received outside of frame boundaries
    if (!inFrame) {
        return;
    }
    
    // Handle KISS escape sequences for byte stuffing
    if (escapeNext) {
        escapeNext = false;
        if (byte == TFEND) {
            byte = FEND;  // Escaped FEND becomes literal FEND
        } else if (byte == TFESC) {
            byte = FESC;  // Escaped FESC becomes literal FESC
        }
        // If neither, treat as literal byte (invalid escape, but we'll accept it)
    } else if (byte == FESC) {
        escapeNext = true;  // Next byte is escaped
        return;
    }
    
    // Prevent buffer overflow attacks - discard frame if too large
    if (rxBufferIndex >= SERIAL_BUFFER_SIZE) {
        resetRxBuffer();
        inFrame = false;
        return;
    }
    
    // Store command byte (first byte identifies frame type)
    if (rxBufferIndex == 0) {
        rxBuffer[rxBufferIndex++] = byte;
        return;
    }
    
    // Handle different command types based on first byte
    // Most commands are simple (single parameter), but DATA, SETHARDWARE, 
    // and GETHARDWARE commands carry variable-length data payloads
    if (rxBufferIndex == 1) {
        uint8_t cmd = rxBuffer[0] & 0x0F;  // Mask to get command (lower 4 bits)
        
        // Simple commands: TXDELAY, PERSISTENCE, SLOTTIME, TXTAIL, FULLDUPLEX
        // These are single-byte parameters, so we can handle them immediately
        if (cmd != CMD_DATA && cmd != CMD_SETHARDWARE && cmd != CMD_GETHARDWARE) {
            handleCommand(cmd, byte);
            resetRxBuffer();
            inFrame = false;
            return;
        }
    }
    
    // Continue accumulating frame data for multi-byte commands
    rxBuffer[rxBufferIndex++] = byte;
}

void KISSProtocol::handleCommand(uint8_t cmd, uint8_t value) {
    // KISS Protocol Compatibility Layer:
    // Traditional KISS was designed for VHF/UHF FM transceivers with different timing requirements.
    // LoRa radios have fundamentally different characteristics:
    // - Instant transmission (no PTT delay needed)
    // - Channel Activity Detection (CAD) instead of Carrier Sense Multiple Access (CSMA)
    // - No squelch tail to wait for
    // - Half-duplex only operation
    //
    // We accept these commands for compatibility with existing KISS clients,
    // but they don't affect LoRa operation. Values are stored for potential
    // future use or for responding to queries.
    
    switch (cmd) {
        case CMD_TXDELAY:
            // VHF/UHF FM: Delay between PTT assertion and data transmission
            // LoRa: Not needed - transmission begins immediately
            txDelay = value;
            break;
            
        case CMD_P:
            // VHF/UHF FM: Persistence for CSMA/CA (probability of transmission attempt)
            // LoRa: Not used - CAD (Clear Channel Assessment) is used instead
            persistence = value;
            break;
            
        case CMD_SLOTTIME:
            // VHF/UHF FM: Slot time for CSMA/CA backoff algorithm
            // LoRa: Not used - CAD provides deterministic channel assessment
            slotTime = value;
            break;
            
        case CMD_TXTAIL:
            // VHF/UHF FM: Delay after transmission to allow squelch tail to clear
            // LoRa: Not applicable - no squelch circuits in digital modulation
            txTail = value;
            break;
            
        case CMD_FULLDUPLEX:
            // VHF/UHF FM: Some transceivers support full-duplex operation
            // LoRa: SX1262 is half-duplex only (single antenna shared for TX/RX)
            fullDuplex = (value != 0);
            break;
            
        case CMD_SETHARDWARE:
            // Hardware-specific configuration commands (LoRa parameters)
            // Handled separately with full frame data in main.cpp
            break;
            
        case CMD_RETURN:
            // Exit KISS mode command (not implemented - always in KISS mode)
            break;
            
        default:
            // Unknown command - silently ignore for compatibility
            break;
    }
}

bool KISSProtocol::handleHardwareCommand(const uint8_t* data, size_t length) {
    if (length < 2) {
        return false;  // Need at least command byte and subcommand
    }
    
    // data[0] is the KISS command (0x06), data[1] is the subcommand
    uint8_t subCmd = data[1];
    
    // This will be called from main.cpp with access to the radio object
    // Return true if radio reconfiguration is needed
    return true;  // Handled in main loop
}

bool KISSProtocol::handleHardwareQuery(const uint8_t* data, size_t length) {
    if (length < 2) {
        return false;  // Need at least command byte and subcommand
    }
    
    // data[0] is the KISS command (0x07), data[1] is the subcommand
    // This will be fully handled in main.cpp with access to radio and board objects
    return true;  // Command recognized
}

void KISSProtocol::sendFrame(const uint8_t* data, size_t length) {
    // Send data frame using KISS protocol framing
    // Frame format: FEND + CMD_DATA + [escaped data] + FEND
    // CMD_DATA (0x00) indicates this is a data frame for transmission
    
    Serial.write(FEND);           // Start frame delimiter
    Serial.write(CMD_DATA);       // Command byte (data frame, port 0)
    
    // Apply KISS byte stuffing to escape special characters
    for (size_t i = 0; i < length; i++) {
        if (data[i] == FEND) {
            // Escape FEND with FESC + TFEND sequence
            Serial.write(FESC);
            Serial.write(TFEND);
        } else if (data[i] == FESC) {
            // Escape FESC with FESC + TFESC sequence
            Serial.write(FESC);
            Serial.write(TFESC);
        } else {
            // Send literal byte
            Serial.write(data[i]);
        }
    }
    
    Serial.write(FEND);  // End frame delimiter
}

void KISSProtocol::sendCommand(uint8_t cmd, const uint8_t* data, size_t length) {
    // Send command/response frame using KISS protocol framing
    // Frame format: FEND + command + [escaped data] + FEND
    // Used for SETHARDWARE responses and GETHARDWARE queries
    
    Serial.write(FEND);     // Start frame delimiter
    Serial.write(cmd);      // Command byte (e.g., CMD_SETHARDWARE)
    
    // Apply KISS byte stuffing to escape special characters
    for (size_t i = 0; i < length; i++) {
        if (data[i] == FEND) {
            Serial.write(FESC);
            Serial.write(TFEND);
        } else if (data[i] == FESC) {
            Serial.write(FESC);
            Serial.write(TFESC);
        } else {
            Serial.write(data[i]);
        }
    }
    
    Serial.write(FEND);  // End frame delimiter
}

bool KISSProtocol::hasFrame() {
    return frameReady;
}

uint8_t* KISSProtocol::getFrame() {
    // Return the whole frame including command byte
    return rxBuffer;
}

size_t KISSProtocol::getFrameLength() {
    // Return full length including command byte
    return rxBufferIndex;
}

void KISSProtocol::clearFrame() {
    resetRxBuffer();
    frameReady = false;
}

void KISSProtocol::resetRxBuffer() {
    rxBufferIndex = 0;
    escapeNext = false;
}
