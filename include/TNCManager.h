/**
 * @file TNCManager.h
 * @brief TNC Manager class for coordinating all TNC operations
 * @author LoRaTNCX Project
 * @date October 28, 2025
 *
 * The TNC Manager coordinates between the KISS protocol handler,
 * LoRa radio interface, and provides the main TNC functionality.
 */

#ifndef TNC_MANAGER_H
#define TNC_MANAGER_H

#include <Arduino.h>
#include "LoRaRadio.h"
#include "KISSProtocol.h"
#include "ConfigurationManager.h"
#include <TNCCommands.h>

class TNCManager
{
public:
    /**
     * @brief Constructor
     */
    TNCManager();
    /**
     * @brief Initialize the TNC system
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Main update loop - should be called repeatedly
     */
    void update();

    /**
     * @brief Get the current TNC status
     * @return Status string
     */
    String getStatus();

    /**
     * @brief Process configuration commands
     * @param command Configuration command string
     * @return true if command processed successfully
     */
    bool processConfigurationCommand(const String& command);



private:
    LoRaRadio radio;                    // LoRa radio interface
    KISSProtocol kiss;                  // KISS protocol handler
    ConfigurationManager configManager; // Configuration management
    TNCCommands commandSystem;    // Command system interface
    
    bool initialized;
    unsigned long lastStatus;
    String serialBuffer;                // Buffer for incoming serial data

    /**
     * @brief Handle incoming LoRa packets
     */
    void handleIncomingRadio();

    /**
     * @brief Handle incoming KISS commands from serial
     */
    void handleIncomingKISS();

    /**
     * @brief Handle incoming serial data (commands or KISS)
     */
    void handleIncomingSerial();

    /**
     * @brief Print periodic status updates
     */
    void printStatus();
};

#endif // TNC_MANAGER_H