#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"

// Screen types
enum DisplayScreen {
    SCREEN_BOOT,         // Boot/splash screen
    SCREEN_WIFI_STARTUP, // WiFi initialization screen
    SCREEN_STATUS,       // Main status screen showing radio config
    SCREEN_WIFI,         // WiFi status screen
    SCREEN_BATTERY,      // Battery status screen
    SCREEN_GNSS,         // GNSS status screen
    SCREEN_OFF,          // Display off (power saving)
    NUM_SCREENS
};

// Display manager class
class DisplayManager {
public:
    DisplayManager();
    
    // Initialize display
    void begin();
    
    // Update display content (call regularly in loop)
    void update();
    
    // Set current screen
    void setScreen(DisplayScreen screen);
    
    // Cycle to next screen
    void nextScreen();
    
    // Show boot screen for specified duration
    void showBootScreen(uint32_t duration_ms = 2000);
    
    // Update radio configuration display
    void setRadioConfig(float freq, float bw, uint8_t sf, uint8_t cr, int8_t pwr, uint16_t sw);
    
    // Update battery voltage display
    void setBatteryVoltage(float voltage);
    
    // Set WiFi startup message
    void setWiFiStartupMessage(String message);
    
    // Update WiFi status display
    void setWiFiStatus(bool apActive, bool staConnected, String apIP, String staIP, int rssi);
    
    // Update GNSS status display
    void setGNSSStatus(bool enabled, bool hasFix, double lat, double lon, uint8_t sats, uint8_t clients);
    
    // Power management
    void displayOff();
    void displayOn();
    bool isDisplayOff() { return currentScreen == SCREEN_OFF; }
    
    // Check if boot screen is still showing
    bool isBootScreenActive();
    
    // Handle button press
    void handleButtonPress();
    
    // Get current screen
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
    
    // WiFi startup status
    String wifiStartupMessage;
    
    // GNSS data
    bool gnssEnabled;
    bool gnssHasFix;
    double gnssLatitude;
    double gnssLongitude;
    uint8_t gnssSatellites;
    uint8_t gnssClients;
    
    // Button handling
    uint32_t lastButtonPress;
    static const uint32_t BUTTON_DEBOUNCE_MS = 200;
    static const uint32_t BUTTON_LONG_PRESS_MS = 2000;
    
    // Screen rendering functions
    void renderBootScreen();
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
