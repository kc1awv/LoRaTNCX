#pragma once

#include <Arduino.h>
#include <TNCCommands.h>
#include <U8g2lib.h>
#include <math.h>
#include "HardwareConfig.h"

/**
 * @brief Simple OLED display manager for the Heltec WiFi LoRa 32 V4 board.
 */
class DisplayManager
{
public:
    enum class Screen : uint8_t
    {
        MAIN = 0,
        WIFI_STATUS,
        LORA_DETAILS,
        BATTERY,
        SYSTEM,
        GNSS_STATUS
    };

    struct StatusData
    {
        enum class WiFiMode : uint8_t
        {
            OFF = 0,
            ACCESS_POINT,
            STATION,
            AP_STATION
        };

        TNCMode mode = TNCMode::COMMAND_MODE;
        uint32_t txCount = 0;
        uint32_t rxCount = 0;
        float batteryVoltage = 0.0f;
        uint8_t batteryPercent = 0;
        bool hasRecentPacket = false;
        float lastRSSI = 0.0f;
        float lastSNR = 0.0f;
        unsigned long lastPacketMillis = 0;
        float frequency = 0.0f;
        float bandwidth = 0.0f;
        uint8_t spreadingFactor = 0;
        uint8_t codingRate = 0;
        int8_t txPower = 0;
        unsigned long uptimeMillis = 0;
        bool powerOffActive = false;
        float powerOffProgress = 0.0f;
        bool powerOffComplete = false;

        bool gnssEnabled = false;
        bool gnssHasFix = false;
        bool gnssIs3DFix = false;
        double gnssLatitude = NAN;
        double gnssLongitude = NAN;
        double gnssAltitude = NAN;
        float gnssSpeed = 0.0f;
        float gnssCourse = 0.0f;
        float gnssHdop = 0.0f;
        uint8_t gnssSatellites = 0;
        bool gnssTimeValid = false;
        bool gnssTimeSynced = false;
        uint16_t gnssYear = 0;
        uint8_t gnssMonth = 0;
        uint8_t gnssDay = 0;
        uint8_t gnssHour = 0;
        uint8_t gnssMinute = 0;
        uint8_t gnssSecond = 0;
        bool gnssPpsAvailable = false;
        unsigned long gnssPpsLastMillis = 0;
        uint32_t gnssPpsCount = 0;

        WiFiMode wifiMode = WiFiMode::OFF;
        bool wifiConnected = false;
        bool wifiConnecting = false;
        bool wifiHasIPAddress = false;
        char wifiSSID[33] = {0};
        char wifiIPAddress[18] = {0};
    };

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
     * @brief Update the runtime status view using the provided telemetry.
     */
    void updateStatus(const StatusData &status);

    /**
     * @brief Advance to the next detail screen.
     */
    void nextScreen();

    /**
     * @brief Set the active screen directly.
     */
    void setScreen(Screen screen);

    /**
     * @brief Get the currently selected screen.
     */
    Screen getScreen() const { return currentScreen; }

    /**
     * @brief Get the total number of available screens.
     */
    uint8_t getScreenCount() const { return SCREEN_COUNT; }

    /**
     * @brief Enable or disable the OLED panel at runtime.
     * @return true if the request could be processed.
     */
    bool setEnabled(bool enable);

    /**
     * @brief Query whether the OLED panel is currently active.
     */
    bool isEnabled() const { return enabled; }

    /**
     * @brief Query whether OLED hardware is available on this build.
     */
    bool isAvailable() const { return hardwarePresent; }

private:
    static constexpr uint8_t SCREEN_COUNT = 6;

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    bool enabled;
    bool hardwarePresent;
    Screen currentScreen;
    Screen lastRenderedScreen;
    bool hasLastStatus;
    bool forceFullRefresh;
    StatusData lastStatus;
    unsigned long lastRefresh;

    const char *modeToLabel(TNCMode mode) const;
    void drawMainScreen();
    void drawWiFiScreen();
    void drawLoRaDetails();
    void drawBatteryScreen();
    void drawSystemScreen();
    void drawGNSSScreen();
    void drawPowerOffWarning();
    void drawPowerOffComplete();
    void drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, float progress);
    void drawBatteryGauge(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t percent);
    void drawCenteredText(int16_t y, const char *text, const uint8_t *font);
    void drawHeader(const char *title);
    static void formatUptime(char *buffer, size_t length, unsigned long millisValue);

    void initializeHardware();
    void shutdownHardware();
};
