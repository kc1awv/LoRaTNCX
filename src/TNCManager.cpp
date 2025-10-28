/**
 * @file TNCManager.cpp
 * @brief TNC Manager implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "TNCManager.h"

// Static instance for callbacks
TNCManager* TNCManager::instance = nullptr;

TNCManager::TNCManager() :
    loraRadio(nullptr),
    currentMode(TNC_MODE_COMMAND),
    initialized(false),
    txQueueHead(0),
    txQueueTail(0),
    txQueueCount(0),
    rxQueueHead(0),
    rxQueueTail(0),
    rxQueueCount(0),
    packetsTransmitted(0),
    packetsReceived(0),
    kissFramesProcessed(0),
    commandsProcessed(0),
    lastStatsTime(0),
    lastTxTime(0),
    lastRxCheck(0),
    channelBusy(false)
{
    instance = this;
}

bool TNCManager::begin() {
    Serial.println("Initializing TNC Manager...");
    
    // Initialize KISS protocol
    if (!kissProtocol.begin()) {
        Serial.println("Failed to initialize KISS protocol");
        return false;
    }
    
    // Initialize command parser
    if (!commandParser.begin()) {
        Serial.println("Failed to initialize command parser");
        return false;
    }
    
    // Set up command parser callbacks
    commandParser.onKissModeEnter = onKissModeEnterCallback;
    commandParser.onKissModeExit = onKissModeExitCallback;
    commandParser.onRestart = onRestartCallback;
    
    currentMode = TNC_MODE_COMMAND;
    initialized = true;
    lastStatsTime = millis();
    
    Serial.println("TNC Manager initialized successfully");
    return true;
}

void TNCManager::update() {
    if (!initialized) return;
    
    // Process serial input
    processSerialInput();
    
    // Process based on current mode
    if (currentMode == TNC_MODE_COMMAND) {
        processCommandMode();
    } else if (currentMode == TNC_MODE_KISS) {
        processKissMode();
    }
    
    // Always process radio packets and transmission queue
    processRadioPackets();
    processTransmitQueue();
}

TNCMode TNCManager::getCurrentMode() const {
    return currentMode;
}

void TNCManager::enterKissMode() {
    if (currentMode == TNC_MODE_KISS) return;
    
    Serial.println("TNC: Entering KISS mode - \"silence is compliance\"");
    
    // Check if this is due to KISS ON + RESTART sequence
    if (commandParser.getKissFlag()) {
        Serial.println("Status LED will flash three times");
        flashKissLEDs();
    }
    
    currentMode = TNC_MODE_KISS;
    
    // From this point forward, only KISS frames should be sent to host
}

void TNCManager::exitKissMode() {
    if (currentMode == TNC_MODE_COMMAND) return;
    
    currentMode = TNC_MODE_COMMAND;
    commandParser.setMode(TNC_MODE_COMMAND);
}

void TNCManager::restart() {
    Serial.println("TNC: Restarting to command mode...");
    currentMode = TNC_MODE_COMMAND;
    
    // Clear queues
    txQueueHead = txQueueTail = txQueueCount = 0;
    rxQueueHead = rxQueueTail = rxQueueCount = 0;
    
    // Reset statistics
    packetsTransmitted = 0;
    packetsReceived = 0;
    kissFramesProcessed = 0;
    commandsProcessed = 0;
    
    delay(100);
    commandParser.setMode(TNC_MODE_COMMAND);
}

bool TNCManager::transmitPacket(const uint8_t* data, size_t length, uint8_t port) {
    if (!data || length == 0 || length > TNC_MAX_PACKET_SIZE) {
        return false;
    }
    
    TNCPacket packet;
    memcpy(packet.data, data, length);
    packet.length = length;
    packet.port = port;
    packet.timestamp = millis();
    
    return enqueueTxPacket(packet);
}

bool TNCManager::hasReceivedPacket() {
    return rxQueueCount > 0;
}

bool TNCManager::getReceivedPacket(uint8_t* data, size_t* length, uint8_t* port) {
    TNCPacket packet;
    if (!dequeueRxPacket(packet)) {
        return false;
    }
    
    if (packet.length > *length) {
        return false;  // Buffer too small
    }
    
    memcpy(data, packet.data, packet.length);
    *length = packet.length;
    *port = packet.port;
    
    return true;
}

void TNCManager::printStatistics() {
    unsigned long uptime = millis();
    unsigned long uptimeSeconds = uptime / 1000;
    
    Serial.println();
    Serial.println("=== TNC Statistics ===");
    Serial.printf("Uptime: %lu seconds\n", uptimeSeconds);
    Serial.printf("Current Mode: %s\n", (currentMode == TNC_MODE_KISS) ? "KISS" : "Command");
    Serial.printf("Packets TX: %lu\n", packetsTransmitted);
    Serial.printf("Packets RX: %lu\n", packetsReceived);
    Serial.printf("KISS Frames: %lu\n", kissFramesProcessed);
    Serial.printf("Commands: %lu\n", commandsProcessed);
    Serial.printf("TX Queue: %d/%d\n", txQueueCount, TNC_TX_QUEUE_SIZE);
    Serial.printf("RX Queue: %d/%d\n", rxQueueCount, TNC_TX_QUEUE_SIZE);
    Serial.printf("Free RAM: %d bytes\n", ESP.getFreeHeap());
    Serial.println("=====================");
    Serial.println();
}

void TNCManager::processCommandMode() {
    // Process any pending commands
    if (commandParser.hasCommand()) {
        commandParser.processCommand();
        commandsProcessed++;
    }
}

void TNCManager::processKissMode() {
    // In KISS mode, process KISS frames from serial input
    if (kissProtocol.hasCompleteFrame()) {
        uint8_t command, port;
        size_t dataLength = TNC_MAX_PACKET_SIZE;
        
        if (kissProtocol.getCompleteFrame(processBuffer, &dataLength, &port, &command)) {
            processKissFrame(processBuffer, dataLength, port, command);
            kissFramesProcessed++;
        }
    }
}

void TNCManager::processSerialInput() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        
        if (currentMode == TNC_MODE_COMMAND) {
            // In command mode, process as ASCII commands
            commandParser.processInputStream((char)byte);
        } else if (currentMode == TNC_MODE_KISS) {
            // In KISS mode, process as KISS frames
            kissProtocol.processInputStream(byte);
        }
    }
}

void TNCManager::processRadioPackets() {
    if (!loraRadio) return;
    
    // Check for received packets (non-blocking)
    uint8_t buffer[TNC_MAX_PACKET_SIZE];
    int packetLength = loraRadio->receive(buffer, sizeof(buffer));
    
    if (packetLength > 0) {
        // We received a packet!
        packetsReceived++;
        
        if (currentMode == TNC_MODE_KISS) {
            // In KISS mode, send as KISS data frame to host
            sendKissDataFrame(buffer, packetLength);
        } else {
            // In command mode, display packet info
            Serial.printf("\n[RX] Received %d bytes, RSSI: %.1f dBm, SNR: %.1f dB\n",
                         packetLength, loraRadio->getLastRSSI(), loraRadio->getLastSNR());
            
            // Show packet contents (first 32 bytes)
            Serial.print("[RX] Data: ");
            for (int i = 0; i < min(packetLength, 32); i++) {
                Serial.printf("%02X ", buffer[i]);
            }
            if (packetLength > 32) Serial.print("...");
            Serial.println();
            Serial.print("cmd:");  // Restore command prompt
        }
        
        // Also add to RX queue for host applications
        TNCPacket packet;
        memcpy(packet.data, buffer, packetLength);
        packet.length = packetLength;
        packet.port = 0;
        packet.timestamp = millis();
        enqueueRxPacket(packet);
    }
}

void TNCManager::processTransmitQueue() {
    if (!loraRadio || txQueueCount == 0) return;
    
    // Implement basic p-persistence CSMA
    if (!shouldTransmit()) {
        return;
    }
    
    TNCPacket packet;
    if (dequeueTxPacket(packet)) {
        // Apply TXDELAY
        unsigned long txDelay = kissProtocol.getTxDelay() * 10;  // Convert to ms
        if (millis() - lastTxTime < txDelay) {
            // Re-queue packet and wait
            enqueueTxPacket(packet);
            return;
        }
        
        // Transmit the packet
        int result = loraRadio->transmit(packet.data, packet.length);
        
        if (result == RADIOLIB_ERR_NONE) {
            packetsTransmitted++;
            lastTxTime = millis();
            
            if (currentMode == TNC_MODE_COMMAND) {
                Serial.printf("\n[TX] Transmitted %d bytes successfully\n", packet.length);
                Serial.print("cmd:");  // Restore prompt
            }
        } else {
            if (currentMode == TNC_MODE_COMMAND) {
                Serial.printf("\n[TX] Transmission failed with code: %d\n", result);
                Serial.print("cmd:");  // Restore prompt
            }
        }
    }
}

bool TNCManager::enqueueTxPacket(const TNCPacket& packet) {
    if (txQueueCount >= TNC_TX_QUEUE_SIZE) {
        return false;  // Queue full
    }
    
    txQueue[txQueueTail] = packet;
    txQueueTail = (txQueueTail + 1) % TNC_TX_QUEUE_SIZE;
    txQueueCount++;
    
    return true;
}

bool TNCManager::dequeueTxPacket(TNCPacket& packet) {
    if (txQueueCount == 0) {
        return false;  // Queue empty
    }
    
    packet = txQueue[txQueueHead];
    txQueueHead = (txQueueHead + 1) % TNC_TX_QUEUE_SIZE;
    txQueueCount--;
    
    return true;
}

bool TNCManager::enqueueRxPacket(const TNCPacket& packet) {
    if (rxQueueCount >= TNC_TX_QUEUE_SIZE) {
        return false;  // Queue full
    }
    
    rxQueue[rxQueueTail] = packet;
    rxQueueTail = (rxQueueTail + 1) % TNC_TX_QUEUE_SIZE;
    rxQueueCount++;
    
    return true;
}

bool TNCManager::dequeueRxPacket(TNCPacket& packet) {
    if (rxQueueCount == 0) {
        return false;  // Queue empty
    }
    
    packet = rxQueue[rxQueueHead];
    rxQueueHead = (rxQueueHead + 1) % TNC_TX_QUEUE_SIZE;
    rxQueueCount--;
    
    return true;
}

void TNCManager::processKissFrame(const uint8_t* data, size_t length, uint8_t port, uint8_t command) {
    switch (command) {
        case KISS_CMD_DATA:
            // Data frame - queue for transmission
            if (length > 0) {
                transmitPacket(data, length, port);
            }
            break;
            
        case KISS_CMD_TXDELAY:
        case KISS_CMD_P:
        case KISS_CMD_SLOTTIME:
        case KISS_CMD_TXTAIL:
        case KISS_CMD_FULLDUPLEX:
        case KISS_CMD_SETHARDWARE:
            // Parameter commands
            if (length > 0) {
                kissProtocol.processCommand(command, data[0], port);
            }
            break;
            
        case KISS_CMD_RETURN:
            // Exit KISS mode - return to normal TNC operation
            Serial.println();  // New line before returning
            Serial.println("Exiting KISS mode - returning to normal TNC operation");
            exitKissMode();
            break;
            
        default:
            // Unknown command - ignore (per KISS spec)
            break;
    }
}

void TNCManager::sendKissDataFrame(const uint8_t* data, size_t length, uint8_t port) {
    size_t kissLength = sizeof(kissBuffer);
    
    if (kissProtocol.encodeDataFrame(data, length, port, kissBuffer, &kissLength)) {
        // Send KISS frame to host
        Serial.write(kissBuffer, kissLength);
    }
}

bool TNCManager::shouldTransmit() {
    // Simple p-persistence implementation
    uint8_t persistence = kissProtocol.getPersistence();
    uint8_t slotTime = kissProtocol.getSlotTime();
    
    // Generate random number 0-255
    uint8_t random = esp_random() & 0xFF;
    
    if (random <= persistence) {
        return true;  // Transmit immediately
    } else {
        // Wait one slot time
        static unsigned long lastSlotWait = 0;
        unsigned long slotTimeMs = slotTime * 10;
        
        if (millis() - lastSlotWait >= slotTimeMs) {
            lastSlotWait = millis();
            return true;  // Try again next time
        }
        
        return false;  // Still waiting
    }
}

void TNCManager::waitSlotTime() {
    uint8_t slotTime = kissProtocol.getSlotTime();
    delay(slotTime * 10);  // Convert to milliseconds
}

void TNCManager::flashKissLEDs() {
    // Flash status LED three times as per TAPR TNC-2 specification
    // (Original TNC-2 flashed CON/STA LEDs, we use the single status LED)
    
    for (int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, LED_ON);
        delay(200);
        digitalWrite(STATUS_LED_PIN, LED_OFF);
        delay(200);
    }
    
    Serial.println("KISS mode activated (status LED flashed 3 times)");
}

// Static callback implementations
void TNCManager::onKissModeEnterCallback() {
    if (instance) {
        instance->enterKissMode();
    }
}

void TNCManager::onKissModeExitCallback() {
    if (instance) {
        instance->exitKissMode();
    }
}

void TNCManager::onRestartCallback() {
    if (instance) {
        instance->restart();
    }
}