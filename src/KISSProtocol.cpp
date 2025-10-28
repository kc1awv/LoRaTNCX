/**
 * @file KISSProtocol.cpp
 * @brief KISS (Keep It Simple, Stupid) protocol implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "KISSProtocol.h"

KISSProtocol::KISSProtocol() :
    txDelay(KISS_DEFAULT_TXDELAY),
    persistence(KISS_DEFAULT_PERSISTENCE),
    slotTime(KISS_DEFAULT_SLOTTIME),
    txTail(KISS_DEFAULT_TXTAIL),
    fullDuplex(KISS_DEFAULT_FULLDUPLEX),
    rxBufferIndex(0),
    inEscapeMode(false),
    frameComplete(false),
    completeFrameLength(0)
{
}

bool KISSProtocol::begin() {
    Serial.println("Initializing KISS Protocol...");
    resetBuffer();
    
    Serial.println("KISS Protocol initialized with defaults:");
    printParameters();
    
    return true;
}

bool KISSProtocol::encodeFrame(const uint8_t* data, size_t dataLen, uint8_t port, uint8_t command, uint8_t* output, size_t* outputLen) {
    if (!data || !output || !outputLen || dataLen == 0) {
        return false;
    }
    
    size_t outputIndex = 0;
    size_t maxOutput = *outputLen;
    
    // Start with FEND
    if (outputIndex >= maxOutput) return false;
    output[outputIndex++] = KISS_FEND;
    
    // Add command/port byte
    if (outputIndex >= maxOutput) return false;
    output[outputIndex++] = ((port & 0x0F) << 4) | (command & 0x0F);
    
    // Escape and add data
    for (size_t i = 0; i < dataLen; i++) {
        uint8_t byte = data[i];
        
        if (byte == KISS_FEND) {
            if (outputIndex + 1 >= maxOutput) return false;
            output[outputIndex++] = KISS_FESC;
            output[outputIndex++] = KISS_TFEND;
        } else if (byte == KISS_FESC) {
            if (outputIndex + 1 >= maxOutput) return false;
            output[outputIndex++] = KISS_FESC;
            output[outputIndex++] = KISS_TFESC;
        } else {
            if (outputIndex >= maxOutput) return false;
            output[outputIndex++] = byte;
        }
    }
    
    // End with FEND
    if (outputIndex >= maxOutput) return false;
    output[outputIndex++] = KISS_FEND;
    
    *outputLen = outputIndex;
    return true;
}

bool KISSProtocol::decodeFrame(const uint8_t* input, size_t inputLen, uint8_t* data, size_t* dataLen, uint8_t* port, uint8_t* command) {
    if (!input || !data || !dataLen || !port || !command || inputLen < 3) {
        return false;
    }
    
    // Check for proper FEND framing
    if (input[0] != KISS_FEND || input[inputLen - 1] != KISS_FEND) {
        return false;
    }
    
    // Extract command/port byte
    uint8_t cmdPortByte = input[1];
    *port = (cmdPortByte & KISS_PORT_MASK) >> 4;
    *command = cmdPortByte & KISS_CMD_MASK;
    
    // Unescape the data portion
    size_t dataIndex = 0;
    bool escapeNext = false;
    
    for (size_t i = 2; i < inputLen - 1; i++) {  // Skip FEND at start and end
        uint8_t byte = input[i];
        
        if (escapeNext) {
            if (byte == KISS_TFEND) {
                if (dataIndex >= *dataLen) return false;
                data[dataIndex++] = KISS_FEND;
            } else if (byte == KISS_TFESC) {
                if (dataIndex >= *dataLen) return false;
                data[dataIndex++] = KISS_FESC;
            } else {
                // Invalid escape sequence - ignore and continue
                Serial.printf("KISS: Invalid escape sequence 0x%02X\n", byte);
            }
            escapeNext = false;
        } else if (byte == KISS_FESC) {
            escapeNext = true;
        } else {
            if (dataIndex >= *dataLen) return false;
            data[dataIndex++] = byte;
        }
    }
    
    *dataLen = dataIndex;
    return true;
}

bool KISSProtocol::encodeDataFrame(const uint8_t* data, size_t dataLen, uint8_t port, uint8_t* output, size_t* outputLen) {
    return encodeFrame(data, dataLen, port, KISS_CMD_DATA, output, outputLen);
}

bool KISSProtocol::decodeDataFrame(const uint8_t* input, size_t inputLen, uint8_t* data, size_t* dataLen, uint8_t* port) {
    uint8_t command;
    if (!decodeFrame(input, inputLen, data, dataLen, port, &command)) {
        return false;
    }
    
    return (command == KISS_CMD_DATA);
}

bool KISSProtocol::encodeCommandFrame(uint8_t command, uint8_t parameter, uint8_t port, uint8_t* output, size_t* outputLen) {
    return encodeFrame(&parameter, 1, port, command, output, outputLen);
}

bool KISSProtocol::processCommand(uint8_t command, uint8_t parameter, uint8_t port) {
    Serial.printf("KISS: Processing command 0x%02X, param=0x%02X, port=%d\n", command, parameter, port);
    
    switch (command) {
        case KISS_CMD_TXDELAY:
            txDelay = parameter;
            Serial.printf("KISS: TXDELAY set to %d (%.1f seconds)\n", txDelay, txDelay * 0.01);
            break;
            
        case KISS_CMD_P:
            persistence = parameter;
            Serial.printf("KISS: Persistence set to %d (p=%.3f)\n", persistence, (persistence + 1) / 256.0);
            break;
            
        case KISS_CMD_SLOTTIME:
            slotTime = parameter;
            Serial.printf("KISS: SlotTime set to %d (%.1f seconds)\n", slotTime, slotTime * 0.01);
            break;
            
        case KISS_CMD_TXTAIL:
            txTail = parameter;
            Serial.printf("KISS: TXTAIL set to %d (%.1f seconds) - obsolete parameter\n", txTail, txTail * 0.01);
            break;
            
        case KISS_CMD_FULLDUPLEX:
            fullDuplex = (parameter != 0);
            Serial.printf("KISS: Full Duplex set to %s\n", fullDuplex ? "ON" : "OFF");
            break;
            
        case KISS_CMD_SETHARDWARE:
            Serial.printf("KISS: Hardware command 0x%02X - implementation specific\n", parameter);
            // Hardware-specific implementation would go here
            break;
            
        case KISS_CMD_RETURN:
            Serial.println("KISS: Return to command mode requested");
            // This should trigger exit from KISS mode - handled by higher level
            return false;  // Signal mode change needed
            
        default:
            Serial.printf("KISS: Unknown command 0x%02X ignored\n", command);
            break;
    }
    
    return true;
}

void KISSProtocol::processInputStream(uint8_t byte) {
    if (byte == KISS_FEND) {
        if (rxBufferIndex > 0) {
            // End of frame
            completeFrameLength = rxBufferIndex;
            frameComplete = true;
        }
        // Reset for next frame (handles back-to-back FENDs)
        rxBufferIndex = 0;
        inEscapeMode = false;
        return;
    }
    
    if (inEscapeMode) {
        if (byte == KISS_TFEND) {
            addByteToBuffer(KISS_FEND);
        } else if (byte == KISS_TFESC) {
            addByteToBuffer(KISS_FESC);
        } else {
            // Invalid escape sequence - ignore
            Serial.printf("KISS: Invalid escape byte 0x%02X\n", byte);
        }
        inEscapeMode = false;
    } else if (byte == KISS_FESC) {
        inEscapeMode = true;
    } else {
        addByteToBuffer(byte);
    }
}

bool KISSProtocol::hasCompleteFrame() {
    return frameComplete;
}

bool KISSProtocol::getCompleteFrame(uint8_t* data, size_t* dataLen, uint8_t* port, uint8_t* command) {
    if (!frameComplete || completeFrameLength < 1) {
        return false;
    }
    
    // Extract command/port from first byte
    uint8_t cmdPortByte = rxBuffer[0];
    *port = (cmdPortByte & KISS_PORT_MASK) >> 4;
    *command = cmdPortByte & KISS_CMD_MASK;
    
    // Copy remaining data
    size_t availableData = completeFrameLength - 1;
    if (availableData > *dataLen) {
        availableData = *dataLen;
    }
    
    memcpy(data, &rxBuffer[1], availableData);
    *dataLen = availableData;
    
    // Reset frame complete flag
    frameComplete = false;
    completeFrameLength = 0;
    
    return true;
}

void KISSProtocol::printParameters() {
    Serial.println("=== KISS Parameters ===");
    Serial.printf("TXDELAY:    %d (%.1f seconds)\n", txDelay, txDelay * 0.01);
    Serial.printf("Persistence: %d (p=%.3f)\n", persistence, (persistence + 1) / 256.0);
    Serial.printf("SlotTime:   %d (%.1f seconds)\n", slotTime, slotTime * 0.01);
    Serial.printf("TXTAIL:     %d (%.1f seconds) [obsolete]\n", txTail, txTail * 0.01);
    Serial.printf("Full Duplex: %s\n", fullDuplex ? "ON" : "OFF");
    Serial.println("========================");
}

void KISSProtocol::addByteToBuffer(uint8_t byte) {
    if (rxBufferIndex < KISS_RX_BUFFER_SIZE - 1) {
        rxBuffer[rxBufferIndex++] = byte;
    } else {
        // Buffer overflow - reset
        Serial.println("KISS: RX buffer overflow, resetting");
        resetBuffer();
    }
}

void KISSProtocol::resetBuffer() {
    rxBufferIndex = 0;
    inEscapeMode = false;
    frameComplete = false;
    completeFrameLength = 0;
}

size_t KISSProtocol::escapeData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t maxOutputLen) {
    size_t outputIndex = 0;
    
    for (size_t i = 0; i < inputLen && outputIndex < maxOutputLen - 1; i++) {
        uint8_t byte = input[i];
        
        if (byte == KISS_FEND) {
            if (outputIndex + 1 >= maxOutputLen) break;
            output[outputIndex++] = KISS_FESC;
            output[outputIndex++] = KISS_TFEND;
        } else if (byte == KISS_FESC) {
            if (outputIndex + 1 >= maxOutputLen) break;
            output[outputIndex++] = KISS_FESC;
            output[outputIndex++] = KISS_TFESC;
        } else {
            output[outputIndex++] = byte;
        }
    }
    
    return outputIndex;
}

size_t KISSProtocol::unescapeData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t maxOutputLen) {
    size_t outputIndex = 0;
    bool escapeNext = false;
    
    for (size_t i = 0; i < inputLen && outputIndex < maxOutputLen; i++) {
        uint8_t byte = input[i];
        
        if (escapeNext) {
            if (byte == KISS_TFEND) {
                output[outputIndex++] = KISS_FEND;
            } else if (byte == KISS_TFESC) {
                output[outputIndex++] = KISS_FESC;
            }
            escapeNext = false;
        } else if (byte == KISS_FESC) {
            escapeNext = true;
        } else {
            output[outputIndex++] = byte;
        }
    }
    
    return outputIndex;
}