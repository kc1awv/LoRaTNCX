/**
 * @file StationConfig.h
 * @brief Station Configuration Management for LoRaTNCX
 * @author LoRaTNCX Project
 * @date October 29, 2025
 *
 * Manages station-specific configuration including callsign, SSID, beacon settings,
 * location information, and amateur radio station identification requirements.
 */

#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// Station configuration limits
#define MAX_CALLSIGN_LENGTH 6
#define MAX_BEACON_TEXT_LENGTH 128
#define MAX_LOCATION_STRING_LENGTH 32
#define MAX_SSID_VALUE 15

// Beacon configuration
struct BeaconConfig
{
    bool enabled;
    uint16_t intervalSeconds;
    String text;
    bool includeLocation;
    bool includeTelemetry;
};

// Station location for APRS/position reporting
struct StationLocation
{
    bool valid;
    float latitude;    // Decimal degrees
    float longitude;   // Decimal degrees
    int16_t altitude;  // Meters above sea level
    String maidenhead; // Maidenhead grid square
};

// Station identification settings
struct StationID
{
    bool cwIdEnabled;
    bool voiceIdEnabled;
    uint16_t idIntervalMinutes;
    String idMessage;
};

// License class information for regulatory compliance
enum class LicenseClass
{
    UNKNOWN = 0,
    TECHNICIAN,
    GENERAL,
    AMATEUR_EXTRA,
    NOVICE,  // For historical/international use
    ADVANCED // For historical/international use
};

/**
 * Station Configuration Manager
 * Handles all station-specific settings and amateur radio compliance
 */
class StationConfig
{
public:
    StationConfig();

    // Initialization and persistence
    bool begin();
    bool save();
    bool load();
    void setDefaults();

    // Callsign management
    bool setCallsign(const String &callsign);
    String getCallsign() const { return callsign; }
    bool setSSID(uint8_t ssid);
    uint8_t getSSID() const { return ssid; }
    String getFullCallsign() const;
    bool validateCallsign(const String &call);

    // Beacon configuration
    bool setBeaconEnabled(bool enabled);
    bool isBeaconEnabled() const { return beaconConfig.enabled; }
    bool setBeaconInterval(uint16_t seconds);
    uint16_t getBeaconInterval() const { return beaconConfig.intervalSeconds; }
    bool setBeaconText(const String &text);
    String getBeaconText() const { return beaconConfig.text; }
    BeaconConfig getBeaconConfig() const { return beaconConfig; }

    // Station location
    bool setLocation(float lat, float lon, int16_t alt = 0);
    bool setLocationFromMaidenhead(const String &grid);
    StationLocation getLocation() const { return location; }
    String getLocationString() const;
    String getMaidenheadGrid() const;

    // Station identification
    bool setCWIDEnabled(bool enabled);
    bool isCWIDEnabled() const { return stationID.cwIdEnabled; }
    bool setIDInterval(uint16_t minutes);
    uint16_t getIDInterval() const { return stationID.idIntervalMinutes; }
    bool setIDMessage(const String &message);
    String getIDMessage() const { return stationID.idMessage; }

    // License information
    bool setLicenseClass(LicenseClass licenseClass);
    LicenseClass getLicenseClass() const { return licenseClass; }
    String getLicenseClassString() const;
    bool isFrequencyAuthorized(float frequency) const;
    int getMaxPowerForFrequency(float frequency) const; // Returns max power in dBm

    // Emergency mode
    bool setEmergencyMode(bool enabled);
    bool isEmergencyMode() const { return emergencyMode; }

    // Configuration validation
    bool isConfigurationValid() const;
    String getConfigurationStatus() const;

    // Configuration display
    String getStationInfo() const;
    String getFullStatus() const;

    // APRS-specific configuration
    bool setAPRSEnabled(bool enabled);
    bool isAPRSEnabled() const { return aprsEnabled; }
    bool setAPRSSymbol(char symbol, char overlay = '/');
    char getAPRSSymbol() const { return aprsSymbol; }
    char getAPRSOverlay() const { return aprsOverlay; }
    String generateAPRSBeacon() const;

    // Winlink configuration
    bool setWinlinkEnabled(bool enabled);
    bool isWinlinkEnabled() const { return winlinkEnabled; }
    bool setWinlinkGateway(const String &gateway);
    String getWinlinkGateway() const { return winlinkGateway; }

private:
    // Core station data
    String callsign;
    uint8_t ssid;
    BeaconConfig beaconConfig;
    StationLocation location;
    StationID stationID;
    LicenseClass licenseClass;
    bool emergencyMode;

    // Application-specific settings
    bool aprsEnabled;
    char aprsSymbol;
    char aprsOverlay;
    bool winlinkEnabled;
    String winlinkGateway;

    // Configuration persistence
    Preferences preferences;
    static const char *PREFS_NAMESPACE;

    // Utility functions
    String locationToMaidenhead(float lat, float lon) const;
    bool maidenheadToLocation(const String &grid, float &lat, float &lon) const;
    String formatLatitude(float lat) const;
    String formatLongitude(float lon) const;
    bool isValidLatitude(float lat) const;
    bool isValidLongitude(float lon) const;

    // Amateur radio band plans (US)
    struct BandPlan
    {
        float minFreq;
        float maxFreq;
        LicenseClass minLicense;
        int maxPower; // dBm
        String name;
    };

    static const BandPlan US_BAND_PLANS[];
    static const size_t NUM_BAND_PLANS;

    const BandPlan *findBandPlan(float frequency) const;
};

#endif // STATION_CONFIG_H