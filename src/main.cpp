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
#include "SystemLogger.h"

// Global instances
TNCManager tnc;
void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Initialize logging system first
    SystemLogger* logger = SystemLogger::getInstance();
    logger->begin();
    
    LOG_BOOT_INFO("LoRaTNCX - LoRa Terminal Node Controller");
    LOG_BOOT_INFO("Based on proven ping/pong communication foundation");
    LOG_BOOT_INFO("System initialization starting...");

    // Initialize TNC - this will handle all subsystem initialization
    if (tnc.begin())
    {
        LOG_BOOT_SUCCESS("TNC initialization successful!");
        LOG_BOOT_INFO("Ready for KISS protocol communication");
    }
    else
    {
        LOG_BOOT_FAILURE("TNC initialization failed!");
        // Critical failure - show on console
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
