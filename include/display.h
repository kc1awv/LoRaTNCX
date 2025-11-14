#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"

/**
 * @brief Display screen types for different operational modes
 */
enum DisplayScreen {
    SCREEN_BOOT,         ///< Boot/splash screen with version info
    SCREEN_INIT,         ///< Initialization progress screen
    SCREEN_READY,        ///< Ready screen showing system status summary
    SCREEN_WIFI_STARTUP, ///< WiFi initialization progress screen
    SCREEN_STATUS,       ///< Main status screen showing radio configuration
    SCREEN_WIFI,         ///< WiFi connection status and IP addresses
    SCREEN_BATTERY,      ///< Battery voltage and power status
    SCREEN_GNSS,         ///< GNSS position and satellite information
    SCREEN_OFF,          ///< Display off for power saving
    NUM_SCREENS          ///< Total number of screen types
};

/**
 * @brief OLED Display Manager for SSD1306 128x64 display
 *
 * Manages the OLED display content, screen transitions, and user interface.
 * Provides multiple screens for different operational states and status information.
 * Handles button input for screen navigation and power management.
 *
 * Features:
 * - Multiple screen types with automatic transitions
 * - Real-time status updates (radio config, WiFi, battery, GNSS)
 * - Button debouncing and long-press detection
 * - Power management with display off capability
 * - Boot screen with configurable duration
 */
class DisplayManager {
public:
    /**
     * @brief Construct a new Display Manager
     */
    DisplayManager();
    
    /**
     * @brief Initialize the OLED display hardware
     *
     * Sets up I2C communication and configures the U8G2 display library.
     * Must be called before using other display functions.
     */
    void begin();
    
    /**
     * @brief Update display content and handle screen transitions
     *
     * Main display update function that should be called regularly in the main loop.
     * Handles screen timeouts, status updates, and display refreshing.
     */
    void update();
    
    /**
     * @brief Set the active display screen
     *
     * Immediately switches to the specified screen type.
     *
     * @param screen The screen type to display
     */
    void setScreen(DisplayScreen screen);
    
    /**
     * @brief Cycle to the next screen in sequence
     *
     * Advances to the next screen type, wrapping around to the first screen.
     * Skips the boot screen and off screen in normal cycling.
     */
    void nextScreen();
    
    /**
     * @brief Show the boot screen for a specified duration
     *
     * Displays the boot/splash screen, then automatically transitions
     * to the main status screen after the timeout.
     *
     * @param duration_ms Duration to show boot screen in milliseconds (default: 2000)
     */
    void showBootScreen(uint32_t duration_ms = 2000);
    
    /**
     * @brief Update the radio configuration display data
     *
     * Stores the current LoRa radio parameters for display on the status screen.
     *
     * @param freq Carrier frequency in MHz
     * @param bw Signal bandwidth in kHz
     * @param sf Spreading factor (7-12)
     * @param cr Coding rate (5-8)
     * @param pwr Output power in dBm
     * @param sw 16-bit synchronization word
     */
    void setRadioConfig(float freq, float bw, uint8_t sf, uint8_t cr, int8_t pwr, uint16_t sw);
    
    /**
     * @brief Update the battery voltage display data
     *
     * @param voltage Current battery voltage in volts
     */
    void setBatteryVoltage(float voltage);
    
    /**
     * @brief Set the WiFi startup status message
     *
     * @param message Status message to display during WiFi initialization
     */
    void setWiFiStartupMessage(String message);
    
    /**
     * @brief Set the initialization status message
     *
     * @param message Status message to display during system initialization
     */
    void setInitMessage(String message);
    
    /**
     * @brief Set the initialization status with success/fail indicator
     *
     * @param component Name of the component being initialized
     * @param success True if initialization succeeded, false if failed
     */
    void setInitStatus(String component, bool success);
    
    /**
     * @brief Set the ready screen status data
     *
     * @param radioOK True if radio is initialized successfully
     * @param wifiStatus WiFi status: "OFF", "AP", "STA", or "AP+STA"
     * @param gnssOK True if GNSS module is initialized
     * @param gnssFix True if GNSS has a valid fix
     * @param boardType Board type string ("V3" or "V4")
     */
    void setReadyStatus(bool radioOK, String wifiStatus, bool gnssOK, bool gnssFix, String boardType);
    
    /**
     * @brief Update the WiFi status display data
     *
     * @param apActive True if access point is active
     * @param staConnected True if connected to external WiFi network
     * @param apIP Access point IP address as string
     * @param staIP Station IP address as string
     * @param rssi WiFi signal strength in dBm
     */
    void setWiFiStatus(bool apActive, bool staConnected, String apIP, String staIP, int rssi, String status);
    
    /**
     * @brief Update the GNSS status display data
     *
     * @param enabled True if GNSS module is enabled
     * @param hasFix True if GNSS has a valid position fix
     * @param lat Latitude in decimal degrees (-90 to +90)
     * @param lon Longitude in decimal degrees (-180 to +180)
     * @param sats Number of satellites used in fix (0-255)
     * @param clients Number of connected NMEA clients (0-255)
     */
    void setGNSSStatus(bool enabled, bool hasFix, double lat, double lon, uint8_t sats, uint8_t clients);
    
    /**
     * @brief Turn off the display for power saving
     *
     * Puts the OLED display into low-power mode. Display content is preserved
     * and will be restored when displayOn() is called.
     */
    void displayOff();
    
    /**
     * @brief Turn on the display
     *
     * Restores the OLED display from power-saving mode.
     */
    void displayOn();
    
    /**
     * @brief Check if the display is currently off
     *
     * @return true if display is in power-saving mode
     * @return false if display is active
     */
    bool isDisplayOff() { return currentScreen == SCREEN_OFF; }
    
    /**
     * @brief Check if the boot screen is currently being displayed
     *
     * @return true if boot screen is active (before timeout)
     * @return false if boot screen has timed out or different screen is active
     */
    bool isBootScreenActive();
    
    /**
     * @brief Handle a button press event
     *
     * Processes button input for screen navigation and power management.
     * Implements debouncing and long-press detection.
     */
    void handleButtonPress();
    
    /**
     * @brief Get the currently active screen type
     *
     * @return Current DisplayScreen enum value
     */
    DisplayScreen getCurrentScreen() { return currentScreen; }
    
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    DisplayScreen currentScreen;
    DisplayScreen lastScreen;
    
    // Boot screen state
    bool bootScreenActive;
    uint32_t bootScreenStartTime;
    uint32_t bootScreenDuration;
    
    // Radio config data
    float radioFreq;
    float radioBW;
    uint8_t radioSF;
    uint8_t radioCR;
    int8_t radioPower;
    uint16_t radioSyncWord;
    
    // Battery data
    float batteryVoltage;
    
    // WiFi data
    bool wifiAPActive;
    bool wifiSTAConnected;
    String wifiAPIP;
    String wifiSTAIP;
    int wifiRSSI;
    String wifiStatus;
    
    // WiFi startup status
    String wifiStartupMessage;
    
    // Initialization status
    String initMessage;
    String initComponent;
    bool initSuccess;
    
    // Ready screen status
    bool readyRadioOK;
    String readyWiFiStatus;
    bool readyGNSSOK;
    bool readyGNSSFix;
    String readyBoardType;
    
    // GNSS data
    bool gnssEnabled;
    bool gnssHasFix;
    double gnssLatitude;
    double gnssLongitude;
    uint8_t gnssSatellites;
    uint8_t gnssClients;
    
    // Button handling
    uint32_t lastButtonPress;
    static const uint32_t BUTTON_DEBOUNCE_MS = 500;
    static const uint32_t BUTTON_LONG_PRESS_MS = 2000;
    static const uint32_t BUTTON_CHECK_DELAY_MS = 50;
    static const uint32_t POWER_OFF_DELAY_MS = 1000;
    
    // Screen rendering functions
    void renderBootScreen();
    void renderInitScreen();
    void renderReadyScreen();
    void renderWiFiStartupScreen();
    void renderStatusScreen();
    void renderWiFiScreen();
    void renderBatteryScreen();
    void renderGNSSScreen();
    void renderOffScreen();
    
    // Helper functions
    String formatFrequency(float freq);
    String formatBandwidth(float bw);
    uint8_t getBatteryPercentage(float voltage);
};

// Button interrupt handler
void buttonInterruptHandler();

// Global display instance
extern DisplayManager displayManager;
extern volatile bool buttonPressed;

#endif // DISPLAY_H
