#pragma once

/**
 * @file CustomBandExamples.h
 * @brief Example implementations for adding custom frequency bands
 * 
 * This header provides examples of how users can programmatically add
 * custom frequency bands for specific regional or application needs.
 */

#include "FrequencyBands.h"

/**
 * Example: Add US-specific restricted ISM bands
 */
void addUSRestrictedBands(FrequencyBandManager* manager) {
    // FCC Part 15 restricted ISM band for low power devices
    FrequencyBand usFccPart15(
        "FCC Part 15 ISM (Restricted)",
        "US_FCC_PART15_902_928",
        902.0f,   // min frequency
        928.0f,   // max frequency  
        915.0f,   // default frequency
        BandLicense::ISM,
        "US",
        "FCC Part 15.247 ISM band with 1W EIRP limit"
    );
    
    manager->addBand(usFccPart15);
}

/**
 * Example: Add European ETSI bands
 */
void addEuropeanBands(FrequencyBandManager* manager) {
    // ETSI EN 300 220 SRD band
    FrequencyBand etsiSrd433(
        "ETSI SRD 433 MHz",
        "EU_ETSI_SRD_433",
        433.050f, // min frequency
        434.790f, // max frequency
        433.920f, // default frequency
        BandLicense::ISM,
        "EU",
        "ETSI EN 300 220 SRD band with 10mW ERP limit"
    );
    
    FrequencyBand etsiSrd868(
        "ETSI SRD 868 MHz",
        "EU_ETSI_SRD_868", 
        863.0f,   // min frequency
        870.0f,   // max frequency
        868.0f,   // default frequency
        BandLicense::ISM,
        "EU",
        "ETSI EN 300 220 SRD band with duty cycle restrictions"
    );
    
    manager->addBand(etsiSrd433);
    manager->addBand(etsiSrd868);
}

/**
 * Example: Add Japanese bands
 */
void addJapaneseBands(FrequencyBandManager* manager) {
    // Japan ISM 920 MHz band (ARIB STD-T108)
    FrequencyBand japanIsm920(
        "Japan ISM 920 MHz",
        "JP_ARIB_920_928",
        920.5f,   // min frequency
        928.1f,   // max frequency
        924.3f,   // default frequency
        BandLicense::ISM,
        "JP",
        "ARIB STD-T108 specific low power radio band"
    );
    
    manager->addBand(japanIsm920);
}

/**
 * Example: Add Australian bands 
 */
void addAustralianBands(FrequencyBandManager* manager) {
    // Australia ISM bands (ACMA regulations)
    FrequencyBand auIsm915(
        "Australia ISM 915 MHz", 
        "AU_ACMA_915_928",
        915.0f,   // min frequency
        928.0f,   // max frequency
        921.5f,   // default frequency
        BandLicense::ISM,
        "AU",
        "ACMA ISM band with 1W EIRP limit"
    );
    
    manager->addBand(auIsm915);
}

/**
 * Example: Add amateur radio subbands
 */
void addAmateurSubbands(FrequencyBandManager* manager) {
    // US Amateur 70cm digital subbands
    FrequencyBand amateur70cmDigital(
        "70cm Digital (US)",
        "AMATEUR_70CM_DIGITAL_US",
        420.0f,   // min frequency  
        450.0f,   // max frequency
        432.1f,   // default frequency (digital segment)
        BandLicense::AMATEUR_RADIO,
        "US",
        "US Amateur 70cm band digital modes segment"
    );
    
    // US Amateur 33cm weak signal segment  
    FrequencyBand amateur33cmWeak(
        "33cm Weak Signal (US)",
        "AMATEUR_33CM_WEAK_US", 
        902.0f,   // min frequency
        903.0f,   // max frequency  
        902.1f,   // default frequency
        BandLicense::AMATEUR_RADIO,
        "US",
        "US Amateur 33cm weak signal communications"
    );
    
    manager->addBand(amateur70cmDigital);
    manager->addBand(amateur33cmWeak);
}

/**
 * Example: Initialize all regional bands
 * Call this during setup to add region-specific bands
 */
void initializeRegionalBands(FrequencyBandManager* manager, const String& region) {
    if (region == "US" || region == "North America") {
        addUSRestrictedBands(manager);
        addAmateurSubbands(manager);
    }
    else if (region == "EU" || region == "Europe") {
        addEuropeanBands(manager);
    }
    else if (region == "JP" || region == "Japan") {
        addJapaneseBands(manager);
    }
    else if (region == "AU" || region == "Australia") {
        addAustralianBands(manager);
    }
    
    Serial.printf("[FreqBand] Initialized bands for region: %s\n", region.c_str());
}

/**
 * Example: Hardware-specific band filtering
 * Some hardware variants have different frequency capabilities
 */
void filterBandsByHardware(FrequencyBandManager* manager) {
    // Example: Disable 23cm bands if hardware doesn't support >1GHz
    #ifndef HARDWARE_SUPPORTS_1GHZ_PLUS
    manager->enableBand("AMATEUR_23CM", false);
    Serial.println("[FreqBand] Disabled 23cm bands - hardware limitation");
    #endif
    
    // Example: Disable 433MHz if using 915MHz-only hardware
    #ifdef HARDWARE_915_ONLY
    manager->enableBand("ISM_433", false);
    manager->enableBand("ISM_470_510", false);
    Serial.println("[FreqBand] Disabled sub-900MHz bands - hardware limitation");
    #endif
}