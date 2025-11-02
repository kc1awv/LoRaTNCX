#include "KissProtocol.h"

KissProtocol::KissProtocol()
{
    state = KISS_STATE_IDLE;
    rxBufferPos = 0;
    
    // TAPR TNC-2 compatible default values
    config.txDelay = KISS_DEFAULT_TXDELAY;    // 50 = 500ms
    config.persistence = KISS_DEFAULT_P;      // 63 ≈ 25%
    config.slotTime = KISS_DEFAULT_SLOTTIME;  // 10 = 100ms
    config.txTail = KISS_DEFAULT_TXTAIL;      // 2 = 20ms
    config.fullDuplex = false;                // Half duplex default
    
    framesReceived = 0;
    framesSent = 0;
    frameErrors = 0;
    onDataFrameCallback = nullptr;
    onCommandCallback = nullptr;
}

void KissProtocol::begin() 
{
    // TAPR TNC-2 compatible initialization
    // Reset to default state
    reset();
    
    // Initialize with default parameters
    config.txDelay = KISS_DEFAULT_TXDELAY;
    config.persistence = KISS_DEFAULT_P;
    config.slotTime = KISS_DEFAULT_SLOTTIME;
    config.txTail = KISS_DEFAULT_TXTAIL;
    config.fullDuplex = false;
    
    // TNC-2 sends no startup message in KISS mode
    // The host application detects KISS mode by successfully 
    // communicating with KISS commands
}
void KissProtocol::reset()
{
    state = KISS_STATE_IDLE;
    rxBufferPos = 0;
}
void KissProtocol::setTxDelay(uint8_t delay) { config.txDelay = delay; }
void KissProtocol::setPersistence(uint8_t p) { config.persistence = p; }
void KissProtocol::setSlotTime(uint8_t slotTime) { config.slotTime = slotTime; }
void KissProtocol::setTxTail(uint8_t tail) { config.txTail = tail; }
void KissProtocol::setFullDuplex(bool enable) { config.fullDuplex = enable; }
void KissProtocol::processByte(uint8_t byte)
{
    switch (state)
    {
    case KISS_STATE_IDLE:
        if (byte == KISS_FEND)
        {
            // Start of frame
            state = KISS_STATE_IN_FRAME;
            rxBufferPos = 0;
        }
        // Ignore all other bytes when idle
        break;

    case KISS_STATE_IN_FRAME:
        if (byte == KISS_FEND)
        {
            // End of frame - process the frame if we have data
            if (rxBufferPos > 0)
            {
                processReceivedFrame();
            }
            state = KISS_STATE_IDLE;
            rxBufferPos = 0;
        }
        else if (byte == KISS_FESC)
        {
            // Escape sequence follows
            state = KISS_STATE_ESCAPED;
        }
        else
        {
            // Regular data byte
            if (rxBufferPos < KISS_MAX_FRAME_SIZE - 1)
            {
                rxBuffer[rxBufferPos++] = byte;
            }
            else
            {
                // Buffer overflow - discard frame
                frameErrors++;
                state = KISS_STATE_IDLE;
                rxBufferPos = 0;
            }
        }
        break;

    case KISS_STATE_ESCAPED:
        if (byte == KISS_TFEND)
        {
            // Escaped FEND
            if (rxBufferPos < KISS_MAX_FRAME_SIZE - 1)
            {
                rxBuffer[rxBufferPos++] = KISS_FEND;
            }
            else
            {
                frameErrors++;
                state = KISS_STATE_IDLE;
                rxBufferPos = 0;
                break;
            }
        }
        else if (byte == KISS_TFESC)
        {
            // Escaped FESC
            if (rxBufferPos < KISS_MAX_FRAME_SIZE - 1)
            {
                rxBuffer[rxBufferPos++] = KISS_FESC;
            }
            else
            {
                frameErrors++;
                state = KISS_STATE_IDLE;
                rxBufferPos = 0;
                break;
            }
        }
        else
        {
            // Invalid escape sequence
            frameErrors++;
            state = KISS_STATE_IDLE;
            rxBufferPos = 0;
            break;
        }
        state = KISS_STATE_IN_FRAME;
        break;
    }
}
void KissProtocol::processReceivedFrame()
{
    // Validate frame has minimum length
    if (rxBufferPos < 1)
    {
        frameErrors++;
        return;
    }

    // First byte is command/port
    uint8_t commandByte = rxBuffer[0];
    uint8_t port = (commandByte >> 4) & 0x0F;  // Upper 4 bits = port (usually 0)
    uint8_t command = commandByte & 0x0F;       // Lower 4 bits = command

    // Validate port number (single-port TNC)
    if (port != 0)
    {
        frameErrors++;
        return;
    }

    // Validate command is in valid range
    if (command > KISS_CMD_SETHARDWARE && command != KISS_CMD_RETURN)
    {
        frameErrors++;
        return;
    }

    framesReceived++;

    if (command == KISS_CMD_DATA)
    {
        // Data frame - must have payload
        if (rxBufferPos > 1)
        {
            uint16_t dataLength = rxBufferPos - 1;
            
            // Validate data length is reasonable
            if (dataLength <= KISS_MAX_DATA_PAYLOAD && onDataFrameCallback)
            {
                onDataFrameCallback(&rxBuffer[1], dataLength);
            }
            else if (dataLength > KISS_MAX_DATA_PAYLOAD)
            {
                frameErrors++;
            }
        }
        // Empty data frame is valid (zero-length packet)
        else if (onDataFrameCallback)
        {
            onDataFrameCallback(nullptr, 0);
        }
    }
    else
    {
        // Command frame - must have parameter byte
        if (rxBufferPos >= 2)
        {
            uint8_t parameter = rxBuffer[1];
            processCommand(command, parameter);
        }
        else
        {
            // Commands require a parameter byte
            frameErrors++;
        }
    }
}

void KissProtocol::processCommand(uint8_t command, uint8_t parameter)
{
    bool validCommand = true;
    
    switch (command)
    {
    case KISS_CMD_TXDELAY:
        // TX delay in 10ms units (0-255 = 0-2550ms)
        config.txDelay = parameter;
        break;

    case KISS_CMD_P:
        // Persistence parameter (0-255, where 255 = always transmit)
        config.persistence = parameter;
        break;

    case KISS_CMD_SLOTTIME:
        // Slot time in 10ms units (0-255 = 0-2550ms)
        config.slotTime = parameter;
        break;

    case KISS_CMD_TXTAIL:
        // TX tail time in 10ms units (0-255 = 0-2550ms)
        config.txTail = parameter;
        break;

    case KISS_CMD_FULLDUPLEX:
        // Full duplex mode: 0 = half duplex, non-zero = full duplex
        config.fullDuplex = (parameter != 0);
        break;

    case KISS_CMD_SETHARDWARE:
        // Hardware-specific commands
        // For TNC-2 compatibility, we acknowledge but don't implement
        // specific hardware commands
        break;

    case KISS_CMD_RETURN:
        // Exit KISS mode - this should be handled at the TNC level
        break;

    default:
        // Unknown command - increment error counter but don't respond
        // TNC-2 behavior: silently ignore unknown commands
        frameErrors++;
        validCommand = false;
        break;
    }

    // Call command callback if registered and command was valid
    if (validCommand && onCommandCallback)
    {
        onCommandCallback(command, parameter);
    }
}
void KissProtocol::sendDataFrame(uint8_t *data, uint16_t length)
{
    // Validate input parameters
    if (length > KISS_MAX_DATA_PAYLOAD || (length > 0 && data == nullptr))
    {
        frameErrors++;
        return;
    }

    // Build frame: FEND + CMD + DATA + FEND
    uint16_t framePos = 0;
    
    // Ensure we have space for minimum frame
    if (KISS_MAX_FRAME_SIZE < 3)  // FEND + CMD + FEND
    {
        frameErrors++;
        return;
    }
    
    // Start frame
    txBuffer[framePos++] = KISS_FEND;
    
    // Command byte (port 0, data command)
    txBuffer[framePos++] = KISS_CMD_DATA;
    
    // Add data with escaping
    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t byte = data[i];
        
        // Check for buffer space before adding bytes
        // Need space for: current byte(s) + end FEND + potential escape
        if (framePos >= KISS_MAX_FRAME_SIZE - 3)
        {
            frameErrors++;
            return;
        }
        
        if (byte == KISS_FEND)
        {
            txBuffer[framePos++] = KISS_FESC;
            txBuffer[framePos++] = KISS_TFEND;
        }
        else if (byte == KISS_FESC)
        {
            txBuffer[framePos++] = KISS_FESC;
            txBuffer[framePos++] = KISS_TFESC;
        }
        else
        {
            txBuffer[framePos++] = byte;
        }
    }
    
    // End frame
    txBuffer[framePos++] = KISS_FEND;
    
    // Send to serial
    sendToSerial(txBuffer, framePos);
    framesSent++;
}
void KissProtocol::onDataFrame(void (*callback)(uint8_t *data, uint16_t length)) { onDataFrameCallback = callback; }
void KissProtocol::onCommand(void (*callback)(uint8_t command, uint8_t parameter)) { onCommandCallback = callback; }
void KissProtocol::handleSerialInput()
{
    while (Serial.available() > 0)
    {
        uint8_t byte = Serial.read();
        processByte(byte);
    }
}
void KissProtocol::sendToSerial(uint8_t *data, uint16_t length)
{
    Serial.write(data, length);
    Serial.flush();  // Ensure data is sent immediately
}
void KissProtocol::printStatistics() 
{
    Serial.println("[KISS] Statistics:");
    Serial.printf("  Frames Received: %u\n", framesReceived);
    Serial.printf("  Frames Sent: %u\n", framesSent);
    Serial.printf("  Frame Errors: %u\n", frameErrors);
    Serial.printf("  Current State: %s\n", 
                  (state == KISS_STATE_IDLE) ? "IDLE" :
                  (state == KISS_STATE_IN_FRAME) ? "IN_FRAME" : "ESCAPED");
    Serial.printf("  RX Buffer Position: %u\n", rxBufferPos);
}
void KissProtocol::clearStatistics()
{
    framesReceived = 0;
    framesSent = 0;
    frameErrors = 0;
}

void KissProtocol::printConfiguration() 
{
    Serial.println("[KISS] Configuration:");
    Serial.printf("  TX Delay: %d (= %d ms)\n", config.txDelay, config.txDelay * 10);
    Serial.printf("  Persistence: %d (= %.1f%%)\n", config.persistence, (config.persistence / 255.0) * 100.0);
    Serial.printf("  Slot Time: %d (= %d ms)\n", config.slotTime, config.slotTime * 10);
    Serial.printf("  TX Tail: %d (= %d ms)\n", config.txTail, config.txTail * 10);
    Serial.printf("  Full Duplex: %s\n", config.fullDuplex ? "Enabled" : "Disabled");
}

bool KissProtocol::isKissFrame(uint8_t *data, uint16_t length)
{
    // Minimum KISS frame: FEND + CMD + FEND = 3 bytes
    if (length < 3)
    {
        return false;
    }
    
    // Must start and end with FEND
    if (data[0] != KISS_FEND || data[length - 1] != KISS_FEND)
    {
        return false;
    }
    
    // Check command byte (second byte)
    uint8_t command = data[1] & 0x0F;
    uint8_t port = (data[1] >> 4) & 0x0F;
    
    // Port should be 0 for single-port TNC
    if (port != 0)
    {
        return false;
    }
    
    // Valid command values
    return (command <= KISS_CMD_SETHARDWARE || command == KISS_CMD_RETURN);
}

String KissProtocol::commandToString(uint8_t command)
{
    switch (command)
    {
    case KISS_CMD_DATA:
        return "DATA";
    case KISS_CMD_TXDELAY:
        return "TXDELAY";
    case KISS_CMD_P:
        return "PERSISTENCE";
    case KISS_CMD_SLOTTIME:
        return "SLOTTIME";
    case KISS_CMD_TXTAIL:
        return "TXTAIL";
    case KISS_CMD_FULLDUPLEX:
        return "FULLDUPLEX";
    case KISS_CMD_SETHARDWARE:
        return "SETHARDWARE";
    case KISS_CMD_RETURN:
        return "RETURN";
    default:
        return "UNKNOWN";
    }
}

void KissProtocol::sendFrame(uint8_t command, uint8_t *data, uint16_t length)
{
    if (length > KISS_MAX_DATA_PAYLOAD)
    {
        frameErrors++;
        return;
    }

    // Build frame: FEND + CMD + DATA + FEND
    uint16_t framePos = 0;
    
    // Start frame
    txBuffer[framePos++] = KISS_FEND;
    
    // Command byte (port 0 + command)
    txBuffer[framePos++] = command & 0x0F;
    
    // Add data with escaping
    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t byte = data[i];
        if (byte == KISS_FEND)
        {
            txBuffer[framePos++] = KISS_FESC;
            txBuffer[framePos++] = KISS_TFEND;
        }
        else if (byte == KISS_FESC)
        {
            txBuffer[framePos++] = KISS_FESC;
            txBuffer[framePos++] = KISS_TFESC;
        }
        else
        {
            txBuffer[framePos++] = byte;
        }
        
        // Check for buffer overflow
        if (framePos >= KISS_MAX_FRAME_SIZE - 2)
        {
            frameErrors++;
            return;
        }
    }
    
    // End frame
    txBuffer[framePos++] = KISS_FEND;
    
    // Send to serial
    sendToSerial(txBuffer, framePos);
    framesSent++;
}

uint16_t KissProtocol::escapeData(uint8_t *input, uint16_t inputLength, uint8_t *output)
{
    uint16_t outPos = 0;
    
    for (uint16_t i = 0; i < inputLength; i++)
    {
        uint8_t byte = input[i];
        if (byte == KISS_FEND)
        {
            output[outPos++] = KISS_FESC;
            output[outPos++] = KISS_TFEND;
        }
        else if (byte == KISS_FESC)
        {
            output[outPos++] = KISS_FESC;
            output[outPos++] = KISS_TFESC;
        }
        else
        {
            output[outPos++] = byte;
        }
    }
    
    return outPos;
}

uint16_t KissProtocol::unescapeData(uint8_t *input, uint16_t inputLength, uint8_t *output)
{
    uint16_t outPos = 0;
    bool escaped = false;
    
    for (uint16_t i = 0; i < inputLength; i++)
    {
        uint8_t byte = input[i];
        
        if (escaped)
        {
            if (byte == KISS_TFEND)
            {
                output[outPos++] = KISS_FEND;
            }
            else if (byte == KISS_TFESC)
            {
                output[outPos++] = KISS_FESC;
            }
            else
            {
                // Invalid escape sequence - return error
                return 0;
            }
            escaped = false;
        }
        else if (byte == KISS_FESC)
        {
            escaped = true;
        }
        else
        {
            output[outPos++] = byte;
        }
    }
    
    return escaped ? 0 : outPos; // Error if we ended in escaped state
}

void KissProtocol::sendCommand(uint8_t command, uint8_t parameter)
{
    uint8_t data[1] = {parameter};
    sendFrame(command, data, 1);
}
