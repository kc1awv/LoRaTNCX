/**
 * @file KISSProtocol.cpp
 * @brief KISS protocol implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "KISSProtocol.h"

bool KISSProtocol::begin()
{
    Serial.println("Initializing KISS protocol...");

    // Initialize state
    rxIndex = 0;
    frameInProgress = false;
    escapeNext = false;
    lastFrameLength = 0;
    frameAvailable = false;

    // Initialize statistics
    framesReceived = 0;
    framesSent = 0;
    errors = 0;

    // Initialize TNC parameters with defaults
    txDelay = 30;          // 300ms default TX delay
    persistence = 63;      // ~25% default persistence
    slotTime = 10;         // 100ms default slot time
    txTail = 5;            // 50ms default TX tail
    fullDuplex = false;    // Half duplex by default
    exitRequested = false; // No exit request initially

    Serial.println("✓ KISS protocol initialized");
    Serial.println("Default parameters:");
    Serial.printf("  TX Delay: %d (%.1f ms)\n", txDelay, txDelay * 10.0);
    Serial.printf("  Persistence: %d (%.1f%%)\n", persistence, (persistence / 255.0) * 100.0);
    Serial.printf("  Slot Time: %d (%.1f ms)\n", slotTime, slotTime * 10.0);
    Serial.printf("  TX Tail: %d (%.1f ms)\n", txTail, txTail * 10.0);
    Serial.printf("  Full Duplex: %s\n", fullDuplex ? "Yes" : "No");

    return true;
}

bool KISSProtocol::processIncoming()
{
    bool frameComplete = false;

    while (Serial.available())
    {
        uint8_t byte = Serial.read();

        if (byte == FEND)
        {
            if (frameInProgress && rxIndex > 0)
            {
                // End of frame - process it
                processFrame(rxBuffer, rxIndex);
                frameComplete = true;
            }
            // Start new frame
            resetRxBuffer();
            frameInProgress = true;
            escapeNext = false;
        }
        else if (frameInProgress)
        {
            if (escapeNext)
            {
                // Handle escaped characters
                if (byte == TFEND)
                {
                    byte = FEND;
                }
                else if (byte == TFESC)
                {
                    byte = FESC;
                }
                else
                {
                    // Invalid escape sequence
                    errors++;
                    resetRxBuffer();
                    frameInProgress = false;
                    continue;
                }
                escapeNext = false;
            }
            else if (byte == FESC)
            {
                // Escape next character
                escapeNext = true;
                continue;
            }

            // Add byte to buffer if there's room
            if (rxIndex < KISS_RX_BUFFER_SIZE - 1)
            {
                rxBuffer[rxIndex++] = byte;
            }
            else
            {
                // Buffer overflow
                errors++;
                resetRxBuffer();
                frameInProgress = false;
            }
        }
    }

    return frameComplete;
}

bool KISSProtocol::processByte(uint8_t byte)
{
    bool frameComplete = false;

    if (byte == FEND)
    {
        if (frameInProgress && rxIndex > 0)
        {
            // End of frame - process it
            processFrame(rxBuffer, rxIndex);
            frameComplete = true;
        }
        // Start new frame
        resetRxBuffer();
        frameInProgress = true;
        escapeNext = false;
    }
    else if (frameInProgress)
    {
        if (escapeNext)
        {
            // Handle escaped characters
            if (byte == TFEND)
            {
                byte = FEND;
            }
            else if (byte == TFESC)
            {
                byte = FESC;
            }
            else
            {
                // Invalid escape sequence
                errors++;
                resetRxBuffer();
                frameInProgress = false;
                return false;
            }
            escapeNext = false;
        }
        else if (byte == FESC)
        {
            // Escape next character
            escapeNext = true;
            return false;
        }

        // Add byte to buffer if there's room
        if (rxIndex < KISS_RX_BUFFER_SIZE - 1)
        {
            rxBuffer[rxIndex++] = byte;
        }
        else
        {
            // Buffer overflow
            errors++;
            resetRxBuffer();
            frameInProgress = false;
        }
    }

    return frameComplete;
}

bool KISSProtocol::available()
{
    return frameAvailable;
}

size_t KISSProtocol::getFrame(uint8_t *buffer, size_t maxLength)
{
    if (!frameAvailable)
    {
        return 0;
    }

    size_t copyLength = min(lastFrameLength, maxLength);
    memcpy(buffer, lastFrame, copyLength);
    frameAvailable = false;

    return copyLength;
}

bool KISSProtocol::sendFrame(const uint8_t *data, size_t length)
{
    if (length == 0)
    {
        return false;
    }

    // Send frame start
    Serial.write(FEND);

    // Send data with escaping
    for (size_t i = 0; i < length; i++)
    {
        sendEscaped(data[i]);
    }

    // Send frame end
    Serial.write(FEND);
    Serial.flush();

    framesSent++;
    return true;
}

bool KISSProtocol::sendData(const uint8_t *data, size_t length)
{
    if (length == 0)
    {
        return false;
    }

    // Send frame start
    Serial.write(FEND);

    // Send data command (0x00)
    Serial.write(CMD_DATA);

    // Send data with escaping
    for (size_t i = 0; i < length; i++)
    {
        sendEscaped(data[i]);
    }

    // Send frame end
    Serial.write(FEND);
    Serial.flush();

    framesSent++;
    return true;
}

bool KISSProtocol::processCommand(uint8_t command, uint8_t parameter)
{
    bool handled = true;

    switch (command)
    {
    case CMD_TXDELAY:
        txDelay = parameter;
        // Silent operation - no messages in KISS mode
        break;

    case CMD_P:
        persistence = parameter;
        // Silent operation - no messages in KISS mode
        break;

    case CMD_SLOTTIME:
        slotTime = parameter;
        // Silent operation - no messages in KISS mode
        break;

    case CMD_TXTAIL:
        txTail = parameter;
        // Silent operation - no messages in KISS mode
        break;

    case CMD_FULLDUPLEX:
        fullDuplex = (parameter != 0);
        // Silent operation - no messages in KISS mode
        break;

    case CMD_SETHARDWARE:
        // Hardware-specific commands can be implemented here
        // Silent operation - no messages in KISS mode
        break;

    case CMD_RETURN:
        // Silent operation - no messages in KISS mode
        exitRequested = true;
        break;

    default:
        // Silent operation - no messages in KISS mode
        handled = false;
        errors++;
        break;
    }

    return handled;
}

String KISSProtocol::getStatus()
{
    String status = "KISS Protocol Status:\n";
    status += "  Frames Received: " + String(framesReceived) + "\n";
    status += "  Frames Sent: " + String(framesSent) + "\n";
    status += "  Errors: " + String(errors) + "\n";
    status += "  TX Delay: " + String(txDelay) + " (" + String(txDelay * 10.0, 1) + " ms)\n";
    status += "  Persistence: " + String(persistence) + " (" + String((persistence / 255.0) * 100.0, 1) + "%)\n";
    status += "  Slot Time: " + String(slotTime) + " (" + String(slotTime * 10.0, 1) + " ms)\n";
    status += "  TX Tail: " + String(txTail) + " (" + String(txTail * 10.0, 1) + " ms)\n";
    status += "  Full Duplex: " + String(fullDuplex ? "Yes" : "No");
    return status;
}

void KISSProtocol::processFrame(const uint8_t *frame, size_t length)
{
    if (length == 0)
    {
        return;
    }

    uint8_t command = frame[0];

    if (command == CMD_DATA)
    {
        // Data frame - store for application
        if (length > 1)
        {
            memcpy(lastFrame, frame + 1, length - 1); // Skip command byte
            lastFrameLength = length - 1;
            frameAvailable = true;
            framesReceived++;
        }
    }
    else
    {
        // Command frame
        uint8_t parameter = (length > 1) ? frame[1] : 0;
        processCommand(command, parameter);
    }
}

void KISSProtocol::sendEscaped(uint8_t byte)
{
    if (byte == FEND)
    {
        Serial.write(FESC);
        Serial.write(TFEND);
    }
    else if (byte == FESC)
    {
        Serial.write(FESC);
        Serial.write(TFESC);
    }
    else
    {
        Serial.write(byte);
    }
}

void KISSProtocol::resetRxBuffer()
{
    rxIndex = 0;
    frameInProgress = false;
    escapeNext = false;
}

bool KISSProtocol::isExitRequested()
{
    return exitRequested;
}

void KISSProtocol::clearExitRequest()
{
    exitRequested = false;
}