#include <Arduino.h>
#include "HardwareConfig.h"
#include "LoRaRadio.h"
#include "TNCManager.h"

LoRaRadio loraRadio;
TNCManager tncManager;

void setup()
{
    // Initialize serial communication at KISS standard baud rate
    Serial.begin(KISS_BAUD_RATE);
    while (!Serial) delay(10);

    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LED_OFF);

    // Initialize power control
    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, POWER_ON);
    
    // Initialize LoRa radio subsystem first
    Serial.println("Starting LoRa radio initialization...");
    if (!loraRadio.begin()) {
        Serial.println("WARNING: Failed to initialize LoRa radio!");
        Serial.println("Check hardware connections and configuration");
        Serial.println("Continuing without LoRa radio for debugging...");
        // Flash LED a few times to indicate warning
        for (int i = 0; i < 5; i++) {
            digitalWrite(STATUS_LED_PIN, LED_ON);
            delay(200);
            digitalWrite(STATUS_LED_PIN, LED_OFF);
            delay(200);
        }
    } else {
        Serial.println("LoRa radio initialized successfully!");
    }
    
    // Initialize TNC Manager
    if (!tncManager.begin()) {
        Serial.println("FATAL: Failed to initialize TNC Manager!");
        while (1) {
            // Blink LED rapidly to indicate error
            digitalWrite(STATUS_LED_PIN, LED_ON);
            delay(50);
            digitalWrite(STATUS_LED_PIN, LED_OFF);
            delay(50);
        }
    }
    
    // Give TNC Manager reference to LoRa radio
    tncManager.setLoRaRadio(&loraRadio);
    
    Serial.println();
    Serial.println("=== LoRaTNCX v2.0 - TAPR TNC-2 Compatible ===");
    Serial.println("Hardware: Heltec WiFi LoRa 32 V4 (ESP32-S3)");
    Serial.println("LoRa Chip: SX1262, RadioLib: v7.4.0");
    Serial.println("Type HELP for command list");
    Serial.println();
    Serial.print("cmd:");
    
    // System is now ready - TNC Manager will handle the interface
}

void loop()
{
    
    // Main TNC processing loop
    tncManager.update();
    
    // Blink status LED to show system is alive
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > STATUS_LED_BLINK)
    {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? LED_ON : LED_OFF);
        lastBlink = millis();
    }

    // Small delay to prevent overwhelming the system
    delay(10);
}