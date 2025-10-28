/**
 * @file main.cpp
 * @brief Main entry point for LoRaTNCX project
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include <Arduino.h>
#include "HardwareConfig.h"

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("LoRaTNCX starting...");
    Serial.println("Hardware: Heltec WiFi LoRa 32 V4");
    
    // TODO: Initialize LoRa radio
    // TODO: Initialize KISS protocol
    // TODO: Initialize display (if enabled)
    
    Serial.println("Setup complete!");
}

void loop() {
    // TODO: Main application loop
    delay(1000);
}