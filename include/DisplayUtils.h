#pragma once
#include <Arduino.h>
#include "SSD1306Wire.h"

class DisplayUtils 
{
public:
    static void setup(SSD1306Wire* display_ptr);
    
    // Efficient string composition without dynamic allocation
    static void drawFormatted(int x, int y, const __FlashStringHelper* format, ...);
    static void drawPrefixedString(int x, int y, const __FlashStringHelper* prefix, const char* text);
    static void drawNumberWithUnit(int x, int y, const __FlashStringHelper* prefix, 
                                   int number, const __FlashStringHelper* suffix);
    static void drawFloatWithUnit(int x, int y, const __FlashStringHelper* prefix, 
                                  float number, int decimals, const __FlashStringHelper* suffix);
    static void drawBooleanStatus(int x, int y, const __FlashStringHelper* prefix, 
                                  bool state, const __FlashStringHelper* trueText = nullptr, 
                                  const __FlashStringHelper* falseText = nullptr);
    static void drawTimeAgo(int x, int y, const __FlashStringHelper* prefix, unsigned long lastTime);
    static void drawProgressBar(int x, int y, int width, int height, float progress);
    static void drawPageIndicator(int currentScreen, int totalScreens);
    
    // Screen-specific optimized functions
    static void drawRadioConfig(int y, float freq, int power, int sf, float bw);
    static void drawBatteryStatus(int y, float voltage, int soc, bool charging);
    static void drawNetworkStatus(int y, bool apMode, bool connected, int rssi, int clients);
    static void drawGNSSStatus(int y, bool enabled, unsigned long lastNmea);
    
private:
    static SSD1306Wire* display;
    static char buffer[64];  // Static buffer for string composition
    
    // Helper methods
    static void formatFloat(char* dest, float value, int decimals);
    static void formatTime(char* dest, unsigned long seconds);
};

extern DisplayUtils displayUtils;