/**
 * @file main.cpp
 * @brief LoRa Ping/Pong Test for Heltec V4 devices
 * @author LoRaTNCX Project
 * @date October 28, 2025
 * 
 * Simple ping/pong test to verify LoRa communication between two devices.
 * Uncomment one of the defines below to set device role:
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "HardwareConfig.h"

// ==========================================
// DEVICE ROLE CONFIGURATION
// ==========================================
// Uncomment ONE of these lines to set device role:
// #define PING_DEVICE     // This device sends pings
#define PONG_DEVICE  // This device responds with pongs

// If neither is defined, device will auto-detect based on button press at startup

// ==========================================
// LORA CONFIGURATION
// ==========================================
#define FREQUENCY       915.0   // MHz (adjust for your region)
#define BANDWIDTH       125.0   // kHz
#define SPREADING_FACTOR   7    // 7-12
#define CODING_RATE        5    // 5-8
#define OUTPUT_POWER      14    // dBm (max 22 for SX1262)
#define PREAMBLE_LENGTH    8    // symbols
#define SYNC_WORD       0x12    // sync word

// ==========================================
// TIMING CONFIGURATION
// ==========================================
#define PING_INTERVAL   2000    // ms between pings
#define RX_TIMEOUT      5000    // ms to wait for response
#define LED_BLINK_TIME   200    // ms for LED indication

// ==========================================
// GLOBAL VARIABLES
// ==========================================
SX1262 radio = new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

bool isPingDevice = true;
uint32_t pingCount = 0;
uint32_t pongCount = 0;
uint32_t lastPingTime = 0;
uint32_t lastLedTime = 0;
bool ledState = false;
bool radioInitialized = false;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================
void initializeRadio();
void sendPing();
void sendPong(uint32_t pingId);
void handleReceivedMessage();
void blinkLED();
void printStatus();

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);  // Give serial time to initialize
    
    Serial.println();
    Serial.println("=== LoRa Ping/Pong Test ===");
    Serial.println("Hardware: Heltec WiFi LoRa 32 V4");
    Serial.println("LoRa Chip: SX1262");
    
    // Initialize built-in LED
    pinMode(35, OUTPUT);  // LED_BUILTIN = 35 for V4
    digitalWrite(35, LOW);
    
    // Determine device role
    #ifdef PING_DEVICE
        isPingDevice = true;
        Serial.println("Device Role: PING (sender)");
    #elif defined(PONG_DEVICE)
        isPingDevice = false;
        Serial.println("Device Role: PONG (responder)");
    #else
        // Auto-detect based on user input
        Serial.println("Press and hold USER button within 3 seconds to set as PONG device...");
        Serial.println("Otherwise will default to PING device.");
        
        // Wait for button press (assuming USER button on pin 0)
        pinMode(0, INPUT_PULLUP);
        unsigned long startTime = millis();
        bool buttonPressed = false;
        
        while (millis() - startTime < 3000) {
            if (digitalRead(0) == LOW) {
                buttonPressed = true;
                break;
            }
            delay(50);
        }
        
        isPingDevice = !buttonPressed;
        Serial.print("Device Role: ");
        Serial.println(isPingDevice ? "PING (sender)" : "PONG (responder)");
    #endif
    
    // Initialize LoRa radio
    initializeRadio();
    
    if (radioInitialized) {
        Serial.println("Ready to communicate!");
        Serial.println("----------------------------------------");
        
        if (isPingDevice) {
            Serial.println("Sending pings every 2 seconds...");
        } else {
            Serial.println("Listening for pings...");
        }
    }
}

void loop() {
    if (!radioInitialized) {
        delay(1000);
        return;
    }
    
    if (isPingDevice) {
        // PING device logic
        if (millis() - lastPingTime >= PING_INTERVAL) {
            sendPing();
            lastPingTime = millis();
        }
        
        // Check for pong responses
        handleReceivedMessage();
        
    } else {
        // PONG device logic - just listen for pings
        handleReceivedMessage();
    }
    
    // Blink LED to show activity
    blinkLED();
    
    // Print status every 10 seconds
    static unsigned long lastStatusTime = 0;
    if (millis() - lastStatusTime >= 10000) {
        printStatus();
        lastStatusTime = millis();
    }
    
    delay(10); // Small delay to prevent excessive CPU usage
}

void initializeRadio() {
    Serial.println("Initializing LoRa radio...");
    
    // Print pin configuration for debugging
    Serial.println("Pin Configuration:");
    Serial.print("  SS: "); Serial.println(LORA_SS_PIN);
    Serial.print("  RST: "); Serial.println(LORA_RST_PIN);
    Serial.print("  DIO0: "); Serial.println(LORA_DIO0_PIN);
    Serial.print("  BUSY: "); Serial.println(LORA_BUSY_PIN);
    
    // Initialize power control pins if needed
    pinMode(LORA_PA_POWER_PIN, OUTPUT);
    digitalWrite(LORA_PA_POWER_PIN, HIGH);  // Enable PA power
    Serial.println("PA power enabled");
    
    // Small delay for power stabilization
    delay(100);
    
    // Initialize SPI explicitly
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
    Serial.println("SPI initialized");
    
    // Initialize the radio with basic settings
    int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SYNC_WORD, OUTPUT_POWER, PREAMBLE_LENGTH);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("✓ Radio initialized successfully!");
        radioInitialized = true;
        
        // Print radio configuration
        Serial.println("Radio Configuration:");
        Serial.print("  Frequency: "); Serial.print(FREQUENCY); Serial.println(" MHz");
        Serial.print("  Bandwidth: "); Serial.print(BANDWIDTH); Serial.println(" kHz");
        Serial.print("  Spreading Factor: "); Serial.println(SPREADING_FACTOR);
        Serial.print("  Coding Rate: 4/"); Serial.println(CODING_RATE);
        Serial.print("  Output Power: "); Serial.print(OUTPUT_POWER); Serial.println(" dBm");
        Serial.print("  Sync Word: 0x"); Serial.println(SYNC_WORD, HEX);
        
    } else {
        Serial.print("✗ Radio initialization failed! Error code: ");
        Serial.print(state);
        
        // Decode common error codes
        switch (state) {
            case -2:
                Serial.println(" (RADIOLIB_ERR_CHIP_NOT_FOUND)");
                Serial.println("  Check SPI connections and pin definitions");
                break;
            case -1:
                Serial.println(" (RADIOLIB_ERR_UNKNOWN)");
                break;
            case -13:
                Serial.println(" (RADIOLIB_ERR_INVALID_FREQUENCY)");
                break;
            case -14:
                Serial.println(" (RADIOLIB_ERR_INVALID_BANDWIDTH)");
                break;
            default:
                Serial.println(" (Unknown error)");
                break;
        }
        
        // Try different approaches for debugging
        Serial.println("Debugging steps:");
        Serial.println("1. Check antenna connection");
        Serial.println("2. Verify pin connections");
        Serial.println("3. Check power supply");
        
        // Blink LED rapidly to indicate error
        for (int i = 0; i < 10; i++) {
            digitalWrite(35, HIGH);
            delay(100);
            digitalWrite(35, LOW);
            delay(100);
        }
    }
}

void sendPing() {
    pingCount++;
    String message = "PING:" + String(pingCount) + ":" + String(millis());
    
    Serial.print("Sending ping #"); Serial.print(pingCount); Serial.print("... ");
    
    int state = radio.transmit(message);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("sent!");
        
        // Quick LED flash for successful transmission
        digitalWrite(35, HIGH);
        delay(50);
        digitalWrite(35, LOW);
        
    } else {
        Serial.print("failed! Error code: ");
        Serial.println(state);
    }
}

void sendPong(uint32_t pingId) {
    String message = "PONG:" + String(pingId) + ":" + String(millis());
    
    Serial.print("Sending pong for ping #"); Serial.print(pingId); Serial.print("... ");
    
    int state = radio.transmit(message);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("sent!");
        pongCount++;
        
        // Double LED flash for pong response
        digitalWrite(35, HIGH);
        delay(50);
        digitalWrite(35, LOW);
        delay(50);
        digitalWrite(35, HIGH);
        delay(50);
        digitalWrite(35, LOW);
        
    } else {
        Serial.print("failed! Error code: ");
        Serial.println(state);
    }
}

void handleReceivedMessage() {
    String receivedMessage;
    int state = radio.receive(receivedMessage);
    
    if (state == RADIOLIB_ERR_NONE) {
        // Message received successfully
        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        
        Serial.println("----------------------------------------");
        Serial.print("Received: "); Serial.println(receivedMessage);
        Serial.print("RSSI: "); Serial.print(rssi); Serial.println(" dBm");
        Serial.print("SNR: "); Serial.print(snr); Serial.println(" dB");
        
        // Parse message
        if (receivedMessage.startsWith("PING:")) {
            // Extract ping ID
            int firstColon = receivedMessage.indexOf(':', 5);
            if (firstColon > 0) {
                uint32_t pingId = receivedMessage.substring(5, firstColon).toInt();
                Serial.print("Ping ID: "); Serial.println(pingId);
                
                if (!isPingDevice) {
                    // Send pong response
                    delay(100); // Small delay before responding
                    sendPong(pingId);
                }
            }
            
        } else if (receivedMessage.startsWith("PONG:")) {
            // Extract pong ID and calculate round-trip time
            int firstColon = receivedMessage.indexOf(':', 5);
            int secondColon = receivedMessage.indexOf(':', firstColon + 1);
            
            if (firstColon > 0 && secondColon > 0) {
                uint32_t pongId = receivedMessage.substring(5, firstColon).toInt();
                uint32_t pongTime = receivedMessage.substring(secondColon + 1).toInt();
                uint32_t roundTripTime = millis() - (pongTime - 1000); // Rough calculation
                
                Serial.print("Pong ID: "); Serial.println(pongId);
                Serial.print("Estimated round-trip time: "); Serial.print(roundTripTime); Serial.println(" ms");
                
                if (isPingDevice) {
                    pongCount++;
                }
            }
        }
        
        Serial.println("----------------------------------------");
        
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        // Only print errors that aren't timeouts
        Serial.print("Receive error: ");
        Serial.println(state);
    }
}

void blinkLED() {
    if (millis() - lastLedTime >= 1000) {
        ledState = !ledState;
        digitalWrite(35, ledState);
        lastLedTime = millis();
    }
}

void printStatus() {
    Serial.println("=== STATUS ===");
    Serial.print("Device Role: "); Serial.println(isPingDevice ? "PING" : "PONG");
    Serial.print("Uptime: "); Serial.print(millis() / 1000); Serial.println(" seconds");
    
    if (isPingDevice) {
        Serial.print("Pings sent: "); Serial.println(pingCount);
        Serial.print("Pongs received: "); Serial.println(pongCount);
        if (pingCount > 0) {
            Serial.print("Success rate: "); 
            Serial.print((float)pongCount / pingCount * 100.0, 1); 
            Serial.println("%");
        }
    } else {
        Serial.print("Pongs sent: "); Serial.println(pongCount);
    }
    
    Serial.println("==============");
}