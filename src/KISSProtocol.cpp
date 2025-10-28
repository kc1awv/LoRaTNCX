/**
 * @file KISSProtocol.cpp
 * @brief KISS (Keep It Simple, Stupid) protocol implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include <Arduino.h>
#include "HardwareConfig.h"
#include "KISSProtocol.h"
#include "LoRaRadio.h"

KISSProtocol::KISSProtocol() :
    txDelay(KISS_DEFAULT_TXDELAY),
    persistence(KISS_DEFAULT_PERSISTENCE),
    slotTime(KISS_DEFAULT_SLOTTIME),
    txTail(KISS_DEFAULT_TXTAIL),
    fullDuplex(KISS_DEFAULT_FULLDUPLEX),
    loraRadio(nullptr),
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
    #if DEBUG_KISS_VERBOSE
    Serial.printf("KISS: Processing command 0x%02X, param=0x%02X, port=%d\n", command, parameter, port);
    #endif
    
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
            Serial.printf("KISS: Hardware command parameter 0x%02X\n", parameter);
            // Handle hardware-specific commands (LoRa parameters)
            // Note: For SetHardware, additional data may follow in the frame
            handleSetHardware(parameter, nullptr, 0);  // Basic handling for now
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

bool KISSProtocol::processSetHardwareCommand(const uint8_t* data, size_t length, uint8_t port) {
    #if DEBUG_KISS_VERBOSE
    Serial.printf("KISS: Processing SetHardware command (port=%d, length=%d)\n", port, length);
    #endif
    
    if (length < 1) {
        #if DEBUG_KISS_VERBOSE
        Serial.println("KISS: SetHardware command too short");
        #endif
        return false;
    }
    
    uint8_t parameter = data[0];
    const uint8_t* paramData = (length > 1) ? &data[1] : nullptr;
    size_t paramDataLen = (length > 1) ? length - 1 : 0;
    
    #if DEBUG_KISS_VERBOSE
    Serial.printf("KISS: SetHardware parameter=0x%02X, dataLen=%d\n", parameter, paramDataLen);
    #endif
    
    // Call the existing handleSetHardware method with proper data
    bool result = handleSetHardware(parameter, paramData, paramDataLen);
    
    #if DEBUG_KISS_VERBOSE
    if (result) {
        Serial.println("KISS: SetHardware command successful");
    } else {
        Serial.println("KISS: SetHardware command failed");
    }
    #endif
    
    return result;
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
}bool KISSProtocol::handleSetHardware(uint8_t parameter, const uint8_t* data, size_t dataLen) {
    if (!loraRadio) {
        Serial.println("KISS: No LoRa radio configured for hardware commands");
        return false;
    }
    
    switch (parameter) {
        case LORA_HW_FREQUENCY:
            if (dataLen >= 4) {
                // Frequency in Hz, little-endian format
                uint32_t freqHz = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                float freqMHz = freqHz / 1000000.0f;
                Serial.printf("KISS: Setting frequency to %.3f MHz\n", freqMHz);
                return loraRadio->setFrequency(freqMHz);
            }
            Serial.println("KISS: Invalid frequency data length");
            return false;
            
        case LORA_HW_TX_POWER:
            if (dataLen >= 1) {
                int8_t power = (int8_t)data[0];
                Serial.printf("KISS: Setting TX power to %d dBm\n", power);
                return loraRadio->setTxPower(power);
            }
            Serial.println("KISS: Invalid TX power data length");
            return false;
            
        case LORA_HW_BANDWIDTH:
            if (dataLen >= 1) {
                uint8_t bwIndex = data[0];
                float bandwidth;
                switch (bwIndex) {
                    case LORA_BW_7_8_KHZ:   bandwidth = 7.8f; break;
                    case LORA_BW_10_4_KHZ:  bandwidth = 10.4f; break;
                    case LORA_BW_15_6_KHZ:  bandwidth = 15.6f; break;
                    case LORA_BW_20_8_KHZ:  bandwidth = 20.8f; break;
                    case LORA_BW_31_25_KHZ: bandwidth = 31.25f; break;
                    case LORA_BW_41_7_KHZ:  bandwidth = 41.7f; break;
                    case LORA_BW_62_5_KHZ:  bandwidth = 62.5f; break;
                    case LORA_BW_125_KHZ:   bandwidth = 125.0f; break;
                    case LORA_BW_250_KHZ:   bandwidth = 250.0f; break;
                    case LORA_BW_500_KHZ:   bandwidth = 500.0f; break;
                    default:
                        Serial.printf("KISS: Invalid bandwidth index %d\n", bwIndex);
                        return false;
                }
                Serial.printf("KISS: Setting bandwidth to %.1f kHz\n", bandwidth);
                return loraRadio->setBandwidth(bandwidth);
            }
            Serial.println("KISS: Invalid bandwidth data length");
            return false;
            
        case LORA_HW_SPREADING_FACTOR:
            if (dataLen >= 1) {
                uint8_t sf = data[0];
                if (sf >= 6 && sf <= 12) {
                    Serial.printf("KISS: Setting spreading factor to SF%d\n", sf);
                    return loraRadio->setSpreadingFactor(sf);
                } else {
                    Serial.printf("KISS: Invalid spreading factor %d (must be 6-12)\n", sf);
                    return false;
                }
            }
            Serial.println("KISS: Invalid spreading factor data length");
            return false;
            
        case LORA_HW_CODING_RATE:
            if (dataLen >= 1) {
                uint8_t cr = data[0];
                if (cr >= 5 && cr <= 8) {
                    Serial.printf("KISS: Setting coding rate to 4/%d\n", cr);
                    return loraRadio->setCodingRate(cr);
                } else {
                    Serial.printf("KISS: Invalid coding rate %d (must be 5-8)\n", cr);
                    return false;
                }
            }
            Serial.println("KISS: Invalid coding rate data length");
            return false;
            
        default:
            Serial.printf("KISS: Unknown hardware parameter 0x%02X\n", parameter);
            return false;
    }
}