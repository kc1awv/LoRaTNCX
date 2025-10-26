#include "DisplayUtils.h"
#include "Constants.h"
#include <stdarg.h>

SSD1306Wire* DisplayUtils::display = nullptr;
char DisplayUtils::buffer[64];

void DisplayUtils::setup(SSD1306Wire* display_ptr) {
    display = display_ptr;
}

void DisplayUtils::drawFormatted(int x, int y, const __FlashStringHelper* format, ...) {
    if (!display) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf_P(buffer, sizeof(buffer), (const char*)format, args);
    va_end(args);
    
    display->drawString(x, y, buffer);
}

// New helper function for combining prefix and string
void DisplayUtils::drawPrefixedString(int x, int y, const __FlashStringHelper* prefix, const char* text) {
    if (!display) return;
    
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%s"), prefix, text);
    display->drawString(x, y, buffer);
}

void DisplayUtils::drawNumberWithUnit(int x, int y, const __FlashStringHelper* prefix, 
                                      int number, const __FlashStringHelper* suffix) {
    if (!display) return;
    
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%d%S"), prefix, number, suffix);
    display->drawString(x, y, buffer);
}

void DisplayUtils::drawFloatWithUnit(int x, int y, const __FlashStringHelper* prefix, 
                                     float number, int decimals, const __FlashStringHelper* suffix) {
    if (!display) return;
    
    // Use dtostrf for consistent float formatting
    char floatBuffer[16];
    dtostrf(number, 0, decimals, floatBuffer);
    
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%s%S"), prefix, floatBuffer, suffix);
    display->drawString(x, y, buffer);
}

void DisplayUtils::drawBooleanStatus(int x, int y, const __FlashStringHelper* prefix, 
                                     bool state, const __FlashStringHelper* trueText, 
                                     const __FlashStringHelper* falseText) {
    if (!display) return;
    
    const __FlashStringHelper* statusText;
    if (trueText && falseText) {
        statusText = state ? trueText : falseText;
    } else {
        statusText = state ? FPSTR(DISPLAY_YES) : FPSTR(DISPLAY_NO);
    }
    
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%S"), prefix, statusText);
    display->drawString(x, y, buffer);
}

void DisplayUtils::drawTimeAgo(int x, int y, const __FlashStringHelper* prefix, unsigned long lastTime) {
    if (!display) return;
    
    unsigned long secondsAgo = (millis() - lastTime) / 1000;
    
    if (secondsAgo < 60) {
        snprintf_P(buffer, sizeof(buffer), PSTR("%S%lu%S"), prefix, secondsAgo, FPSTR(DISPLAY_SECONDS_AGO));
    } else {
        strcpy_P(buffer, (const char*)FPSTR(DISPLAY_IDLE));
    }
    
    display->drawString(x, y, buffer);
}

void DisplayUtils::drawProgressBar(int x, int y, int width, int height, float progress) {
    if (!display) return;
    
    // Draw outline
    display->drawRect(x, y, width, height);
    
    // Fill progress
    int fillWidth = (int)(width * progress);
    if (fillWidth > 0) {
        for (int dy = y + 1; dy < y + height - 1; dy++) {
            display->drawHorizontalLine(x + 1, dy, fillWidth - 1);
        }
    }
}

void DisplayUtils::drawPageIndicator(int currentScreen, int totalScreens) {
    if (!display) return;
    
    snprintf_P(buffer, sizeof(buffer), PSTR("%d/%d"), currentScreen + 1, totalScreens);
    int indicatorWidth = display->getStringWidth(buffer);
    display->drawString(128 - indicatorWidth, 0, buffer);
}

void DisplayUtils::drawRadioConfig(int y, float freq, int power, int sf, float bw) {
    if (!display) return;
    
    // Frequency line
    drawFloatWithUnit(0, y, FPSTR(DISPLAY_FREQUENCY), freq, 3, FPSTR(DISPLAY_MHZ));
    
    // Power line
    drawNumberWithUnit(0, y + 10, FPSTR(DISPLAY_POWER), power, FPSTR(DISPLAY_DBM));
    
    // SF and BW line - need custom formatting for this complex line
    char bwBuffer[8];
    dtostrf(bw, 0, 0, bwBuffer);
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%d%S%s%S"), 
               FPSTR(DISPLAY_SF), sf, FPSTR(DISPLAY_BW), bwBuffer, FPSTR(DISPLAY_KHZ));
    display->drawString(0, y + 20, buffer);
}

void DisplayUtils::drawBatteryStatus(int y, float voltage, int soc, bool charging) {
    if (!display) return;
    
    // Battery voltage and SoC
    char voltageBuffer[8];
    dtostrf(voltage, 0, 1, voltageBuffer);
    
    const char* chargingText = charging ? (const char*)FPSTR(DISPLAY_CHARGING) : "";
    snprintf_P(buffer, sizeof(buffer), PSTR("%S%d%%%s %s%S%s"), 
               FPSTR(DISPLAY_BATTERY), soc, FPSTR(DISPLAY_PERCENT), voltageBuffer, FPSTR(DISPLAY_VOLTAGE), chargingText);
    display->drawString(0, y, buffer);
}

void DisplayUtils::drawNetworkStatus(int y, bool apMode, bool connected, int rssi, int clients) {
    if (!display) return;
    
    if (apMode) {
        drawNumberWithUnit(0, y, FPSTR(DISPLAY_AP_CLIENTS), clients, F(""));
    } else if (connected) {
        drawNumberWithUnit(0, y, FPSTR(DISPLAY_RSSI), rssi, FPSTR(DISPLAY_DBM));
    }
}

void DisplayUtils::drawGNSSStatus(int y, bool enabled, unsigned long lastNmea) {
    if (!display) return;
    
    if (enabled) {
        drawTimeAgo(0, y, FPSTR(DISPLAY_NMEA), lastNmea);
    } else {
        display->drawString(0, y, FPSTR(DISPLAY_USE_CONFIG));
        display->drawString(0, y + 10, FPSTR(DISPLAY_ENABLE_APRS));
    }
}

// Global instance
DisplayUtils displayUtils;