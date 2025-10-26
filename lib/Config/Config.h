#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Operating modes for the device
enum class OperatingMode
{
    TNC_MODE = 0,    // Traditional KISS TNC mode
    APRS_TRACKER = 1 // APRS Tracker mode
};

// APRS symbol structure
struct APRSSymbol
{
    char table;  // '/' for primary table, '\' for alternate
    char symbol; // Symbol character
};

struct APRSConfig
{
    OperatingMode mode = OperatingMode::TNC_MODE; // Operating mode
    char callsign[10] = "N0CALL";                 // Station callsign
    uint8_t ssid = 9;                             // SSID (0-15), 9 is common for mobile
    uint32_t beaconInterval = 300;                // Beacon interval in seconds (5 minutes default)
    char path[32] = "WIDE1-1,WIDE2-1";            // Digipeater path
    char comment[64] = "LoRa APRS Tracker";       // Status comment
    APRSSymbol symbol = {'/', '>'};               // APRS symbol (mobile by default)

    // Smart beaconing parameters
    bool smartBeaconing = true;   // Enable smart beaconing
    uint32_t fastInterval = 60;   // Fast beacon interval (1 minute) when moving
    uint32_t slowInterval = 1800; // Slow beacon interval (30 minutes) when stopped
    float speedThreshold = 3.0;   // Speed threshold in km/h to trigger fast beaconing
    float minDistance = 100.0;    // Minimum distance in meters to trigger beacon

    // Position reporting
    bool includeAltitude = true; // Include altitude in position reports
    bool includeSpeed = true;    // Include speed in position reports
    bool includeCourse = true;   // Include course in position reports
};

struct WiFiConfig
{
    bool useAP = false; // false = STA-first mode with AP fallback, true = AP only
    char ssid[32] = "LoRaTNCX";
    char password[64] = "tncpass123";
    char sta_ssid[32] = "";     // For STA mode - empty means AP fallback
    char sta_password[64] = ""; // For STA mode
};

struct RadioConfig
{
    float frequency = 915.0;     // MHz
    float bandwidth = 125.0;     // kHz
    uint8_t spreadingFactor = 9; // 7-12
    uint8_t codingRate = 7;      // 5-8 (represents 4/5 to 4/8)
    int8_t txPower = 8;          // dBm

    // KISS timing parameters
    uint8_t txDelay = 30;  // TX delay in 10ms units (300ms default)
    uint8_t persist = 63;  // Persistence (0-255, 63 = ~25%)
    uint8_t slotTime = 10; // Slot time in 10ms units (100ms default)
};

struct GNSSConfig
{
    bool enabled = true;               // GNSS on/off
    uint32_t baudRate = 9600;          // GNSS baud rate
    bool routeToTcp = true;            // Route NMEA to TCP port 10110
    bool routeToUsb = false;           // Route NMEA to USB CDC (Serial)
    bool synthesizeOnSilence = true;   // Generate synthetic NMEA when GNSS goes silent
    uint32_t silenceTimeoutMs = 30000; // Consider GNSS silent after 30 seconds
    bool verboseLogging = false;       // Enable verbose GNSS debug logging
};

struct BatteryConfig
{
    bool debugMessages = false;        // Show battery diagnostic messages in serial console
};

class ConfigManager
{
public:
    bool begin();
    void saveConfig();
    void loadConfig();
    void resetToDefaults();

    // Configuration access
    WiFiConfig &getWiFiConfig() { return wifiConfig; }
    RadioConfig &getRadioConfig() { return radioConfig; }
    GNSSConfig &getGNSSConfig() { return gnssConfig; }
    APRSConfig &getAPRSConfig() { return aprsConfig; }
    BatteryConfig &getBatteryConfig() { return batteryConfig; }

    // Menu system
    void showMenu();
    void handleMenuInput();
    void powerOffDevice();
    bool inMenu = false;

private:
    Preferences prefs;
    WiFiConfig wifiConfig;
    RadioConfig radioConfig;
    GNSSConfig gnssConfig;
    APRSConfig aprsConfig;
    BatteryConfig batteryConfig;

    String inputBuffer;

    // Menu handlers
    void showMainMenu();
    void handleWiFiMenu();
    void handleRadioMenu();
    void handleGNSSMenu();
    void handleAPRSMenu();
    void handleBatteryMenu();
    void showCurrentConfig();

    // Helper functions
    float parseFloat(const String &str, float defaultVal = 0.0);
    int parseInt(const String &str, int defaultVal = 0);
    bool parseYesNo(const String &str);
    String waitForInput(unsigned long timeoutMs = 30000);
    String promptForString(const String &prompt, unsigned long timeoutMs = 30000);

    // Radio diagnostic functions
    void runRadioHealthCheck();
    void runHardwarePinCheck();
    void runTransmissionTest();
    void runContinuousTransmissionTest();
};

extern ConfigManager config;