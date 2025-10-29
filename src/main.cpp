/**
 * @file main.cpp
 * @brief LoRaTNCX - LoRa Terminal Node Controller
 * @author LoRaTNCX Project
 * @date October 28, 2025
 *
 * A TNC implementation using the reliable LoRa communication foundation
 * established in our ping/pong testing.
 *
 * Features:
 * - KISS protocol support for packet radio applications
 * - Reliable LoRa PHY layer with proven PA control
 * - Hardware abstraction for Heltec WiFi LoRa 32 V4
 * - Serial interface for host computer connection
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "HardwareConfig.h"
#include "TNCManager.h"
#include "KISSProtocol.h"
#include "LoRaRadio.h"

// Global instances
TNCManager tnc;
void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("LoRaTNCX - LoRa Terminal Node Controller");
    Serial.println("Based on proven ping/pong communication foundation");
    Serial.println("Initializing...");

    // Initialize TNC - this will handle all subsystem initialization
    if (tnc.begin())
    {
        Serial.println("TNC initialization successful!");
        Serial.println("Ready for KISS protocol communication");
    }
    else
    {
        Serial.println("TNC initialization failed!");
        while (true)
        {
            delay(10);
        }
    }
}

void loop()
{
    // Let the TNC manager handle all operations
    tnc.update();

    // Small delay to prevent overwhelming the loop
    delay(1);
}
