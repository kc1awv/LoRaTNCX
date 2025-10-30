#include "DisplayManager.h"

#include <Wire.h>

#include <cmath>

DisplayManager::DisplayManager()
    : u8g2(U8G2_R0, OLED_RST_PIN),
      enabled(false),
      lastMode(TNCMode::COMMAND_MODE),
      lastTx(0),
      lastRx(0),
      lastBatteryVoltage(0.0f),
      lastBatteryPercent(0),
      lastRefresh(0)
{
}

bool DisplayManager::begin()
{
    if (!DISPLAY_ENABLED)
    {
        return false;
    }

    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, POWER_ON);
    delay(10);

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    u8g2.begin();
    u8g2.setI2CAddress(OLED_ADDRESS << 1);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);

    enabled = true;
    lastMode = TNCMode::COMMAND_MODE;
    lastTx = 0;
    lastRx = 0;
    lastBatteryVoltage = 0.0f;
    lastBatteryPercent = 0;
    lastRefresh = 0;

    return true;
}

void DisplayManager::showBootScreen()
{
    if (!enabled)
    {
        return;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    const char *title = "LoRaTNCX";
    int16_t titleWidth = u8g2.getStrWidth(title);
    int16_t titleX = (HELTEC_V4_DISPLAY_WIDTH - titleWidth) / 2;
    if (titleX < 0)
    {
        titleX = 0;
    }
    u8g2.drawStr(titleX, 32, title);

    u8g2.setFont(u8g2_font_6x10_tf);
    const char *subtitle = "Booting...";
    int16_t subtitleWidth = u8g2.getStrWidth(subtitle);
    int16_t subtitleX = (HELTEC_V4_DISPLAY_WIDTH - subtitleWidth) / 2;
    if (subtitleX < 0)
    {
        subtitleX = 0;
    }
    u8g2.drawStr(subtitleX, 54, subtitle);
    u8g2.sendBuffer();
}

void DisplayManager::updateStatus(TNCMode mode, uint32_t txCount, uint32_t rxCount, float batteryVoltage, uint8_t batteryPercent)
{
    if (!enabled)
    {
        return;
    }

    bool changed = (mode != lastMode) || (txCount != lastTx) || (rxCount != lastRx);
    if (!changed)
    {
        changed = (std::fabs(batteryVoltage - lastBatteryVoltage) >= 0.05f) || (batteryPercent != lastBatteryPercent);
    }
    unsigned long now = millis();

    if (!changed && (now - lastRefresh) < DISPLAY_UPDATE)
    {
        return;
    }

    lastMode = mode;
    lastTx = txCount;
    lastRx = rxCount;
    lastBatteryVoltage = batteryVoltage;
    lastBatteryPercent = batteryPercent;
    lastRefresh = now;

    drawStatus(mode, txCount, rxCount, batteryVoltage, batteryPercent);
}

const char *DisplayManager::modeToLabel(TNCMode mode) const
{
    switch (mode)
    {
    case TNCMode::COMMAND_MODE:
        return "CMD";
    case TNCMode::TERMINAL_MODE:
        return "CONV";
    case TNCMode::TRANSPARENT_MODE:
        return "TRAN";
    case TNCMode::KISS_MODE:
        return "KISS";
    default:
        return "UNK";
    }
}

void DisplayManager::drawStatus(TNCMode mode, uint32_t txCount, uint32_t rxCount, float batteryVoltage, uint8_t batteryPercent)
{
    u8g2.clearBuffer();

    const char *title = "LoRaTNCX";
    u8g2.setFont(u8g2_font_9x15B_tf);
    int16_t titleWidth = u8g2.getStrWidth(title);
    int16_t titleX = (HELTEC_V4_DISPLAY_WIDTH - titleWidth) / 2;
    if (titleX < 0)
    {
        titleX = 0;
    }
    u8g2.drawStr(titleX, 16, title);

    u8g2.setFont(u8g2_font_6x12_tf);
    char modeLine[20];
    snprintf(modeLine, sizeof(modeLine), "MODE: %s", modeToLabel(mode));
    u8g2.drawStr(0, 36, modeLine);

    char statsLine[24];
    snprintf(statsLine, sizeof(statsLine), "LoRa TX:%lu RX:%lu", static_cast<unsigned long>(txCount), static_cast<unsigned long>(rxCount));
    u8g2.drawStr(0, 52, statsLine);

    char batteryLine[24];
    snprintf(batteryLine, sizeof(batteryLine), "BAT: %.2fV %3u%%", static_cast<double>(batteryVoltage), static_cast<unsigned int>(batteryPercent));
    u8g2.drawStr(0, 62, batteryLine);

    u8g2.sendBuffer();
}
