/**
 * @file TNCManager.cpp
 * @brief TNC Manager implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "TNCManager.h"

TNCManager::TNCManager() : configManager(&radio)
{
    initialized = false;
    lastStatus = 0;
}

bool TNCManager::begin()
{
    Serial.println("=== LoRaTNCX Initialization ===");

    initialized = false;
    lastStatus = 0;

    // Initialize KISS protocol
    if (!kiss.begin())
    {
        Serial.println("✗ KISS protocol initialization failed");
        return false;
    }

    // Initialize with default amateur radio configuration (balanced)
    Serial.println("✓ KISS protocol initialized");
    Serial.println("Configuring LoRa radio with amateur radio settings...");
    
    if (!configManager.setConfiguration(LoRaConfigPreset::BALANCED))
    {
        Serial.println("✗ LoRa radio configuration failed");
        return false;
    }
    
    Serial.println("✓ LoRa radio configured");
    Serial.println("\n=== TNC Ready ===");
    Serial.println("Entering KISS mode...");
    Serial.println("Connect your packet radio application to this serial port.");
    Serial.println(configManager.getConfigStatus());
    Serial.println("\nConfiguration commands available:");
    Serial.println("  LISTCONFIG - Show all available presets");
    Serial.println("  SETCONFIG <preset> - Change configuration");
    Serial.println("  GETCONFIG - Show current configuration");
    
    Serial.println("===============================");

    initialized = true;
    return true;
}

void TNCManager::update()
{
    if (!initialized)
    {
        return;
    }

    // Handle incoming KISS commands from serial
    handleIncomingKISS();

    // Handle incoming LoRa packets
    handleIncomingRadio();

    // Print periodic status (less frequently in KISS mode)
    if (millis() - lastStatus >= 30000)
    { // Every 30 seconds
        printStatus();
        lastStatus = millis();
    }
    

}

String TNCManager::getStatus()
{
    String status = "=== TNC Status ===\n";
    status += "Initialized: " + String(initialized ? "Yes" : "No") + "\n";
    status += "Uptime: " + String(millis() / 1000) + " seconds\n\n";
    status += configManager.getConfigStatus() + "\n\n";
    status += kiss.getStatus() + "\n\n";
    status += radio.getStatus();
    return status;
}

bool TNCManager::processConfigurationCommand(const String& command)
{
    return configManager.processConfigCommand(command);
}

void TNCManager::handleIncomingKISS()
{
    // Process incoming serial data for KISS frames
    if (kiss.processIncoming())
    {
        // Complete frame received
        uint8_t buffer[512];
        size_t length = kiss.getFrame(buffer, sizeof(buffer));

        if (length > 0)
        {
            // Check if this is a configuration command (text starting with CONFIG, SETCONFIG, LISTCONFIG, or GETCONFIG)
            String frameData = "";
            bool isText = true;
            for (size_t i = 0; i < length; i++) {
                if (buffer[i] >= 32 && buffer[i] < 127) {
                    frameData += (char)buffer[i];
                } else if (buffer[i] != 0) {  // Allow null termination
                    isText = false;
                    break;
                }
            }
            
            if (isText && (frameData.startsWith("CONFIG") || frameData.startsWith("SETCONFIG") || 
                          frameData.startsWith("LISTCONFIG") || frameData.startsWith("GETCONFIG"))) {
                Serial.println("TNC: Processing configuration command: " + frameData);
                processConfigurationCommand(frameData);
                return;  // Don't transmit configuration commands over radio
            }
            
            // Regular data packet - transmit via LoRa
            Serial.print("TNC: Transmitting ");
            Serial.print(length);
            Serial.print(" bytes: ");
            for (size_t i = 0; i < length && i < 20; i++)
            {
                if (buffer[i] >= 32 && buffer[i] < 127)
                {
                    Serial.print((char)buffer[i]);
                }
                else
                {
                    Serial.print('.');
                }
            }
            Serial.println();

            // Transmit packet
            if (radio.transmit(buffer, length))
            {
                Serial.println("TNC: Transmission successful");
            }
            else
            {
                Serial.println("TNC: Transmission failed");
            }
        }
    }
}

void TNCManager::handleIncomingRadio()
{
    // Check for incoming LoRa packets
    if (radio.available())
    {
        uint8_t buffer[512];
        size_t length = radio.receive(buffer, sizeof(buffer));

        if (length > 0)
        {
            Serial.print("TNC: Received ");
            Serial.print(length);
            Serial.print(" bytes via LoRa: ");
            for (size_t i = 0; i < length && i < 20; i++)
            {
                if (buffer[i] >= 32 && buffer[i] < 127)
                {
                    Serial.print((char)buffer[i]);
                }
                else
                {
                    Serial.print('.');
                }
            }
            Serial.print(" (RSSI: ");
            Serial.print(radio.getRSSI(), 1);
            Serial.print(" dBm, SNR: ");
            Serial.print(radio.getSNR(), 1);
            Serial.println(" dB)");

            // Send received packet to host via KISS
            kiss.sendData(buffer, length);
        }
    }
}

void TNCManager::printStatus()
{
    // In KISS mode, minimize status output to avoid interfering with protocol
    // Only print to debug output if available, otherwise skip

    // Could implement a debug output mechanism here
    // For now, silent operation in KISS mode

    // If we want status output, it should go to a separate debug interface
    // or be disabled entirely in production KISS mode
}