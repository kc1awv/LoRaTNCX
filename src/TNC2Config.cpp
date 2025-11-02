#include "TNC2Config.h"

TNC2Config::TNC2Config() {
    loadDefaults();
}

TNC2Config::~TNC2Config() {
    prefs.end();
}

void TNC2Config::begin() {
    prefs.begin("tnc2config", false);
    loadFromNVS();
}

void TNC2Config::loadDefaults() {
    myCall = "NOCALL";
    myAlias = "";
    beaconText = "";
    connectText = "";
    beaconInterval = 600;  // 10 minutes
    beaconEnabled = false;
    
    monitorEnabled = true;
    timestampEnabled = false;
    echoEnabled = true;
    xflowEnabled = true;
    
    maxFrame = 4;
    retryCount = 10;
    packetLength = 128;
    frackTime = 3;      // 3 * 100ms = 300ms
    respTime = 12;      // 12 * 100ms = 1200ms
    connectionOk = true;
    
    digipeatEnabled = true;
    
    // Flow control characters (standard TNC-2 defaults)
    xonChar = 0x11;     // Ctrl+Q
    xoffChar = 0x13;    // Ctrl+S
    commandChar = 0x03; // Ctrl+C
    
    // Terminal settings
    screenWidth = 80;
    characterLength = 8;
}

void TNC2Config::loadFromNVS() {
    myCall = prefs.getString("mycall", "NOCALL");
    myAlias = prefs.getString("myalias", "");
    beaconText = prefs.getString("btext", "");
    connectText = prefs.getString("ctext", "");
    beaconInterval = prefs.getUInt("beacon_int", 600);
    beaconEnabled = prefs.getBool("beacon_en", false);
    
    monitorEnabled = prefs.getBool("monitor", true);
    timestampEnabled = prefs.getBool("mstamp", false);
    echoEnabled = prefs.getBool("echo", true);
    xflowEnabled = prefs.getBool("xflow", true);
    
    maxFrame = prefs.getUChar("maxframe", 4);
    retryCount = prefs.getUChar("retry", 10);
    packetLength = prefs.getUShort("paclen", 128);
    frackTime = prefs.getUShort("frack", 3);
    respTime = prefs.getUShort("resptime", 12);
    connectionOk = prefs.getBool("conok", true);
    
    digipeatEnabled = prefs.getBool("digipeat", true);
    
    xonChar = prefs.getUChar("xon", 0x11);
    xoffChar = prefs.getUChar("xoff", 0x13);
    commandChar = prefs.getUChar("command", 0x03);
    
    screenWidth = prefs.getUChar("screen", 80);
    characterLength = prefs.getUChar("charlen", 8);
}

void TNC2Config::saveToNVS() {
    prefs.putString("mycall", myCall);
    prefs.putString("myalias", myAlias);
    prefs.putString("btext", beaconText);
    prefs.putString("ctext", connectText);
    prefs.putUInt("beacon_int", beaconInterval);
    prefs.putBool("beacon_en", beaconEnabled);
    
    prefs.putBool("monitor", monitorEnabled);
    prefs.putBool("mstamp", timestampEnabled);
    prefs.putBool("echo", echoEnabled);
    prefs.putBool("xflow", xflowEnabled);
    
    prefs.putUChar("maxframe", maxFrame);
    prefs.putUChar("retry", retryCount);
    prefs.putUShort("paclen", packetLength);
    prefs.putUShort("frack", frackTime);
    prefs.putUShort("resptime", respTime);
    prefs.putBool("conok", connectionOk);
    
    prefs.putBool("digipeat", digipeatEnabled);
    
    prefs.putUChar("xon", xonChar);
    prefs.putUChar("xoff", xoffChar);
    prefs.putUChar("command", commandChar);
    
    prefs.putUChar("screen", screenWidth);
    prefs.putUChar("charlen", characterLength);
}

void TNC2Config::resetToDefaults() {
    loadDefaults();
    saveToNVS();
}

bool TNC2Config::setMyCall(const String& call) {
    if (call.isEmpty() || call.equalsIgnoreCase("NOCALL")) {
        myCall = "NOCALL";
        return true;
    }
    
    if (!isValidCallsign(call)) {
        return false;
    }
    
    myCall = call;
    myCall.toUpperCase();
    return true;
}

void TNC2Config::setMyAlias(const String& alias) {
    myAlias = alias;
    myAlias.toUpperCase();
    // Limit to 6 characters like original TNC-2
    if (myAlias.length() > 6) {
        myAlias = myAlias.substring(0, 6);
    }
}

void TNC2Config::setBeaconText(const String& text) {
    // Limit to 120 characters like original TNC-2
    if (text.length() <= 120) {
        beaconText = text;
    } else {
        beaconText = text.substring(0, 120);
    }
}

void TNC2Config::setConnectText(const String& text) {
    // Limit to 120 characters like original TNC-2
    if (text.length() <= 120) {
        connectText = text;
    } else {
        connectText = text.substring(0, 120);
    }
}

void TNC2Config::setBeaconInterval(uint16_t seconds) {
    // Reasonable limits: 30 seconds to 24 hours
    if (seconds >= 30 && seconds <= 86400) {
        beaconInterval = seconds;
    }
}

void TNC2Config::setMaxFrame(uint8_t frames) {
    // TNC-2 standard: 1-7 frames
    if (frames >= 1 && frames <= 7) {
        maxFrame = frames;
    }
}

void TNC2Config::setRetryCount(uint8_t retries) {
    // TNC-2 standard: 0-15 retries
    if (retries <= 15) {
        retryCount = retries;
    }
}

void TNC2Config::setPacketLength(uint16_t length) {
    // LoRa packet limits: 1-255 bytes
    if (length >= 1 && length <= 255) {
        packetLength = length;
    }
}

void TNC2Config::setFrackTime(uint16_t time) {
    // TNC-2 standard: 1-15 units (each unit = 100ms)
    if (time >= 1 && time <= 250) {
        frackTime = time;
    }
}

void TNC2Config::setRespTime(uint16_t time) {
    // TNC-2 standard: 0-250 units (each unit = 100ms)
    if (time <= 250) {
        respTime = time;
    }
}

void TNC2Config::setScreenWidth(uint8_t width) {
    // Reasonable terminal widths
    if (width >= 40 && width <= 255) {
        screenWidth = width;
    }
}

void TNC2Config::setCharacterLength(uint8_t length) {
    if (length == 7 || length == 8) {
        characterLength = length;
    }
}

bool TNC2Config::isValidCallsign(const String& call) {
    if (call.length() < 3 || call.length() > 9) {
        return false;
    }
    
    // Basic callsign validation (letters, numbers, dash)
    // Must start with letter or number
    if (!isAlphaNumeric(call.charAt(0))) {
        return false;
    }
    
    bool hasDash = false;
    for (int i = 0; i < call.length(); i++) {
        char c = call.charAt(i);
        if (c == '-') {
            if (hasDash || i == 0 || i == call.length() - 1) {
                return false; // Multiple dashes or dash at start/end
            }
            hasDash = true;
        } else if (!isAlphaNumeric(c)) {
            return false;
        }
    }
    
    return true;
}

void TNC2Config::printConfiguration() {
    Serial.println();
    Serial.println("TNC-2 Configuration Parameters:");
    Serial.println("================================");
    Serial.printf("MYcall      %s\n", myCall.c_str());
    Serial.printf("MYAlias     %s\n", myAlias.isEmpty() ? "(none)" : myAlias.c_str());
    Serial.printf("Beacon      %s", beaconEnabled ? "EVERY" : "OFF");
    if (beaconEnabled) {
        Serial.printf(" %d", beaconInterval);
    }
    Serial.println();
    Serial.printf("BText       %s\n", beaconText.isEmpty() ? "(none)" : beaconText.c_str());
    Serial.printf("CText       %s\n", connectText.isEmpty() ? "(none)" : connectText.c_str());
    Serial.println();
    Serial.printf("Monitor     %s\n", monitorEnabled ? "ON" : "OFF");
    Serial.printf("MStamp      %s\n", timestampEnabled ? "ON" : "OFF");
    Serial.printf("Echo        %s\n", echoEnabled ? "ON" : "OFF");
    Serial.printf("Xflow       %s\n", xflowEnabled ? "ON" : "OFF");
    Serial.println();
    Serial.printf("CONOk       %s\n", connectionOk ? "ON" : "OFF");
    Serial.printf("DIGipeat    %s\n", digipeatEnabled ? "ON" : "OFF");
    Serial.printf("MAXframe    %d\n", maxFrame);
    Serial.printf("RETry       %d\n", retryCount);
    Serial.printf("Paclen      %d\n", packetLength);
    Serial.printf("FRack       %d\n", frackTime);
    Serial.printf("RESptime    %d\n", respTime);
    Serial.println();
    Serial.printf("Screenln    %d\n", screenWidth);
    Serial.printf("AWlen       %d\n", characterLength);
    Serial.printf("XON         $%02X\n", xonChar);
    Serial.printf("XOff        $%02X\n", xoffChar);
    Serial.printf("COMmand     $%02X\n", commandChar);
    Serial.println();
}

void TNC2Config::printParameter(const String& param) {
    String p = param;
    p.toUpperCase();
    
    if (p == "MYCALL" || p == "MY") {
        Serial.printf("MYcall %s\n", myCall.c_str());
    } else if (p == "MYALIAS" || p == "MYA") {
        Serial.printf("MYAlias %s\n", myAlias.isEmpty() ? "(none)" : myAlias.c_str());
    } else if (p == "BEACON" || p == "B") {
        Serial.printf("Beacon %s", beaconEnabled ? "EVERY" : "OFF");
        if (beaconEnabled) {
            Serial.printf(" %d", beaconInterval);
        }
        Serial.println();
    } else if (p == "BTEXT" || p == "BT") {
        Serial.printf("BText %s\n", beaconText.isEmpty() ? "(none)" : beaconText.c_str());
    } else if (p == "CTEXT" || p == "CT") {
        Serial.printf("CText %s\n", connectText.isEmpty() ? "(none)" : connectText.c_str());
    } else if (p == "MONITOR" || p == "M") {
        Serial.printf("Monitor %s\n", monitorEnabled ? "ON" : "OFF");
    } else if (p == "MSTAMP" || p == "MS") {
        Serial.printf("MStamp %s\n", timestampEnabled ? "ON" : "OFF");
    } else if (p == "ECHO" || p == "E") {
        Serial.printf("Echo %s\n", echoEnabled ? "ON" : "OFF");
    } else if (p == "XFLOW" || p == "XF") {
        Serial.printf("Xflow %s\n", xflowEnabled ? "ON" : "OFF");
    } else if (p == "CONOK" || p == "CONO") {
        Serial.printf("CONOk %s\n", connectionOk ? "ON" : "OFF");
    } else if (p == "DIGIPEAT" || p == "DIG") {
        Serial.printf("DIGipeat %s\n", digipeatEnabled ? "ON" : "OFF");
    } else if (p == "MAXFRAME" || p == "MAX") {
        Serial.printf("MAXframe %d\n", maxFrame);
    } else if (p == "RETRY" || p == "R") {
        Serial.printf("RETry %d\n", retryCount);
    } else if (p == "PACLEN" || p == "P") {
        Serial.printf("Paclen %d\n", packetLength);
    } else if (p == "FRACK" || p == "FR") {
        Serial.printf("FRack %d\n", frackTime);
    } else if (p == "RESPTIME" || p == "RESP") {
        Serial.printf("RESptime %d\n", respTime);
    } else if (p == "SCREENLN" || p == "SC") {
        Serial.printf("Screenln %d\n", screenWidth);
    } else if (p == "AWLEN" || p == "AW") {
        Serial.printf("AWlen %d\n", characterLength);
    } else {
        Serial.printf("Unknown parameter: %s\n", param.c_str());
        Serial.println("Use DISPLAY with no parameter to show all settings");
    }
}