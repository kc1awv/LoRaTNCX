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
    serialBuffer = "";
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
    
    // Connect radio to command system for hardware integration
    commandSystem.setRadio(&radio);
    Serial.println("✓ Command system radio integration enabled");
    
    Serial.println("\n=== TNC Ready ===");
    Serial.println("Starting in Command mode...");
    Serial.println("Type HELP for available commands");
    Serial.println("Type KISS to enter KISS mode");
    Serial.println(configManager.getConfigStatus());
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

    // Handle incoming serial data (could be commands or KISS)
    handleIncomingSerial();

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

void TNCManager::handleIncomingSerial()
{
    // Read available serial data
    while (Serial.available())
    {
        uint8_t byte = Serial.read();
        
        // Handle special initialization sequences before KISS auto-detection
        if (commandSystem.getCurrentMode() != TNCMode::KISS_MODE)
        {
            // Handle common TNC initialization sequences
            static uint8_t initState = 0;
            static uint32_t lastInitByte = 0;
            
            // Reset init state if too much time has passed
            if (millis() - lastInitByte > 1000)
            {
                initState = 0;
            }
            lastInitByte = millis();
            
            // State machine for ESC @ k CR sequence
            bool skipNormalProcessing = false;
            
            switch (initState)
            {
                case 0: // Waiting for ESC or KISS
                    if (byte == 0x1B) // ESC
                    {
                        initState = 1;
                        skipNormalProcessing = true;
                    }
                    else if (byte == 0xC0) // KISS frame start
                    {
                        // Auto-switch to KISS mode
                        commandSystem.setMode(TNCMode::KISS_MODE);
                        serialBuffer = "";
                        initState = 0;
                    }
                    break;
                    
                case 1: // Got ESC, expecting @
                    if (byte == 0x40) // @
                    {
                        initState = 2;
                        skipNormalProcessing = true;
                    }
                    else
                    {
                        initState = 0; // Reset on unexpected byte
                    }
                    break;
                    
                case 2: // Got ESC @, expecting k
                    if (byte == 0x6B) // k
                    {
                        initState = 3;
                        skipNormalProcessing = true;
                    }
                    else
                    {
                        initState = 0; // Reset on unexpected byte
                    }
                    break;
                    
                case 3: // Got ESC @ k, expecting CR
                    if (byte == 0x0D) // CR
                    {
                        // Complete "ESC @ k CR" sequence - enter KISS mode command
                        Serial.println("Entering KISS mode via ESC@k command");
                        commandSystem.setMode(TNCMode::KISS_MODE);
                        serialBuffer = "";
                        initState = 0;
                        return;
                    }
                    else
                    {
                        initState = 0; // Reset on unexpected byte
                    }
                    break;
            }
            
            // If we're processing an init sequence, skip normal command handling
            if (skipNormalProcessing)
            {
                continue;
            }
        }
        
        if (commandSystem.getCurrentMode() == TNCMode::KISS_MODE)
        {
            // Check for ESC character to exit KISS mode (TAPR TNC2 standard)
            if (byte == 0x1B) // ESC
            {
                commandSystem.setMode(TNCMode::COMMAND_MODE);
                Serial.println("Exiting KISS mode - returned to command mode");
                serialBuffer = "";
                return;
            }
            
            // In KISS mode, process bytes directly through KISS protocol
            // The KISS protocol will handle frame assembly and parsing
            if (kiss.processByte(byte))
            {
                handleIncomingKISS();
            }
            
            // Check if KISS protocol requested exit (CMD_RETURN 0xFF)
            if (kiss.isExitRequested())
            {
                commandSystem.setMode(TNCMode::COMMAND_MODE);
                kiss.clearExitRequest();
                Serial.println("Exited KISS mode via CMD_RETURN - returned to command mode");
                serialBuffer = "";
                return;
            }
        }
        else
        {
            // In command mode, handle line-by-line text commands
            char c = (char)byte;
            
            if (c == '\r' || c == '\n')
            {
                if (serialBuffer.length() > 0)
                {
                    // Process command
                    commandSystem.processCommand(serialBuffer);
                    serialBuffer = "";
                }
                else
                {
                    // Empty line, show prompt
                    commandSystem.sendPrompt();
                }
            }
            else if (c >= ' ')  // Printable characters
            {
                serialBuffer += c;
            }
            else if (c == 8 || c == 127)  // Backspace or DEL
            {
                if (serialBuffer.length() > 0)
                {
                    serialBuffer.remove(serialBuffer.length() - 1);
                }
            }
        }
    }
}

void TNCManager::handleIncomingKISS()
{
    // Process incoming serial data for KISS frames
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

void TNCManager::handleIncomingRadio()
{
    // Check for incoming LoRa packets
    if (radio.available())
    {
        uint8_t buffer[512];
        size_t length = radio.receive(buffer, sizeof(buffer));

        if (length > 0)
        {
            // Convert to string for processing
            String packet = "";
            for (size_t i = 0; i < length; i++)
            {
                packet += (char)buffer[i];
            }
            
            // Get signal quality
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();
            
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
            Serial.print(rssi, 1);
            Serial.print(" dBm, SNR: ");
            Serial.print(snr, 1);
            Serial.println(" dB)");

            // Process packet for connection management and protocol handling
            commandSystem.processReceivedPacket(packet, rssi, snr);

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