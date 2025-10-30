#include "CommandContext.h"

TNCCommandResult TNCCommands::handleLOAD(const String args[], int argCount) {
    sendResponse("Loading configuration from flash...");
    
    Preferences preferences;
    if (!preferences.begin("tnc_config", true)) {  // Read-only mode
        sendResponse("ERROR: Failed to open preferences storage");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
    
    try {
        // Station configuration
        config.myCall = preferences.getString("myCall", "NOCALL");
        config.mySSID = preferences.getUChar("mySSID", 0);
        config.beaconText = preferences.getString("beaconText", "LoRaTNCX Test Station");
        config.idEnabled = preferences.getBool("idEnabled", true);
        config.cwidEnabled = preferences.getBool("cwidEnabled", false);
        config.latitude = preferences.getFloat("latitude", 0.0);
        config.longitude = preferences.getFloat("longitude", 0.0);
        config.altitude = preferences.getInt("altitude", 0);
        config.gridSquare = preferences.getString("gridSquare", "");
        config.licenseClass = preferences.getString("licenseClass", "GENERAL");
        
        // Radio parameters
        config.frequency = preferences.getFloat("frequency", 915.0);
        config.txPower = preferences.getInt("txPower", 10);
        config.spreadingFactor = preferences.getUChar("spreadingFactor", 7);
        config.bandwidth = preferences.getFloat("bandwidth", 125.0);
        config.codingRate = preferences.getUChar("codingRate", 5);
        config.syncWord = preferences.getUChar("syncWord", 0x12);
        config.preambleLength = preferences.getUChar("preambleLength", 8);
        config.paControl = preferences.getBool("paControl", true);
        
        // Protocol stack
        config.txDelay = preferences.getUShort("txDelay", 300);
        config.txTail = preferences.getUShort("txTail", 100);
        config.persist = preferences.getUChar("persist", 63);
        config.slotTime = preferences.getUShort("slotTime", 100);
        config.respTime = preferences.getUShort("respTime", 1000);
        config.maxFrame = preferences.getUChar("maxFrame", 4);
        config.frack = preferences.getUShort("frack", 3000);
        config.retry = preferences.getUChar("retry", 10);
        
        // Operating modes
        config.echoEnabled = preferences.getBool("echoEnabled", true);
        config.promptEnabled = preferences.getBool("promptEnabled", true);
        config.monitorEnabled = preferences.getBool("monitorEnabled", false);
        
        // Beacon and digi
        config.beaconEnabled = preferences.getBool("beaconEnabled", false);
        config.beaconInterval = preferences.getUShort("beaconInterval", 600);
        config.digiEnabled = preferences.getBool("digiEnabled", false);
        config.digiPath = preferences.getUChar("digiPath", 4);
        
        preferences.end();
        
        // Apply loaded radio settings to hardware if radio is available
        if (radio != nullptr) {
            sendResponse("Applying loaded settings to radio hardware...");
            bool radioOk = true;
            
            // Apply radio parameters in sequence
            if (!radio->setFrequency(config.frequency)) {
                sendResponse("WARNING: Failed to set frequency on radio");
                radioOk = false;
            }
            if (!radio->setTxPower(config.txPower)) {
                sendResponse("WARNING: Failed to set TX power on radio");
                radioOk = false;
            }
            if (!radio->setSpreadingFactor(config.spreadingFactor)) {
                sendResponse("WARNING: Failed to set spreading factor on radio");
                radioOk = false;
            }
            if (!radio->setBandwidth(config.bandwidth)) {
                sendResponse("WARNING: Failed to set bandwidth on radio");
                radioOk = false;
            }
            if (!radio->setCodingRate(config.codingRate)) {
                sendResponse("WARNING: Failed to set coding rate on radio");
                radioOk = false;
            }
            if (!radio->setSyncWord(config.syncWord)) {
                sendResponse("WARNING: Failed to set sync word on radio");
                radioOk = false;
            }
            
            if (radioOk) {
                sendResponse("Configuration loaded and applied to radio");
            } else {
                sendResponse("Configuration loaded with radio warnings");
            }
        } else {
            sendResponse("Configuration loaded (radio not available for hardware update)");
        }
        
        return TNCCommandResult::SUCCESS;
        
    } catch (...) {
        preferences.end();
        sendResponse("ERROR: Failed to load configuration");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }
}
