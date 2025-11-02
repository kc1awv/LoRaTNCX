#pragma once

#include <Arduino.h>
#include <vector>

/**
 * @file FrequencyBands.h
 * @brief Frequency band configuration system for LoRaTNCX
 * 
 * This system provides predefined frequency bands for ISM and amateur radio use,
 * as well as regional configurations that can be extended by users.
 */

// Forward declarations
struct FrequencyBand;
class FrequencyBandManager;

/**
 * License types for frequency band usage
 */
enum class BandLicense
{
    ISM,            // ISM bands - no license required
    AMATEUR_RADIO,  // Amateur radio bands - license required
    CUSTOM          // User-defined - user responsible for legality
};

/**
 * Regional frequency band configuration
 */
struct FrequencyBand
{
    String name;                // Human-readable name
    String identifier;          // Short identifier (e.g., "ISM_915")
    float minFrequency;         // Minimum frequency in MHz
    float maxFrequency;         // Maximum frequency in MHz
    float defaultFrequency;     // Default frequency in MHz
    BandLicense license;        // License requirement
    String region;              // Geographic region (e.g., "US", "EU", "Global")
    String description;         // Usage description
    bool enabled;               // Whether this band is available

    FrequencyBand(const String& n, const String& id, float minFreq, float maxFreq, 
                  float defFreq, BandLicense lic, const String& reg, const String& desc)
        : name(n), identifier(id), minFrequency(minFreq), maxFrequency(maxFreq), 
          defaultFrequency(defFreq), license(lic), region(reg), description(desc), enabled(true) {}
};

/**
 * Frequency Band Manager
 * Manages available frequency bands and validates frequency selections
 */
class FrequencyBandManager
{
private:
    std::vector<FrequencyBand> availableBands;
    FrequencyBand* currentBand;
    float currentFrequency;
    bool spiffsInitialized;
    
    // SPIFFS initialization
    bool initializeSPIFFS();
    
    // Load predefined ISM and amateur bands
    void loadPredefinedBands();
    
    // Load user-defined regional bands from file
    void loadRegionalBandsFromFile();
    
public:
    FrequencyBandManager();
    ~FrequencyBandManager();
    
    // Initialize full system after setup() when SPIFFS is safe to use
    void initializeFullSystem();
    
    // Band management
    bool addBand(const FrequencyBand& band);
    bool removeBand(const String& identifier);
    bool enableBand(const String& identifier, bool enabled = true);
    
    // Band selection
    bool selectBand(const String& identifier);
    bool selectBandByFrequency(float frequency);
    bool setFrequency(float frequency);
    
    // Getters
    const FrequencyBand* getCurrentBand() const { return currentBand; }
    float getCurrentFrequency() const { return currentFrequency; }
    std::vector<FrequencyBand> getAvailableBands() const;
    std::vector<FrequencyBand> getBandsByLicense(BandLicense license) const;
    std::vector<FrequencyBand> getBandsByRegion(const String& region) const;
    
    // Validation
    bool isFrequencyValid(float frequency) const;
    bool isFrequencyInCurrentBand(float frequency) const;
    const FrequencyBand* findBandForFrequency(float frequency) const;
    
    // Information display
    void printAvailableBands() const;
    void printCurrentConfiguration() const;
    String getBandInfo(const String& identifier) const;
    
    // Configuration persistence
    bool saveConfiguration();
    bool loadConfiguration();
    
    // Regional band file management
    bool saveRegionalBands(const String& filename = "/regional_bands.json");
    bool loadRegionalBands(const String& filename = "/regional_bands.json");
};

/**
 * Predefined ISM Band Identifiers
 */
#define BAND_ISM_433     "ISM_433"
#define BAND_ISM_470_510 "ISM_470_510" 
#define BAND_ISM_863_870 "ISM_863_870"
#define BAND_ISM_902_928 "ISM_902_928"

/**
 * Predefined Amateur Radio Band Identifiers
 */
#define BAND_AMATEUR_70CM  "AMATEUR_70CM"
#define BAND_AMATEUR_33CM  "AMATEUR_33CM"
#define BAND_AMATEUR_23CM  "AMATEUR_23CM"
#define BAND_AMATEUR_FREE  "AMATEUR_FREE"

/**
 * Band configuration macros for easy access
 */
#define ISM_433_MIN      433.050f
#define ISM_433_MAX      433.920f
#define ISM_433_DEFAULT  433.500f

#define ISM_470_510_MIN      470.0f
#define ISM_470_510_MAX      510.0f
#define ISM_470_510_DEFAULT  490.0f

#define ISM_863_870_MIN      863.0f
#define ISM_863_870_MAX      870.0f
#define ISM_863_870_DEFAULT  868.0f

#define ISM_902_928_MIN      902.0f
#define ISM_902_928_MAX      928.0f
#define ISM_902_928_DEFAULT  915.0f

#define AMATEUR_70CM_MIN      420.0f
#define AMATEUR_70CM_MAX      450.0f
#define AMATEUR_70CM_DEFAULT  432.6f

#define AMATEUR_33CM_MIN      902.0f
#define AMATEUR_33CM_MAX      928.0f
#define AMATEUR_33CM_DEFAULT  906.0f

#define AMATEUR_23CM_MIN      1240.0f
#define AMATEUR_23CM_MAX      1300.0f
#define AMATEUR_23CM_DEFAULT  1290.0f

#define AMATEUR_FREE_MIN      144.0f
#define AMATEUR_FREE_MAX      1300.0f
#define AMATEUR_FREE_DEFAULT  915.0f

// Hardware capability defines (based on SX1262 specs)
#define HARDWARE_MIN_FREQ   150.0f   // SX1262 minimum
#define HARDWARE_MAX_FREQ   960.0f   // SX1262 maximum for most variants