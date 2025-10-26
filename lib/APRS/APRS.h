#pragma once
#include <Arduino.h>
#include "Config.h"

// Forward declarations
class RadioHAL;
class GNSSDriver;

// Common APRS symbols
const APRSSymbol APRS_SYMBOL_HOUSE = {'/', '-'};     // House
const APRSSymbol APRS_SYMBOL_CAR = {'/', '>'};       // Car
const APRSSymbol APRS_SYMBOL_PERSON = {'/', '['};    // Person
const APRSSymbol APRS_SYMBOL_MOBILE = {'/', '>'};    // Mobile
const APRSSymbol APRS_SYMBOL_BALLOON = {'/', 'O'};   // Balloon
const APRSSymbol APRS_SYMBOL_AIRCRAFT = {'/', '\''}; // Aircraft
const APRSSymbol APRS_SYMBOL_SHIP = {'/', 's'};      // Ship
const APRSSymbol APRS_SYMBOL_JEEP = {'/', 'j'};      // Jeep
const APRSSymbol APRS_SYMBOL_TRUCK = {'/', 'k'};     // Truck

class APRSDriver
{
public:
    APRSDriver();

    // Initialization
    bool begin(RadioHAL *radio, GNSSDriver *gnss);
    void loadConfig();
    void saveConfig();

    // APRS packet creation
    String createPositionReport();
    String createStatusMessage(const String &status);
    String createMessage(const String &destination, const String &message);

    // Beacon management
    void poll();
    bool shouldBeacon();
    void sendBeacon();
    void sendStatusBeacon(const String &status = "");

    // Smart beaconing
    void updateSmartBeaconing();
    bool hasMovedSignificantly();
    float calculateDistance(double lat1, double lng1, double lat2, double lng2);

    // Configuration access - use ConfigManager instead
    // APRSConfig& getConfig() - use config.getAPRSConfig() instead

    // Status information
    unsigned long getLastBeaconTime() const { return lastBeaconTime; }
    unsigned long getNextBeaconTime() const;
    uint32_t getCurrentInterval() const { return currentBeaconInterval; }
    bool isMoving() const { return moving; }
    float getCurrentSpeed() const { return currentSpeed; }

    // Statistics
    struct APRSStats
    {
        uint32_t beaconsSent = 0;
        uint32_t positionReports = 0;
        uint32_t statusMessages = 0;
        unsigned long uptime = 0;
        unsigned long lastBeacon = 0;
    };

    const APRSStats &getStats() const { return stats; }
    void resetStats();

private:
    RadioHAL *radio;
    GNSSDriver *gnss;
    APRSStats stats;

    // Beacon timing
    unsigned long lastBeaconTime = 0;
    uint32_t currentBeaconInterval;

    // Smart beaconing state
    bool moving = false;
    float currentSpeed = 0.0;
    double lastBeaconLat = 0.0;
    double lastBeaconLng = 0.0;
    bool hasValidLastPosition = false;
    unsigned long lastMovementTime = 0;

    // Position encoding
    String encodePosition(double lat, double lng, const APRSSymbol &symbol);
    String encodeLatitude(double lat);
    String encodeLongitude(double lng);
    String encodeBase91(uint32_t value, int digits);

    // Packet formatting
    String formatAX25Address(const String &callsign, uint8_t ssid, bool lastAddress = false);
    String createAX25Packet(const String &source, const String &destination,
                            const String &path, const String &payload);

    // Utility functions
    String padCallsign(const String &call);
    uint8_t calculateChecksum(const String &data);
    bool isValidCallsign(const String &call);

    // Configuration helpers (config managed externally by ConfigManager)
};

// Global APRS driver instance
extern APRSDriver aprs;