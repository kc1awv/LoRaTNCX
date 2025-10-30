#pragma once

#include <Arduino.h>
#include <TNCCommands.h>
#include <U8g2lib.h>
#include "HardwareConfig.h"

/**
 * @brief Simple OLED display manager for the Heltec WiFi LoRa 32 V4 board.
 */
class DisplayManager
{
public:
    DisplayManager();

    /**
     * @brief Initialize the OLED display hardware.
     * @return true if the display is available and ready for use.
     */
    bool begin();

    /**
     * @brief Show a minimal boot screen while the system is starting.
     */
    void showBootScreen();

    /**
     * @brief Update the runtime status view.
     *
     * @param mode    Current operating mode of the TNC.
     * @param txCount Number of LoRa frames transmitted.
     * @param rxCount Number of LoRa frames received.
     */
    void updateStatus(TNCMode mode, uint32_t txCount, uint32_t rxCount, float batteryVoltage, uint8_t batteryPercent);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    bool enabled;
    TNCMode lastMode;
    uint32_t lastTx;
    uint32_t lastRx;
    float lastBatteryVoltage;
    uint8_t lastBatteryPercent;
    unsigned long lastRefresh;

    const char *modeToLabel(TNCMode mode) const;
    void drawStatus(TNCMode mode, uint32_t txCount, uint32_t rxCount, float batteryVoltage, uint8_t batteryPercent);
};
