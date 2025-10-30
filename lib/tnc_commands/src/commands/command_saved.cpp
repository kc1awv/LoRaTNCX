#include "CommandContext.h"

TNCCommandResult TNCCommands::handleSAVED(const String args[], int argCount) {
    if (argCount > 0) {
        sendResponse("Usage: SAVED");
        return TNCCommandResult::ERROR_TOO_MANY_ARGS;
    }

    Preferences preferences;
    if (!preferences.begin("tnc_config", true)) {
        sendResponse("ERROR: Failed to open preferences storage");
        return TNCCommandResult::ERROR_SYSTEM_ERROR;
    }

    bool hasSavedConfig = preferences.isKey("myCall") || preferences.isKey("frequency") ||
                          preferences.isKey("txPower");

    if (!hasSavedConfig) {
        preferences.end();
        sendResponse("No configuration saved in flash");
        return TNCCommandResult::SUCCESS;
    }

    sendResponse("Saved Configuration Snapshot");
    sendResponse("============================");

    // Station configuration
    sendResponse("Station:");
    sendResponse("  MYCALL       : " + preferences.getString("myCall", "NOCALL"));
    sendResponse("  MYSSID       : " + String(preferences.getUChar("mySSID", 0)));
    sendResponse("  BTEXT        : " + preferences.getString("beaconText", "LoRaTNCX Test Station"));
    sendResponse("  ID ENABLED   : " + String(preferences.getBool("idEnabled", true) ? "ON" : "OFF"));
    sendResponse("  CWID ENABLED : " + String(preferences.getBool("cwidEnabled", false) ? "ON" : "OFF"));
    sendResponse("  LAT/LON      : " + String(preferences.getFloat("latitude", 0.0), 5) + ", " +
                 String(preferences.getFloat("longitude", 0.0), 5));
    sendResponse("  ALTITUDE     : " + String(preferences.getInt("altitude", 0)) + " m");
    sendResponse("  GRID         : " + preferences.getString("gridSquare", ""));
    sendResponse("  LICENSE      : " + preferences.getString("licenseClass", "GENERAL"));

    // Radio parameters
    sendResponse("Radio:");
    sendResponse("  FREQUENCY    : " + String(preferences.getFloat("frequency", 915.0), 4) + " MHz");
    sendResponse("  POWER        : " + String(preferences.getInt("txPower", 10)) + " dBm");
    sendResponse("  SPREAD FACTOR: SF" + String(preferences.getUChar("spreadingFactor", 7)));
    sendResponse("  BANDWIDTH    : " + String(preferences.getFloat("bandwidth", 125.0), 3) + " kHz");
    sendResponse("  CODING RATE  : 4/" + String(preferences.getUChar("codingRate", 5)));
    sendResponse("  SYNC WORD    : 0x" + String(preferences.getUChar("syncWord", 0x12), HEX));
    sendResponse("  PREAMBLE     : " + String(preferences.getUChar("preambleLength", 8)) + " sym");
    sendResponse("  PA CONTROL   : " + String(preferences.getBool("paControl", true) ? "ON" : "OFF"));

    // Protocol stack
    sendResponse("AX.25:");
    sendResponse("  TXDELAY      : " + String(preferences.getUShort("txDelay", 300)) + " ms");
    sendResponse("  TXTAIL       : " + String(preferences.getUShort("txTail", 100)) + " ms");
    sendResponse("  PERSIST      : " + String(preferences.getUChar("persist", 63)));
    sendResponse("  SLOTTIME     : " + String(preferences.getUShort("slotTime", 100)) + " ms");
    sendResponse("  RESPTIME     : " + String(preferences.getUShort("respTime", 1000)) + " ms");
    sendResponse("  MAXFRAME     : " + String(preferences.getUChar("maxFrame", 4)));
    sendResponse("  FRACK        : " + String(preferences.getUShort("frack", 3000)) + " ms");
    sendResponse("  RETRY        : " + String(preferences.getUChar("retry", 10)));

    // Operating modes
    sendResponse("UI:");
    sendResponse("  ECHO         : " + String(preferences.getBool("echoEnabled", true) ? "ON" : "OFF"));
    sendResponse("  PROMPT       : " + String(preferences.getBool("promptEnabled", true) ? "ON" : "OFF"));
    sendResponse("  MONITOR      : " + String(preferences.getBool("monitorEnabled", false) ? "ON" : "OFF"));
    bool lineEndingCR = preferences.getBool("lineEndingCR", true);
    bool lineEndingLF = preferences.getBool("lineEndingLF", true);
    String lineEnding = lineEndingCR ? "CR" : "";
    if (lineEndingLF) {
        lineEnding += (lineEnding.length() > 0) ? "/LF" : "LF";
    }
    if (lineEnding.length() == 0) {
        lineEnding = "NONE";
    }
    sendResponse("  CR/LF        : " + lineEnding);

    // Beacon and digi
    sendResponse("Beacon/Digi:");
    sendResponse("  BEACON       : " + String(preferences.getBool("beaconEnabled", false) ? "ON" : "OFF"));
    sendResponse("  BEACON INT   : " + String(preferences.getUShort("beaconInterval", 600)) + " s");
    sendResponse("  DIGI         : " + String(preferences.getBool("digiEnabled", false) ? "ON" : "OFF"));
    sendResponse("  DIGI PATH    : " + String(preferences.getUChar("digiPath", 4)));

    // System
    sendResponse("System:");
    sendResponse("  DEBUG LEVEL  : " + String(preferences.getUChar("debugLevel", config.debugLevel)));

    preferences.end();
    return TNCCommandResult::SUCCESS;
}
