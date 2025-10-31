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
#include <DisplayManager.h>
#include <BatteryMonitor.h>
#include <GNSSManager.h>

class WebInterfaceManager;

class TNCManager
{
public:
    struct StatusSnapshot
    {
        DisplayManager::StatusData displayStatus;
        String statusText;
    };

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
     * @brief Retrieve the latest status information in structured form.
     */
    StatusSnapshot getStatusSnapshot();

    /**
     * @brief Retrieve the status structure used for display updates.
     */
    DisplayManager::StatusData getDisplayStatusSnapshot();

    /**
     * @brief Process configuration commands
     * @param command Configuration command string
     * @return true if command processed successfully
     */
    bool processConfigurationCommand(const String &command);

    /**
     * @brief Process configuration commands from C-style strings safely.
     */
    bool processConfigurationCommand(const char *command);

    /**
     * @brief Execute a textual TNC command.
     */
    TNCCommandResult executeCommand(const String &command);

    /**
     * @brief Execute a textual TNC command from a C-style string safely.
     */
    TNCCommandResult executeCommand(const char *command);

    /**
     * @brief Retrieve the formatted configuration status text.
     */
    String getConfigurationStatus() const;

    /**
     * @brief Enable or disable the GNSS module
     * @param enable true to enable, false to disable
     * @return true if operation successful
     */
    bool setGNSSEnabled(bool enable);

    /**
     * @brief Get current GNSS enabled status
     * @return true if GNSS is enabled and initialized
     */
    bool isGNSSEnabled() const { return gnssEnabled && gnssInitialised; }

    /**
     * @brief Enable or disable the OLED display
     */
    bool setOLEDEnabled(bool enable);

    /**
     * @brief Query whether the OLED display is active.
     */
    bool isOLEDEnabled() const { return oledEnabled && display.isEnabled(); }

    /**
     * @brief Register the web interface for real-time telemetry callbacks.
     */
    void setWebInterface(WebInterfaceManager *interface);

    // Static callback functions for TNCCommands
    static bool gnssSetEnabledCallback(bool enable);
    static bool gnssGetEnabledCallback();
    static bool oledSetEnabledCallback(bool enable);
    static bool oledGetEnabledCallback();

private:
    static TNCManager* instance; // Static instance for callbacks
    LoRaRadio radio;                    // LoRa radio interface
    KISSProtocol kiss;                  // KISS protocol handler
    ConfigurationManager configManager; // Configuration management
    TNCCommands commandSystem;          // Command system interface
    DisplayManager display;             // OLED display manager
    BatteryMonitor batteryMonitor;      // Battery monitoring helper
    GNSSManager gnss;                   // GNSS module interface

    bool gnssEnabled;
    bool gnssInitialised;
    bool oledEnabled;

    WebInterfaceManager *webInterface;

    bool initialized;
    unsigned long lastStatus;
    String serialBuffer; // Buffer for incoming serial data
    float lastBatteryVoltage;
    uint8_t lastBatteryPercent;
    unsigned long lastBatterySample;

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

    /**
     * @brief Handle the front-panel user button actions.
     */
    void handleUserButton();

    /**
     * @brief Build the data structure used for OLED updates.
     */
    DisplayManager::StatusData buildDisplayStatus();

    /**
     * @brief Execute the hardware power-off procedure.
     */
    void performPowerOff();

    float lastPacketRSSI;
    float lastPacketSNR;
    unsigned long lastPacketTimestamp;
    bool hasRecentPacket;

    bool buttonStableState;
    bool buttonLastReading;
    unsigned long buttonLastChange;
    unsigned long buttonPressStart;
    bool buttonLongPressHandled;

    bool powerOffWarningActive;
    float powerOffProgress;
    bool powerOffInitiated;
    bool powerOffComplete;
};

#endif // TNC_MANAGER_H
