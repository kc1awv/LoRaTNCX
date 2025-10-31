#include "DisplayManager.h"

#include <Wire.h>

#include <cmath>

namespace
{
float clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

bool floatsDiffer(float a, float b, float epsilon = 0.01f)
{
    if (std::isnan(a) && std::isnan(b))
    {
        return false;
    }
    if (std::isnan(a) || std::isnan(b))
    {
        return true;
    }
    return std::fabs(a - b) >= epsilon;
}

bool doublesDiffer(double a, double b, double epsilon = 0.0001)
{
    if (std::isnan(a) && std::isnan(b))
    {
        return false;
    }
    if (std::isnan(a) || std::isnan(b))
    {
        return true;
    }
    return std::fabs(a - b) >= epsilon;
}
}

DisplayManager::DisplayManager()
    : u8g2(U8G2_R0, OLED_RST_PIN),
      enabled(false),
      hardwarePresent(false),
      currentScreen(Screen::MAIN),
      lastRenderedScreen(Screen::MAIN),
      hasLastStatus(false),
      forceFullRefresh(true),
      lastStatus(),
      lastRefresh(0)
{
}

bool DisplayManager::begin()
{
    hardwarePresent = DISPLAY_ENABLED;
    if (!hardwarePresent)
    {
        enabled = false;
        return false;
    }

    initializeHardware();

    enabled = true;
    currentScreen = Screen::MAIN;
    lastRenderedScreen = Screen::MAIN;
    hasLastStatus = false;
    forceFullRefresh = true;
    lastStatus = StatusData();
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

void DisplayManager::updateStatus(const StatusData &status)
{
    if (!enabled)
    {
        return;
    }

    bool dataChanged = !hasLastStatus ||
                       status.mode != lastStatus.mode ||
                       status.txCount != lastStatus.txCount ||
                       status.rxCount != lastStatus.rxCount ||
                       (std::fabs(status.batteryVoltage - lastStatus.batteryVoltage) >= 0.05f) ||
                       status.batteryPercent != lastStatus.batteryPercent ||
                       status.hasRecentPacket != lastStatus.hasRecentPacket ||
                       (status.hasRecentPacket && (!lastStatus.hasRecentPacket ||
                                                   std::fabs(status.lastRSSI - lastStatus.lastRSSI) >= 0.5f ||
                                                   std::fabs(status.lastSNR - lastStatus.lastSNR) >= 0.5f ||
                                                   status.lastPacketMillis != lastStatus.lastPacketMillis)) ||
                       (std::fabs(status.frequency - lastStatus.frequency) >= 0.01f) ||
                       (std::fabs(status.bandwidth - lastStatus.bandwidth) >= 0.1f) ||
                       status.spreadingFactor != lastStatus.spreadingFactor ||
                       status.codingRate != lastStatus.codingRate ||
                       status.txPower != lastStatus.txPower;

    bool powerStateChanged = !hasLastStatus ||
                             status.powerOffActive != lastStatus.powerOffActive ||
                             status.powerOffComplete != lastStatus.powerOffComplete;

    bool progressChanged = !hasLastStatus ||
                           (std::fabs(status.powerOffProgress - lastStatus.powerOffProgress) >= 0.02f);

    bool gnssChanged = !hasLastStatus ||
                       status.gnssEnabled != lastStatus.gnssEnabled ||
                       status.gnssHasFix != lastStatus.gnssHasFix ||
                       status.gnssIs3DFix != lastStatus.gnssIs3DFix ||
                       doublesDiffer(status.gnssLatitude, lastStatus.gnssLatitude, 0.0005) ||
                       doublesDiffer(status.gnssLongitude, lastStatus.gnssLongitude, 0.0005) ||
                       doublesDiffer(status.gnssAltitude, lastStatus.gnssAltitude, 0.5) ||
                       floatsDiffer(status.gnssSpeed, lastStatus.gnssSpeed, 0.2f) ||
                       floatsDiffer(status.gnssCourse, lastStatus.gnssCourse, 1.0f) ||
                       floatsDiffer(status.gnssHdop, lastStatus.gnssHdop, 0.1f) ||
                       status.gnssSatellites != lastStatus.gnssSatellites ||
                       status.gnssTimeValid != lastStatus.gnssTimeValid ||
                       status.gnssTimeSynced != lastStatus.gnssTimeSynced ||
                       status.gnssYear != lastStatus.gnssYear ||
                       status.gnssMonth != lastStatus.gnssMonth ||
                       status.gnssDay != lastStatus.gnssDay ||
                       status.gnssHour != lastStatus.gnssHour ||
                       status.gnssMinute != lastStatus.gnssMinute ||
                       status.gnssSecond != lastStatus.gnssSecond ||
                       status.gnssPpsAvailable != lastStatus.gnssPpsAvailable ||
                       status.gnssPpsCount != lastStatus.gnssPpsCount ||
                       status.gnssPpsLastMillis != lastStatus.gnssPpsLastMillis;

    bool screenChanged = (!status.powerOffActive && !status.powerOffComplete && (currentScreen != lastRenderedScreen));

    unsigned long now = millis();
    bool timedRefresh = (now - lastRefresh) >= DISPLAY_UPDATE;

    bool shouldRefresh = forceFullRefresh || dataChanged || powerStateChanged || progressChanged || screenChanged || timedRefresh || gnssChanged;

    if (!shouldRefresh)
    {
        return;
    }

    lastStatus = status;
    hasLastStatus = true;
    lastRefresh = now;

    if (status.powerOffComplete)
    {
        drawPowerOffComplete();
    }
    else if (status.powerOffActive)
    {
        drawPowerOffWarning();
    }
    else
    {
        switch (currentScreen)
        {
        case Screen::MAIN:
            drawMainScreen();
            break;
        case Screen::LORA_DETAILS:
            drawLoRaDetails();
            break;
        case Screen::BATTERY:
            drawBatteryScreen();
            break;
        case Screen::SYSTEM:
            drawSystemScreen();
            break;
        case Screen::GNSS_STATUS:
        default:
            drawGNSSScreen();
            break;
        }
        lastRenderedScreen = currentScreen;
    }

    forceFullRefresh = false;
    u8g2.sendBuffer();
}

bool DisplayManager::setEnabled(bool enable)
{
    if (!hardwarePresent)
    {
        enabled = false;
        return false;
    }

    if (enable == enabled)
    {
        return true;
    }

    if (enable)
    {
        initializeHardware();
        enabled = true;
        currentScreen = Screen::MAIN;
        lastRenderedScreen = Screen::MAIN;
        hasLastStatus = false;
        forceFullRefresh = true;
        lastStatus = StatusData();
        lastRefresh = 0;
    }
    else
    {
        shutdownHardware();
        enabled = false;
        hasLastStatus = false;
        forceFullRefresh = true;
    }

    return true;
}

void DisplayManager::nextScreen()
{
    if (!enabled)
    {
        return;
    }

    uint8_t next = (static_cast<uint8_t>(currentScreen) + 1U) % SCREEN_COUNT;
    currentScreen = static_cast<Screen>(next);
    forceFullRefresh = true;

    if (hasLastStatus)
    {
        updateStatus(lastStatus);
    }
}

void DisplayManager::setScreen(Screen screen)
{
    if (!enabled)
    {
        return;
    }

    currentScreen = screen;
    forceFullRefresh = true;

    if (hasLastStatus)
    {
        updateStatus(lastStatus);
    }
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

void DisplayManager::drawMainScreen()
{
    u8g2.clearBuffer();
    drawHeader("LoRaTNCX");

    u8g2.setFont(u8g2_font_6x12_tf);

    char modeLine[24];
    snprintf(modeLine, sizeof(modeLine), "MODE: %s", modeToLabel(lastStatus.mode));
    u8g2.drawStr(0, 32, modeLine);

    char statsLine[28];
    snprintf(statsLine, sizeof(statsLine), "LoRa TX:%lu RX:%lu",
             static_cast<unsigned long>(lastStatus.txCount),
             static_cast<unsigned long>(lastStatus.rxCount));
    u8g2.drawStr(0, 46, statsLine);

    char batteryLine[28];
    snprintf(batteryLine, sizeof(batteryLine), "BAT: %.2fV %3u%%",
             static_cast<double>(lastStatus.batteryVoltage),
             static_cast<unsigned int>(lastStatus.batteryPercent));
    u8g2.drawStr(0, 60, batteryLine);
}

void DisplayManager::drawLoRaDetails()
{
    u8g2.clearBuffer();
    drawHeader("LoRa Packets");
    u8g2.setFont(u8g2_font_6x10_tf);

    char totals[32];
    snprintf(totals, sizeof(totals), "TX:%lu  RX:%lu",
             static_cast<unsigned long>(lastStatus.txCount),
             static_cast<unsigned long>(lastStatus.rxCount));
    u8g2.drawStr(0, 28, totals);

    if (lastStatus.hasRecentPacket)
    {
        char rssiLine[32];
        snprintf(rssiLine, sizeof(rssiLine), "RSSI: %.1f dBm", static_cast<double>(lastStatus.lastRSSI));
        u8g2.drawStr(0, 40, rssiLine);

        char snrLine[32];
        snprintf(snrLine, sizeof(snrLine), "SNR: %.1f dB", static_cast<double>(lastStatus.lastSNR));
        u8g2.drawStr(0, 52, snrLine);

        char ageBuffer[16];
        unsigned long ageMillis = (lastStatus.uptimeMillis >= lastStatus.lastPacketMillis) ? (lastStatus.uptimeMillis - lastStatus.lastPacketMillis) : 0UL;
        formatUptime(ageBuffer, sizeof(ageBuffer), ageMillis);

        char ageLine[32];
        snprintf(ageLine, sizeof(ageLine), "Last RX: %s ago", ageBuffer);
        u8g2.drawStr(0, 62, ageLine);
    }
    else
    {
        u8g2.drawStr(0, 44, "Waiting for packets...");
    }
}

void DisplayManager::drawBatteryScreen()
{
    u8g2.clearBuffer();
    drawHeader("Battery");

    drawBatteryGauge(94, 22, 28, 14, lastStatus.batteryPercent);

    char percentLine[16];
    snprintf(percentLine, sizeof(percentLine), "%3u%%", static_cast<unsigned int>(lastStatus.batteryPercent));
    drawCenteredText(44, percentLine, u8g2_font_9x15B_tf);

    u8g2.setFont(u8g2_font_6x10_tf);
    char voltageLine[24];
    snprintf(voltageLine, sizeof(voltageLine), "Voltage: %.2f V", static_cast<double>(lastStatus.batteryVoltage));
    u8g2.drawStr(0, 58, voltageLine);

    drawProgressBar(0, 60, HELTEC_V4_DISPLAY_WIDTH, 4, static_cast<float>(lastStatus.batteryPercent) / 100.0f);
}

void DisplayManager::drawSystemScreen()
{
    u8g2.clearBuffer();
    drawHeader("System Info");
    u8g2.setFont(u8g2_font_6x10_tf);

    char modeLine[24];
    snprintf(modeLine, sizeof(modeLine), "Mode: %s", modeToLabel(lastStatus.mode));
    u8g2.drawStr(0, 28, modeLine);

    char uptimeBuffer[16];
    formatUptime(uptimeBuffer, sizeof(uptimeBuffer), lastStatus.uptimeMillis);
    char uptimeLine[32];
    snprintf(uptimeLine, sizeof(uptimeLine), "Uptime: %s", uptimeBuffer);
    u8g2.drawStr(0, 40, uptimeLine);

    char freqPowerLine[36];
    snprintf(freqPowerLine, sizeof(freqPowerLine), "Freq %.1f MHz  %d dBm",
             static_cast<double>(lastStatus.frequency),
             static_cast<int>(lastStatus.txPower));
    u8g2.drawStr(0, 54, freqPowerLine);

    char radioLine[36];
    snprintf(radioLine, sizeof(radioLine), "BW %.1f kHz  SF%u  CR4/%u",
             static_cast<double>(lastStatus.bandwidth),
             lastStatus.spreadingFactor,
             lastStatus.codingRate);
    u8g2.drawStr(0, 64, radioLine);
}

void DisplayManager::drawGNSSScreen()
{
    u8g2.clearBuffer();
    drawHeader("GNSS Status");

    if (!lastStatus.gnssEnabled)
    {
        drawCenteredText(44, "GNSS module disabled", u8g2_font_6x10_tf);
        return;
    }

    u8g2.setFont(u8g2_font_6x10_tf);

    const char *fixLabel = "None";
    if (lastStatus.gnssHasFix)
    {
        fixLabel = lastStatus.gnssIs3DFix ? "3D" : "2D";
    }

    char fixLine[32];
    snprintf(fixLine, sizeof(fixLine), "Fx:%s Sa:%02u PP:%s",
             fixLabel,
             static_cast<unsigned int>(lastStatus.gnssSatellites),
             lastStatus.gnssPpsAvailable ? "OK" : "--");
    u8g2.drawStr(0, 28, fixLine);

    char qualityLine[32];
    if (lastStatus.gnssHasFix)
    {
        if (!std::isnan(lastStatus.gnssAltitude))
        {
            snprintf(qualityLine, sizeof(qualityLine), "HD:%.1f Al:%.0fm S:%c",
                     static_cast<double>(lastStatus.gnssHdop),
                     static_cast<double>(lastStatus.gnssAltitude),
                     lastStatus.gnssTimeSynced ? 'Y' : 'N');
        }
        else
        {
            snprintf(qualityLine, sizeof(qualityLine), "HD:%.1f Al:-- S:%c",
                     static_cast<double>(lastStatus.gnssHdop),
                     lastStatus.gnssTimeSynced ? 'Y' : 'N');
        }
    }
    else
    {
        snprintf(qualityLine, sizeof(qualityLine), "HD:-- Al:-- S:%c", lastStatus.gnssTimeSynced ? 'Y' : 'N');
    }
    u8g2.drawStr(0, 40, qualityLine);

    char latLine[32];
    if (lastStatus.gnssHasFix && !std::isnan(lastStatus.gnssLatitude))
    {
        char hemisphere = (lastStatus.gnssLatitude >= 0.0) ? 'N' : 'S';
        double magnitude = std::fabs(lastStatus.gnssLatitude);
        if (lastStatus.gnssSpeed >= 0.1f)
        {
            snprintf(latLine, sizeof(latLine), "La:%6.2f%c Sp:%02.0fkt",
                     magnitude,
                     hemisphere,
                     static_cast<double>(lastStatus.gnssSpeed));
        }
        else
        {
            snprintf(latLine, sizeof(latLine), "La:%6.2f%c Sp:--",
                     magnitude,
                     hemisphere);
        }
    }
    else
    {
        snprintf(latLine, sizeof(latLine), "La:-- Sp:--");
    }
    u8g2.drawStr(0, 52, latLine);

    char lonLine[32];
    if (lastStatus.gnssHasFix && !std::isnan(lastStatus.gnssLongitude))
    {
        char hemisphere = (lastStatus.gnssLongitude >= 0.0) ? 'E' : 'W';
        double magnitude = std::fabs(lastStatus.gnssLongitude);
        if (lastStatus.gnssTimeValid)
        {
            snprintf(lonLine, sizeof(lonLine), "Lo:%6.2f%c T:%02u:%02u",
                     magnitude,
                     hemisphere,
                     static_cast<unsigned int>(lastStatus.gnssHour),
                     static_cast<unsigned int>(lastStatus.gnssMinute));
        }
        else
        {
            snprintf(lonLine, sizeof(lonLine), "Lo:%6.2f%c T:--:--",
                     magnitude,
                     hemisphere);
        }
    }
    else
    {
        if (lastStatus.gnssTimeValid)
        {
            snprintf(lonLine, sizeof(lonLine), "Lo:-- T:%02u:%02u",
                     static_cast<unsigned int>(lastStatus.gnssHour),
                     static_cast<unsigned int>(lastStatus.gnssMinute));
        }
        else
        {
            snprintf(lonLine, sizeof(lonLine), "Lo:-- T:--:--");
        }
    }
    u8g2.drawStr(0, 64, lonLine);
}

void DisplayManager::drawPowerOffWarning()
{
    u8g2.clearBuffer();
    drawHeader("Power Off");

    drawCenteredText(36, "Hold button to power off", u8g2_font_6x10_tf);
    drawCenteredText(48, "Release to cancel", u8g2_font_5x8_tf);

    drawProgressBar(12, 54, HELTEC_V4_DISPLAY_WIDTH - 24, 12, clamp01(lastStatus.powerOffProgress));
}

void DisplayManager::drawPowerOffComplete()
{
    u8g2.clearBuffer();
    drawHeader("Shutting Down");
    drawCenteredText(40, "Powering off...", u8g2_font_6x12_tf);
}

void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, float progress)
{
    float clamped = clamp01(progress);
    u8g2.drawFrame(x, y, width, height);

    int16_t innerWidth = width - 2;
    int16_t innerHeight = height - 2;
    int16_t fillWidth = static_cast<int16_t>(innerWidth * clamped);
    if (fillWidth > 0 && innerHeight > 0)
    {
        u8g2.drawBox(x + 1, y + 1, fillWidth, innerHeight);
    }
}

void DisplayManager::drawBatteryGauge(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t percent)
{
    uint8_t clamped = percent > 100 ? 100 : percent;
    int16_t capWidth = 3;
    int16_t bodyWidth = width - capWidth;
    if (bodyWidth < 4)
    {
        bodyWidth = 4;
    }

    u8g2.drawFrame(x, y, bodyWidth, height);
    u8g2.drawBox(x + bodyWidth, y + height / 4, capWidth, height / 2);

    int16_t innerWidth = bodyWidth - 2;
    int16_t innerHeight = height - 2;
    float fill = static_cast<float>(clamped) / 100.0f;
    int16_t fillWidth = static_cast<int16_t>(innerWidth * fill);
    if (fillWidth > 0 && innerHeight > 0)
    {
        u8g2.drawBox(x + 1, y + 1, fillWidth, innerHeight);
    }
}

void DisplayManager::drawCenteredText(int16_t y, const char *text, const uint8_t *font)
{
    if (font != nullptr)
    {
        u8g2.setFont(font);
    }

    int16_t width = u8g2.getStrWidth(text);
    int16_t x = (HELTEC_V4_DISPLAY_WIDTH - width) / 2;
    if (x < 0)
    {
        x = 0;
    }
    u8g2.drawStr(x, y, text);
}

void DisplayManager::drawHeader(const char *title)
{
    u8g2.setFont(u8g2_font_9x15B_tf);
    int16_t titleWidth = u8g2.getStrWidth(title);
    int16_t titleX = (HELTEC_V4_DISPLAY_WIDTH - titleWidth) / 2;
    if (titleX < 0)
    {
        titleX = 0;
    }
    u8g2.drawStr(titleX, 16, title);
    u8g2.drawHLine(0, 20, HELTEC_V4_DISPLAY_WIDTH);
}

void DisplayManager::formatUptime(char *buffer, size_t length, unsigned long millisValue)
{
    if (length == 0)
    {
        return;
    }

    unsigned long totalSeconds = millisValue / 1000UL;
    unsigned long hours = totalSeconds / 3600UL;
    unsigned long minutes = (totalSeconds % 3600UL) / 60UL;
    unsigned long seconds = totalSeconds % 60UL;

    if (hours > 99UL)
    {
        hours = 99UL;
    }

    snprintf(buffer, length, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

void DisplayManager::initializeHardware()
{
    pinMode(POWER_CTRL_PIN, OUTPUT);
    digitalWrite(POWER_CTRL_PIN, POWER_ON);
    delay(10);

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    u8g2.begin();
    u8g2.setI2CAddress(OLED_ADDRESS << 1);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void DisplayManager::shutdownHardware()
{
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(1);
    digitalWrite(POWER_CTRL_PIN, POWER_OFF);
}
