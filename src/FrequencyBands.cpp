#include "FrequencyBands.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

FrequencyBandManager::FrequencyBandManager() 
    : currentBand(nullptr), currentFrequency(0.0f), spiffsInitialized(false)
{
    // Only load predefined bands in constructor (no SPIFFS operations)
    loadPredefinedBands();
    
    // Select a safe default band without SPIFFS/Preferences operations
    for (auto& band : availableBands) {
        if (band.identifier == BAND_ISM_902_928 && band.enabled) {
            currentBand = &band;
            currentFrequency = band.defaultFrequency;
            Serial.printf("[FreqBand] Default band selected: %s (%.3f MHz)\n", 
                         band.name.c_str(), currentFrequency);
            break;
        }
    }
    
    // Note: SPIFFS and Preferences initialization will be done lazily when first needed
    Serial.println("[FreqBand] Frequency band manager created (deferred initialization mode)");
}

FrequencyBandManager::~FrequencyBandManager()
{
    saveConfiguration();
}

bool FrequencyBandManager::initializeSPIFFS()
{
    if (spiffsInitialized) {
        return true;
    }
    
    // Try to initialize SPIFFS without auto-formatting first
    spiffsInitialized = SPIFFS.begin(false);
    
    if (!spiffsInitialized) {
        Serial.println("[FreqBand] SPIFFS not formatted, trying with format...");
        // Only format if explicitly requested and safe to do so
        spiffsInitialized = SPIFFS.begin(true);
    }
    
    if (!spiffsInitialized) {
        Serial.println("[FreqBand] Warning: SPIFFS initialization failed - running without filesystem support");
        Serial.println("[FreqBand] Regional bands and configuration persistence will not be available");
    } else {
        Serial.println("[FreqBand] SPIFFS initialized successfully");
    }
    
    return spiffsInitialized;
}

void FrequencyBandManager::initializeFullSystem()
{
    // This method should be called after setup() when it's safe to use SPIFFS
    Serial.println("[FreqBand] Initializing full frequency band system...");
    
    // Initialize SPIFFS
    initializeSPIFFS();
    
    // Load regional bands and saved configuration
    loadRegionalBandsFromFile();
    loadConfiguration();
    
    Serial.println("[FreqBand] Full system initialization complete");
}

void FrequencyBandManager::loadPredefinedBands()
{
    // ISM Bands - No license required
    availableBands.push_back(FrequencyBand(
        "433 MHz ISM", BAND_ISM_433, 
        ISM_433_MIN, ISM_433_MAX, ISM_433_DEFAULT,
        BandLicense::ISM, "Global", 
        "433.05-433.92 MHz ISM band, globally available"
    ));
    
    availableBands.push_back(FrequencyBand(
        "470-510 MHz ISM", BAND_ISM_470_510,
        ISM_470_510_MIN, ISM_470_510_MAX, ISM_470_510_DEFAULT,
        BandLicense::ISM, "China/Asia",
        "470-510 MHz ISM band, primarily China and Asia"
    ));
    
    availableBands.push_back(FrequencyBand(
        "863-870 MHz ISM", BAND_ISM_863_870,
        ISM_863_870_MIN, ISM_863_870_MAX, ISM_863_870_DEFAULT,
        BandLicense::ISM, "EU",
        "863-870 MHz ISM band, European Union"
    ));
    
    availableBands.push_back(FrequencyBand(
        "902-928 MHz ISM", BAND_ISM_902_928,
        ISM_902_928_MIN, ISM_902_928_MAX, ISM_902_928_DEFAULT,
        BandLicense::ISM, "US/Canada",
        "902-928 MHz ISM band, North America"
    ));
    
    // Amateur Radio Bands - License required
    availableBands.push_back(FrequencyBand(
        "70cm Amateur", BAND_AMATEUR_70CM,
        AMATEUR_70CM_MIN, AMATEUR_70CM_MAX, AMATEUR_70CM_DEFAULT,
        BandLicense::AMATEUR_RADIO, "Global",
        "420-450 MHz Amateur Radio 70cm band"
    ));
    
    availableBands.push_back(FrequencyBand(
        "33cm Amateur", BAND_AMATEUR_33CM,
        AMATEUR_33CM_MIN, AMATEUR_33CM_MAX, AMATEUR_33CM_DEFAULT,
        BandLicense::AMATEUR_RADIO, "US",
        "902-928 MHz Amateur Radio 33cm band (US)"
    ));
    
    availableBands.push_back(FrequencyBand(
        "23cm Amateur", BAND_AMATEUR_23CM,
        AMATEUR_23CM_MIN, AMATEUR_23CM_MAX, AMATEUR_23CM_DEFAULT,
        BandLicense::AMATEUR_RADIO, "Global",
        "1240-1300 MHz Amateur Radio 23cm band"
    ));
    
    availableBands.push_back(FrequencyBand(
        "Amateur Radio Free", BAND_AMATEUR_FREE,
        AMATEUR_FREE_MIN, AMATEUR_FREE_MAX, AMATEUR_FREE_DEFAULT,
        BandLicense::AMATEUR_RADIO, "Global",
        "Free frequency selection within amateur allocations (operator responsibility)"
    ));
    
    Serial.printf("[FreqBand] Loaded %d predefined bands\n", availableBands.size());
}

void FrequencyBandManager::loadRegionalBandsFromFile()
{
    loadRegionalBands("/regional_bands.json");
}

bool FrequencyBandManager::addBand(const FrequencyBand& band)
{
    // Check for duplicate identifier
    for (const auto& existing : availableBands) {
        if (existing.identifier == band.identifier) {
            Serial.printf("[FreqBand] Band %s already exists\n", band.identifier.c_str());
            return false;
        }
    }
    
    // Validate frequency range
    if (band.minFrequency >= band.maxFrequency || 
        band.defaultFrequency < band.minFrequency || 
        band.defaultFrequency > band.maxFrequency) {
        Serial.println("[FreqBand] Invalid frequency range for new band");
        return false;
    }
    
    // Check hardware capability
    if (band.minFrequency < HARDWARE_MIN_FREQ || band.maxFrequency > HARDWARE_MAX_FREQ) {
        Serial.println("[FreqBand] Warning: Band exceeds hardware capability");
    }
    
    availableBands.push_back(band);
    Serial.printf("[FreqBand] Added band: %s\n", band.name.c_str());
    return true;
}

bool FrequencyBandManager::removeBand(const String& identifier)
{
    for (auto it = availableBands.begin(); it != availableBands.end(); ++it) {
        if (it->identifier == identifier) {
            // Don't allow removal if it's the current band
            if (currentBand && currentBand->identifier == identifier) {
                Serial.println("[FreqBand] Cannot remove currently selected band");
                return false;
            }
            
            Serial.printf("[FreqBand] Removed band: %s\n", it->name.c_str());
            availableBands.erase(it);
            return true;
        }
    }
    return false;
}

bool FrequencyBandManager::enableBand(const String& identifier, bool enabled)
{
    for (auto& band : availableBands) {
        if (band.identifier == identifier) {
            band.enabled = enabled;
            Serial.printf("[FreqBand] %s band: %s\n", 
                         enabled ? "Enabled" : "Disabled", band.name.c_str());
            return true;
        }
    }
    return false;
}

bool FrequencyBandManager::selectBand(const String& identifier)
{
    for (auto& band : availableBands) {
        if (band.identifier == identifier && band.enabled) {
            currentBand = &band;
            currentFrequency = band.defaultFrequency;
            
            Serial.printf("[FreqBand] Selected band: %s (%.3f MHz)\n", 
                         band.name.c_str(), currentFrequency);
            saveConfiguration();
            return true;
        }
    }
    
    Serial.printf("[FreqBand] Band not found or disabled: %s\n", identifier.c_str());
    return false;
}

bool FrequencyBandManager::selectBandByFrequency(float frequency)
{
    const FrequencyBand* band = findBandForFrequency(frequency);
    if (band && band->enabled) {
        currentBand = const_cast<FrequencyBand*>(band);
        currentFrequency = frequency;
        
        Serial.printf("[FreqBand] Auto-selected band: %s for %.3f MHz\n", 
                     band->name.c_str(), frequency);
        saveConfiguration();
        return true;
    }
    
    Serial.printf("[FreqBand] No suitable band found for %.3f MHz\n", frequency);
    return false;
}

bool FrequencyBandManager::setFrequency(float frequency)
{
    // Check if frequency is within current band
    if (currentBand && isFrequencyInCurrentBand(frequency)) {
        currentFrequency = frequency;
        Serial.printf("[FreqBand] Frequency set to %.3f MHz\n", frequency);
        saveConfiguration();
        return true;
    }
    
    // Try to find and switch to appropriate band
    if (selectBandByFrequency(frequency)) {
        return true;
    }
    
    // For amateur free band, allow any frequency within amateur ranges
    if (currentBand && currentBand->identifier == BAND_AMATEUR_FREE) {
        if (frequency >= AMATEUR_FREE_MIN && frequency <= AMATEUR_FREE_MAX) {
            currentFrequency = frequency;
            Serial.printf("[FreqBand] Amateur free frequency set to %.3f MHz\n", frequency);
            saveConfiguration();
            return true;
        }
    }
    
    Serial.printf("[FreqBand] Frequency %.3f MHz not allowed in current band configuration\n", frequency);
    return false;
}

std::vector<FrequencyBand> FrequencyBandManager::getAvailableBands() const
{
    std::vector<FrequencyBand> enabled;
    for (const auto& band : availableBands) {
        if (band.enabled) {
            enabled.push_back(band);
        }
    }
    return enabled;
}

std::vector<FrequencyBand> FrequencyBandManager::getBandsByLicense(BandLicense license) const
{
    std::vector<FrequencyBand> filtered;
    for (const auto& band : availableBands) {
        if (band.enabled && band.license == license) {
            filtered.push_back(band);
        }
    }
    return filtered;
}

std::vector<FrequencyBand> FrequencyBandManager::getBandsByRegion(const String& region) const
{
    std::vector<FrequencyBand> filtered;
    for (const auto& band : availableBands) {
        if (band.enabled && (band.region == region || band.region == "Global")) {
            filtered.push_back(band);
        }
    }
    return filtered;
}

bool FrequencyBandManager::isFrequencyValid(float frequency) const
{
    return findBandForFrequency(frequency) != nullptr;
}

bool FrequencyBandManager::isFrequencyInCurrentBand(float frequency) const
{
    if (!currentBand) return false;
    
    return (frequency >= currentBand->minFrequency && 
            frequency <= currentBand->maxFrequency);
}

const FrequencyBand* FrequencyBandManager::findBandForFrequency(float frequency) const
{
    for (const auto& band : availableBands) {
        if (band.enabled && 
            frequency >= band.minFrequency && 
            frequency <= band.maxFrequency) {
            return &band;
        }
    }
    return nullptr;
}

void FrequencyBandManager::printAvailableBands() const
{
    Serial.println("\n[FreqBand] Available Frequency Bands:");
    Serial.println("ID               Name                 Range (MHz)      License      Region    Description");
    Serial.println("---------------  -------------------  ---------------  -----------  --------  ------------------");
    
    for (const auto& band : availableBands) {
        if (!band.enabled) continue;
        
        const char* licenseStr = (band.license == BandLicense::ISM) ? "ISM" : 
                                (band.license == BandLicense::AMATEUR_RADIO) ? "Amateur" : "Custom";
        
        Serial.printf("%-15s  %-19s  %6.3f-%6.3f   %-11s  %-8s  %s\n",
                     band.identifier.c_str(),
                     band.name.c_str(),
                     band.minFrequency,
                     band.maxFrequency,
                     licenseStr,
                     band.region.c_str(),
                     band.description.c_str());
    }
    Serial.println();
}

void FrequencyBandManager::printCurrentConfiguration() const
{
    Serial.println("\n[FreqBand] Current Configuration:");
    if (currentBand) {
        const char* licenseStr = (currentBand->license == BandLicense::ISM) ? "ISM" : 
                                (currentBand->license == BandLicense::AMATEUR_RADIO) ? "Amateur" : "Custom";
        
        Serial.printf("  Band: %s (%s)\n", currentBand->name.c_str(), currentBand->identifier.c_str());
        Serial.printf("  Frequency: %.3f MHz\n", currentFrequency);
        Serial.printf("  Range: %.3f - %.3f MHz\n", currentBand->minFrequency, currentBand->maxFrequency);
        Serial.printf("  License: %s\n", licenseStr);
        Serial.printf("  Region: %s\n", currentBand->region.c_str());
        Serial.printf("  Description: %s\n", currentBand->description.c_str());
    } else {
        Serial.println("  No band selected");
    }
    Serial.println();
}

String FrequencyBandManager::getBandInfo(const String& identifier) const
{
    for (const auto& band : availableBands) {
        if (band.identifier == identifier) {
            const char* licenseStr = (band.license == BandLicense::ISM) ? "ISM" : 
                                    (band.license == BandLicense::AMATEUR_RADIO) ? "Amateur" : "Custom";
            
            return String(band.name) + " (" + String(band.minFrequency, 3) + "-" + 
                   String(band.maxFrequency, 3) + " MHz, " + licenseStr + ", " + band.region + ")";
        }
    }
    return "Band not found";
}

bool FrequencyBandManager::saveConfiguration()
{
    Preferences prefs;
    if (prefs.begin("freq_config", false)) {
        bool success = true;
        
        if (currentBand) {
            success &= prefs.putString("band_id", currentBand->identifier);
            success &= prefs.putFloat("frequency", currentFrequency);
            if (success) {
                Serial.printf("[FreqBand] Configuration saved: %s @ %.3f MHz\n", 
                             currentBand->identifier.c_str(), currentFrequency);
            }
        }
        
        prefs.end();
        return success;
    } else {
        Serial.println("[FreqBand] Warning: Cannot save configuration - NVS not available");
        return false;
    }
}

bool FrequencyBandManager::loadConfiguration()
{
    Preferences prefs;
    if (prefs.begin("freq_config", true)) {
        String bandId = prefs.getString("band_id", "");
        float freq = prefs.getFloat("frequency", 0.0f);
        prefs.end();
        
        if (!bandId.isEmpty() && freq > 0.0f) {
            if (selectBand(bandId)) {
                currentFrequency = freq;
                Serial.printf("[FreqBand] Loaded saved config: %s @ %.3f MHz\n", 
                             bandId.c_str(), freq);
                return true;
            }
        }
        Serial.println("[FreqBand] No saved configuration found, using defaults");
    } else {
        Serial.println("[FreqBand] NVS not available, using default configuration");
    }
    return false;
}

bool FrequencyBandManager::saveRegionalBands(const String& filename)
{
    if (!spiffsInitialized) {
        Serial.println("[FreqBand] SPIFFS not available for saving regional bands");
        return false;
    }
    
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.printf("[FreqBand] Failed to open %s for writing\n", filename.c_str());
        return false;
    }
    
    JsonDocument doc;
    JsonArray bandsArray = doc["regional_bands"].to<JsonArray>();
    
    for (const auto& band : availableBands) {
        // Only save custom/regional bands, not predefined ones
        if (band.license == BandLicense::CUSTOM || 
            (band.license != BandLicense::ISM && band.region != "Global")) {
            
            JsonObject bandObj = bandsArray.add<JsonObject>();
            bandObj["name"] = band.name;
            bandObj["identifier"] = band.identifier;
            bandObj["min_frequency"] = band.minFrequency;
            bandObj["max_frequency"] = band.maxFrequency;
            bandObj["default_frequency"] = band.defaultFrequency;
            bandObj["license"] = (int)band.license;
            bandObj["region"] = band.region;
            bandObj["description"] = band.description;
            bandObj["enabled"] = band.enabled;
        }
    }
    
    serializeJson(doc, file);
    file.close();
    
    Serial.printf("[FreqBand] Regional bands saved to %s\n", filename.c_str());
    return true;
}

bool FrequencyBandManager::loadRegionalBands(const String& filename)
{
    if (!spiffsInitialized) {
        Serial.println("[FreqBand] SPIFFS not available, skipping regional bands");
        return false;
    }
    
    if (!SPIFFS.exists(filename)) {
        Serial.printf("[FreqBand] Regional bands file %s not found\n", filename.c_str());
        return true; // Not an error, just no custom bands
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.printf("[FreqBand] Failed to open %s\n", filename.c_str());
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[FreqBand] JSON parse error in %s: %s\n", 
                     filename.c_str(), error.c_str());
        return false;
    }
    
    JsonArray bandsArray = doc["regional_bands"];
    int loadedCount = 0;
    
    for (JsonObject bandObj : bandsArray) {
        FrequencyBand band(
            bandObj["name"] | "",
            bandObj["identifier"] | "",
            bandObj["min_frequency"] | 0.0f,
            bandObj["max_frequency"] | 0.0f,
            bandObj["default_frequency"] | 0.0f,
            static_cast<BandLicense>(bandObj["license"] | 0),
            bandObj["region"] | "",
            bandObj["description"] | ""
        );
        
        band.enabled = bandObj["enabled"] | true;
        
        if (addBand(band)) {
            loadedCount++;
        }
    }
    
    Serial.printf("[FreqBand] Loaded %d regional bands from %s\n", 
                 loadedCount, filename.c_str());
    return true;
}