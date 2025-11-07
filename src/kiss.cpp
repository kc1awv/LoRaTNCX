#include "kiss.h"

KISSProtocol::KISSProtocol() 
    : rxBufferIndex(0), frameReady(false), escapeNext(false), inFrame(false),
      txDelay(50), persistence(63), slotTime(10), txTail(10), fullDuplex(false) {
}

void KISSProtocol::processSerialByte(uint8_t byte) {
    // Handle FEND
    if (byte == FEND) {
        if (inFrame && rxBufferIndex > 0) {
            // End of frame
            frameReady = true;
            inFrame = false;
        } else {
            // Start of new frame
            resetRxBuffer();
            inFrame = true;
        }
        return;
    }
    
    if (!inFrame) {
        return; // Ignore data outside of frames
    }
    
    // Handle escape sequences
    if (escapeNext) {
        escapeNext = false;
        if (byte == TFEND) {
            byte = FEND;
        } else if (byte == TFESC) {
            byte = FESC;
        }
    } else if (byte == FESC) {
        escapeNext = true;
        return;
    }
    
    // Check for buffer overflow
    if (rxBufferIndex >= SERIAL_BUFFER_SIZE) {
        resetRxBuffer();
        inFrame = false;
        return;
    }
    
    // First byte is the command/port
    if (rxBufferIndex == 0) {
        rxBuffer[rxBufferIndex++] = byte;
        return;
    }
    
    // For simple single-parameter commands (not SETHARDWARE or DATA)
    if (rxBufferIndex == 1) {
        uint8_t cmd = rxBuffer[0] & 0x0F;
        
        if (cmd != CMD_DATA && cmd != CMD_SETHARDWARE) {
            // Simple command with single parameter
            handleCommand(cmd, byte);
            resetRxBuffer();
            inFrame = false;
            return;
        }
    }
    
    // Continue building frame for DATA and SETHARDWARE commands
    rxBuffer[rxBufferIndex++] = byte;
}

void KISSProtocol::handleCommand(uint8_t cmd, uint8_t value) {
    // Accept KISS commands for compatibility, but store them as ignored
    // These parameters are specific to VHF/UHF FM operation and don't apply to LoRa
    switch (cmd) {
        case CMD_TXDELAY:
            txDelay = value;  // Stored but not used - LoRa has instant TX
            break;
        case CMD_P:
            persistence = value;  // Stored but not used - LoRa uses CAD not CSMA
            break;
        case CMD_SLOTTIME:
            slotTime = value;  // Stored but not used - LoRa uses CAD not CSMA
            break;
        case CMD_TXTAIL:
            txTail = value;  // Stored but not used - no squelch tail in LoRa
            break;
        case CMD_FULLDUPLEX:
            fullDuplex = (value != 0);  // Stored but not used - SX1262 is half-duplex
            break;
        case CMD_SETHARDWARE:
            // Hardware commands need full frame data, handled separately
            break;
        case CMD_RETURN:
            // Exit KISS mode (not implemented)
            break;
        default:
            // Unknown command, ignore
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

void KISSProtocol::sendFrame(const uint8_t* data, size_t length) {
    Serial.write(FEND);
    Serial.write(CMD_DATA); // Port 0, data frame
    
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
    
    Serial.write(FEND);
}

void KISSProtocol::sendCommand(uint8_t cmd, const uint8_t* data, size_t length) {
    Serial.write(FEND);
    Serial.write(cmd); // Command byte (e.g., CMD_SETHARDWARE)
    
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
    
    Serial.write(FEND);
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
