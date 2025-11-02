#include <Arduino.h>
#include "LoRaRadio.h"
#include "LoRaTNC.h"
#include "CommandProcessor.h"

// Global instances
LoRaRadio loraRadio;
LoRaTNC *tnc;
CommandProcessor *cmdProcessor;

// Input buffer for serial commands
String inputBuffer = "";

// LoRa event handlers
void onTxDone() {
    if (tnc) {
        tnc->handleLoRaTransmitDone();
    }
}

void onTxTimeout() {
    if (tnc) {
        tnc->handleLoRaTransmitTimeout();
    }
}

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, float snr) {
    // Forward to TNC for processing
    if (tnc) {
        tnc->handleLoRaReceive(payload, size, rssi, snr);
    }
}

void onRxTimeout() {
    Serial.println("[LoRa] RX timeout occurred");
}

void onRxError() {
    Serial.println("[LoRa] RX error occurred");
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    
    // Initialize LoRa radio
    Serial.println("Initializing LoRa radio...");
    
    // Set up LoRa event handlers
    loraRadio.onTxDone(onTxDone);
    loraRadio.onTxTimeout(onTxTimeout);
    loraRadio.onRxDone(onRxDone);
    loraRadio.onRxTimeout(onRxTimeout);
    loraRadio.onRxError(onRxError);
    
    // Initialize LoRa radio
    if (loraRadio.begin()) {
        Serial.println("LoRa radio initialized successfully!");
        
        // Initialize TNC
        tnc = new LoRaTNC(&loraRadio);
        if (tnc->begin()) {
            Serial.println("TNC initialized successfully!");
        } else {
            Serial.println("Failed to initialize TNC!");
        }
        
        // Initialize command processor (this creates TNC2Config and StationHeard)
        cmdProcessor = new CommandProcessor(&loraRadio, tnc);
        
        // Wire up TNC-2 integration
        tnc->setTNC2Config(cmdProcessor->getTNC2Config());
        tnc->setStationHeard(cmdProcessor->getStationHeard());
        
        // Print header and help
        cmdProcessor->printHeader();
        
        // Start in receive mode
        loraRadio.startReceive();
    } else {
        Serial.println("Failed to initialize LoRa radio!");
        Serial.print("> ");
    }
}

void loop() {
    // Handle LoRa radio events
    loraRadio.handle();
    
    // Handle TNC processing
    if (tnc) {
        tnc->handle();
    }
    
    // Check if data is available on serial port (only in command mode)
    if (tnc && tnc->getMode() == TNC_MODE_COMMAND && Serial.available() > 0) {
        char incomingChar = Serial.read();
        
        // Handle different characters
        if (incomingChar == '\r' || incomingChar == '\n') {
            // End of line - process the command
            // Only process if we have content or this is the first line ending character
            if (inputBuffer.length() > 0 || incomingChar == '\r') {
                if (incomingChar == '\r') {
                    // Echo the carriage return as newline for proper formatting
                    Serial.println();
                }
                
                bool shouldPrintPrompt = cmdProcessor->processCommand(inputBuffer);
                inputBuffer = ""; // Clear the buffer for next command
                
                // Print prompt for next command (unless command handler said not to)
                if (shouldPrintPrompt) {
                    Serial.print("> ");
                }
            }
            // Ignore \n if it immediately follows \r (CRLF sequence)
        }
        else if (incomingChar == '\b' || incomingChar == 127) {
            // Backspace or DEL - remove last character
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                // Echo backspace sequence to terminal
                Serial.print("\b \b");
            }
        }
        else if (incomingChar >= 32 && incomingChar < 127) {
            // Printable character - add to buffer and echo
            inputBuffer += incomingChar;
            Serial.print(incomingChar);
        }
        // Ignore other control characters
    }
}