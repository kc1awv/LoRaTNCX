#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"

// Screen types
enum DisplayScreen {
    SCREEN_BOOT,        // Boot/splash screen
    SCREEN_STATUS,      // Main status screen showing radio config
    SCREEN_WIFI,        // WiFi status screen
    SCREEN_BATTERY,     // Battery status screen
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
    
    // Update WiFi status display
    void setWiFiStatus(bool apActive, bool staConnected, String apIP, String staIP, int rssi);
    
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
    
    // Button handling
    uint32_t lastButtonPress;
    static const uint32_t BUTTON_DEBOUNCE_MS = 200;
    static const uint32_t BUTTON_LONG_PRESS_MS = 2000;
    
    // Screen rendering functions
    void renderBootScreen();
    void renderStatusScreen();
    void renderWiFiScreen();
    void renderBatteryScreen();
    
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
