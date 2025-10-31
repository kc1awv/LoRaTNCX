#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSAVE(const String args[], int argCount) {
    sendResponse("Saving configuration to flash...");
    
    Preferences preferences;
    if (!preferences.begin("tnc_config", false)) {
        sendResponse("ERROR: Failed to open preferences storage");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    try {
        // Station configuration
        preferences.putString("myCall", config.myCall);
        preferences.putUChar("mySSID", config.mySSID);
        preferences.putString("beaconText", config.beaconText);
        preferences.putBool("idEnabled", config.idEnabled);
        preferences.putBool("cwidEnabled", config.cwidEnabled);
        preferences.putFloat("latitude", config.latitude);
        preferences.putFloat("longitude", config.longitude);
        preferences.putInt("altitude", config.altitude);
        preferences.putString("gridSquare", config.gridSquare);
        preferences.putString("licenseClass", config.licenseClass);
        
        // Radio parameters
        preferences.putFloat("frequency", config.frequency);
        preferences.putInt("txPower", config.txPower);
        preferences.putUChar("spreadingFactor", config.spreadingFactor);
        preferences.putFloat("bandwidth", config.bandwidth);
        preferences.putUChar("codingRate", config.codingRate);
        preferences.putUChar("syncWord", config.syncWord);
        preferences.putUChar("preambleLength", config.preambleLength);
        preferences.putBool("paControl", config.paControl);
        
        // Protocol stack
        preferences.putUShort("txDelay", config.txDelay);
        preferences.putUShort("txTail", config.txTail);
        preferences.putUChar("persist", config.persist);
        preferences.putUShort("slotTime", config.slotTime);
        preferences.putUShort("respTime", config.respTime);
        preferences.putUChar("maxFrame", config.maxFrame);
        preferences.putUShort("frack", config.frack);
        preferences.putUChar("retry", config.retry);
        
        // Operating modes
        preferences.putBool("echoEnabled", config.echoEnabled);
        preferences.putBool("promptEnabled", config.promptEnabled);
        preferences.putBool("monitorEnabled", config.monitorEnabled);
        preferences.putBool("lineEndingCR", config.lineEndingCR);
        preferences.putBool("lineEndingLF", config.lineEndingLF);
        
        // Beacon and digi
        preferences.putBool("beaconEnabled", config.beaconEnabled);
        preferences.putUShort("beaconInterval", config.beaconInterval);
        preferences.putBool("digiEnabled", config.digiEnabled);
        preferences.putUChar("digiPath", config.digiPath);
        
        // System settings
        preferences.putUChar("debugLevel", config.debugLevel);
        preferences.putBool("gnssEnabled", config.gnssEnabled);
        preferences.putBool("oledEnabled", config.oledEnabled);

        preferences.end();
        sendResponse("Configuration saved to flash");
        return TNCCommandResult::SUCCESS;
        
    } catch (...) {
        preferences.end();
        sendResponse("ERROR: Failed to save configuration");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
