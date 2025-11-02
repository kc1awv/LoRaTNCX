# TNC-2 Command Implementation Examples

This document provides concrete code examples for implementing key TNC-2 commands in LoRaTNCX. These examples follow the existing code style and architecture.

## Core Command Framework Enhancement

### Enhanced CommandProcessor.h

```cpp
#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>
#include <map>
#include "TNC2Config.h"
#include "StationHeard.h"

// Forward declarations
class LoRaRadio;
class LoRaTNC;

class CommandProcessor {
public:
    CommandProcessor(LoRaRadio* radio, LoRaTNC* tncInstance);
    
    void printHeader();
    bool processCommand(const String& input);
    
private:
    LoRaRadio* loraRadio;
    LoRaTNC* tnc;
    TNC2Config* tnc2Config;
    StationHeard* heardList;
    
    // Command abbreviation system
    std::map<String, String> commandAliases;
    void initializeAliases();
    String resolveCommand(const String& input);
    
    // Existing command parsing helpers
    String getCommand(const String& input);
    String getArguments(const String& input);
    
    // Enhanced command routing
    bool handleSystemCommand(const String& cmd, const String& args);
    bool handleLoRaCommand(const String& cmd, const String& args);
    bool handleTncCommand(const String& cmd, const String& args);
    bool handleTNC2Command(const String& cmd, const String& args);  // NEW
    
    // TNC-2 compatible command implementations
    void handleMycall(const String& args);
    void handleMyAlias(const String& args);
    void handleBtext(const String& args);
    void handleBeaconTNC2(const String& args);
    void handleMonitorTNC2(const String& args);
    void handleMheard(const String& args);
    void handleMstamp(const String& args);
    void handleConnect(const String& args);
    void handleConok(const String& args);
    void handleCstatus(const String& args);
    void handleConvers(const String& args);
    void handleMaxframe(const String& args);
    void handleRetry(const String& args);
    void handlePaclen(const String& args);
    void handleFrack(const String& args);
    void handleResptime(const String& args);
    void handleEcho(const String& args);
    void handleXflow(const String& args);
    void handleDigipeat(const String& args);
    
    // LoRa-enhanced TNC-2 commands
    void handleLoraFreqTNC2(const String& args);
    void handleLoraPowerTNC2(const String& args);
    void handleRssi(const String& args);
    void handleSnr(const String& args);
    void handleLinkqual(const String& args);
};

#endif
```

### Command Alias Registration

```cpp
// CommandProcessor.cpp - initializeAliases() method
void CommandProcessor::initializeAliases() {
    // Core TNC-2 command aliases (capital letters show minimum abbreviation)
    commandAliases["MY"] = "MYcall";
    commandAliases["MYC"] = "MYcall";
    commandAliases["MYCA"] = "MYcall";
    commandAliases["MYCAL"] = "MYcall";
    
    commandAliases["MYA"] = "MYAlias";
    commandAliases["MYAL"] = "MYAlias";
    commandAliases["MYALI"] = "MYAlias";
    
    commandAliases["B"] = "Beacon";
    commandAliases["BE"] = "Beacon";
    commandAliases["BEA"] = "Beacon";
    
    commandAliases["BT"] = "BText";
    commandAliases["BTE"] = "BText";
    commandAliases["BTEX"] = "BText";
    
    commandAliases["M"] = "Monitor";
    commandAliases["MO"] = "Monitor";
    commandAliases["MON"] = "Monitor";
    
    commandAliases["MH"] = "MHeard";
    commandAliases["MHE"] = "MHeard";
    commandAliases["MHEA"] = "MHeard";
    
    commandAliases["MS"] = "MStamp";
    commandAliases["MST"] = "MStamp";
    commandAliases["MSTA"] = "MStamp";
    
    commandAliases["C"] = "Connect";
    commandAliases["CO"] = "Connect";
    commandAliases["CON"] = "Connect";
    
    commandAliases["CONO"] = "CONOk";
    commandAliases["CONOK"] = "CONOk";
    
    commandAliases["CS"] = "CStatus";
    commandAliases["CST"] = "CStatus";
    commandAliases["CSTA"] = "CStatus";
    
    commandAliases["CONV"] = "CONVers";
    commandAliases["CONVE"] = "CONVers";
    commandAliases["CONVER"] = "CONVers";
    
    commandAliases["K"] = "KISS";
    
    // LoRa-enhanced commands
    commandAliases["LF"] = "LORAfreq";
    commandAliases["LORAF"] = "LORAfreq";
    commandAliases["LP"] = "LORApower";
    commandAliases["LORAP"] = "LORApower";
    commandAliases["RS"] = "RSsi";
    commandAliases["SN"] = "SNr";
    commandAliases["LQ"] = "LINKqual";
}

String CommandProcessor::resolveCommand(const String& input) {
    String upperInput = input;
    upperInput.toUpperCase();
    
    auto it = commandAliases.find(upperInput);
    if (it != commandAliases.end()) {
        return it->second;
    }
    return input;  // Return original if no alias found
}
```

## TNC-2 Configuration Management

### TNC2Config.h

```cpp
#ifndef TNC2CONFIG_H
#define TNC2CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

class TNC2Config {
private:
    Preferences prefs;
    
    // Core TNC-2 parameters
    String myCall;
    String myAlias;
    String beaconText;
    uint16_t beaconInterval;  // seconds
    bool beaconEnabled;
    
    // Monitor parameters
    bool monitorEnabled;
    bool timestampEnabled;
    bool echoEnabled;
    bool xflowEnabled;
    
    // Link parameters  
    uint8_t maxFrame;
    uint8_t retryCount;
    uint16_t packetLength;
    uint16_t frackTime;     // frame ACK timeout
    uint16_t respTime;      // response time
    bool connectionOk;
    
    // Digipeater parameters
    bool digipeatEnabled;
    
    // Validation helpers
    bool isValidCallsign(const String& call);
    
public:
    TNC2Config();
    ~TNC2Config();
    
    // Initialization
    void begin();
    void loadDefaults();
    void loadFromNVS();
    void saveToNVS();
    
    // Station identification
    bool setMyCall(const String& call);
    String getMyCall() const { return myCall; }
    
    void setMyAlias(const String& alias) { myAlias = alias; }
    String getMyAlias() const { return myAlias; }
    
    // Beacon configuration
    void setBeaconText(const String& text);
    String getBeaconText() const { return beaconText; }
    
    void setBeaconEnabled(bool enabled) { beaconEnabled = enabled; }
    bool getBeaconEnabled() const { return beaconEnabled; }
    
    void setBeaconInterval(uint16_t seconds) { beaconInterval = seconds; }
    uint16_t getBeaconInterval() const { return beaconInterval; }
    
    // Monitor configuration
    void setMonitorEnabled(bool enabled) { monitorEnabled = enabled; }
    bool getMonitorEnabled() const { return monitorEnabled; }
    
    void setTimestampEnabled(bool enabled) { timestampEnabled = enabled; }
    bool getTimestampEnabled() const { return timestampEnabled; }
    
    void setEchoEnabled(bool enabled) { echoEnabled = enabled; }
    bool getEchoEnabled() const { return echoEnabled; }
    
    // Link parameters
    void setMaxFrame(uint8_t frames);
    uint8_t getMaxFrame() const { return maxFrame; }
    
    void setRetryCount(uint8_t retries);
    uint8_t getRetryCount() const { return retryCount; }
    
    void setPacketLength(uint16_t length);
    uint16_t getPacketLength() const { return packetLength; }
    
    void setFrackTime(uint16_t time) { frackTime = time; }
    uint16_t getFrackTime() const { return frackTime; }
    
    void setRespTime(uint16_t time) { respTime = time; }
    uint16_t getRespTime() const { return respTime; }
    
    void setConnectionOk(bool ok) { connectionOk = ok; }
    bool getConnectionOk() const { return connectionOk; }
    
    // Digipeater
    void setDigipeatEnabled(bool enabled) { digipeatEnabled = enabled; }
    bool getDigipeatEnabled() const { return digipeatEnabled; }
    
    // Display
    void printConfiguration();
};

#endif
```

### TNC2Config.cpp Implementation

```cpp
#include "TNC2Config.h"

TNC2Config::TNC2Config() {
    loadDefaults();
}

void TNC2Config::begin() {
    prefs.begin("tnc2config", false);
    loadFromNVS();
}

void TNC2Config::loadDefaults() {
    myCall = "NOCALL";
    myAlias = "";
    beaconText = "";
    beaconInterval = 600;  // 10 minutes
    beaconEnabled = false;
    
    monitorEnabled = true;
    timestampEnabled = false;
    echoEnabled = true;
    xflowEnabled = true;
    
    maxFrame = 4;
    retryCount = 10;
    packetLength = 128;
    frackTime = 3;
    respTime = 12;
    connectionOk = true;
    
    digipeatEnabled = true;
}

void TNC2Config::loadFromNVS() {
    myCall = prefs.getString("mycall", "NOCALL");
    myAlias = prefs.getString("myalias", "");
    beaconText = prefs.getString("btext", "");
    beaconInterval = prefs.getUInt("beacon_int", 600);
    beaconEnabled = prefs.getBool("beacon_en", false);
    
    monitorEnabled = prefs.getBool("monitor", true);
    timestampEnabled = prefs.getBool("mstamp", false);
    echoEnabled = prefs.getBool("echo", true);
    
    maxFrame = prefs.getUChar("maxframe", 4);
    retryCount = prefs.getUChar("retry", 10);
    packetLength = prefs.getUShort("paclen", 128);
    frackTime = prefs.getUShort("frack", 3);
    respTime = prefs.getUShort("resptime", 12);
    connectionOk = prefs.getBool("conok", true);
    
    digipeatEnabled = prefs.getBool("digipeat", true);
}

void TNC2Config::saveToNVS() {
    prefs.putString("mycall", myCall);
    prefs.putString("myalias", myAlias);
    prefs.putString("btext", beaconText);
    prefs.putUInt("beacon_int", beaconInterval);
    prefs.putBool("beacon_en", beaconEnabled);
    
    prefs.putBool("monitor", monitorEnabled);
    prefs.putBool("mstamp", timestampEnabled);
    prefs.putBool("echo", echoEnabled);
    
    prefs.putUChar("maxframe", maxFrame);
    prefs.putUChar("retry", retryCount);
    prefs.putUShort("paclen", packetLength);
    prefs.putUShort("frack", frackTime);
    prefs.putUShort("resptime", respTime);
    prefs.putBool("conok", connectionOk);
    
    prefs.putBool("digipeat", digipeatEnabled);
}

bool TNC2Config::setMyCall(const String& call) {
    if (!isValidCallsign(call)) {
        return false;
    }
    myCall = call;
    myCall.toUpperCase();
    return true;
}

void TNC2Config::setBeaconText(const String& text) {
    // Limit to 120 characters like original TNC-2
    if (text.length() <= 120) {
        beaconText = text;
    } else {
        beaconText = text.substring(0, 120);
    }
}

bool TNC2Config::isValidCallsign(const String& call) {
    if (call.length() < 3 || call.length() > 9) {
        return false;
    }
    
    // Basic callsign validation (letters, numbers, dash)
    for (int i = 0; i < call.length(); i++) {
        char c = call.charAt(i);
        if (!isAlphaNumeric(c) && c != '-') {
            return false;
        }
    }
    return true;
}

void TNC2Config::printConfiguration() {
    Serial.println("TNC-2 Configuration:");
    Serial.printf("  MYcall: %s\n", myCall.c_str());
    Serial.printf("  MYAlias: %s\n", myAlias.c_str());
    Serial.printf("  Beacon: %s", beaconEnabled ? "EVERY" : "OFF");
    if (beaconEnabled) {
        Serial.printf(" %d seconds\n", beaconInterval);
    } else {
        Serial.println();
    }
    Serial.printf("  BText: %s\n", beaconText.c_str());
    Serial.printf("  Monitor: %s\n", monitorEnabled ? "ON" : "OFF");
    Serial.printf("  MStamp: %s\n", timestampEnabled ? "ON" : "OFF");
    Serial.printf("  Echo: %s\n", echoEnabled ? "ON" : "OFF");
    Serial.printf("  CONOk: %s\n", connectionOk ? "ON" : "OFF");
    Serial.printf("  DIGipeat: %s\n", digipeatEnabled ? "ON" : "OFF");
    Serial.printf("  MAXframe: %d\n", maxFrame);
    Serial.printf("  RETry: %d\n", retryCount);
    Serial.printf("  Paclen: %d\n", packetLength);
    Serial.printf("  FRack: %d\n", frackTime);
    Serial.printf("  RESptime: %d\n", respTime);
}
```

## Station Heard List Implementation

### StationHeard.h

```cpp
#ifndef STATIONHEARD_H
#define STATIONHEARD_H

#include <Arduino.h>
#include <vector>

struct StationInfo {
    String callsign;
    unsigned long lastHeard;
    int16_t lastRSSI;
    float lastSNR;
    uint8_t lastSF;
    uint16_t packetCount;
    String lastMessage;
};

class StationHeard {
private:
    std::vector<StationInfo> stations;
    static const size_t MAX_STATIONS = 50;
    
public:
    void addStation(const String& callsign, int16_t rssi, float snr, 
                   uint8_t sf, const String& message = "");
    void updateStation(const String& callsign, int16_t rssi, float snr, 
                      uint8_t sf, const String& message = "");
    StationInfo* findStation(const String& callsign);
    void removeOldStations(unsigned long maxAge = 3600000); // 1 hour default
    void clearAll();
    void printHeardList(bool showTimestamp = false);
    size_t getCount() const { return stations.size(); }
    
private:
    String formatTimestamp(unsigned long timestamp);
};

#endif
```

### StationHeard.cpp

```cpp
#include "StationHeard.h"

void StationHeard::addStation(const String& callsign, int16_t rssi, float snr, 
                             uint8_t sf, const String& message) {
    // Check if station already exists
    StationInfo* existing = findStation(callsign);
    if (existing) {
        updateStation(callsign, rssi, snr, sf, message);
        return;
    }
    
    // Add new station
    StationInfo newStation;
    newStation.callsign = callsign;
    newStation.lastHeard = millis();
    newStation.lastRSSI = rssi;
    newStation.lastSNR = snr;
    newStation.lastSF = sf;
    newStation.packetCount = 1;
    newStation.lastMessage = message;
    
    // Remove oldest if at maximum capacity
    if (stations.size() >= MAX_STATIONS) {
        removeOldStations(0); // Remove oldest entry
        if (stations.size() >= MAX_STATIONS) {
            stations.erase(stations.begin());
        }
    }
    
    stations.push_back(newStation);
}

void StationHeard::updateStation(const String& callsign, int16_t rssi, float snr, 
                                uint8_t sf, const String& message) {
    StationInfo* station = findStation(callsign);
    if (station) {
        station->lastHeard = millis();
        station->lastRSSI = rssi;
        station->lastSNR = snr;
        station->lastSF = sf;
        station->packetCount++;
        if (!message.isEmpty()) {
            station->lastMessage = message;
        }
    }
}

StationInfo* StationHeard::findStation(const String& callsign) {
    for (auto& station : stations) {
        if (station.callsign.equalsIgnoreCase(callsign)) {
            return &station;
        }
    }
    return nullptr;
}

void StationHeard::printHeardList(bool showTimestamp) {
    if (stations.empty()) {
        Serial.println("No stations heard");
        return;
    }
    
    Serial.println("Stations Heard:");
    Serial.printf("%-10s %-8s %-6s %-4s %-4s %s\n", 
                  "Callsign", "Time", "RSSI", "SNR", "SF", "Count");
    Serial.println("----------------------------------------------------");
    
    for (const auto& station : stations) {
        String timeStr = showTimestamp ? formatTimestamp(station.lastHeard) : 
                        String((millis() - station.lastHeard) / 1000) + "s ago";
        
        Serial.printf("%-10s %-8s %4ddBm %4.1fdB SF%d %4d\n",
                      station.callsign.c_str(),
                      timeStr.c_str(),
                      station.lastRSSI,
                      station.lastSNR,
                      station.lastSF,
                      station.packetCount);
    }
}

String StationHeard::formatTimestamp(unsigned long timestamp) {
    unsigned long now = millis();
    unsigned long age = now - timestamp;
    
    if (age < 60000) { // Less than 1 minute
        return String(age / 1000) + "s";
    } else if (age < 3600000) { // Less than 1 hour
        return String(age / 60000) + "m";
    } else { // More than 1 hour
        return String(age / 3600000) + "h";
    }
}
```

## Command Handler Examples

### MYcall Command Implementation

```cpp
void CommandProcessor::handleMycall(const String& args) {
    if (args.length() == 0) {
        // Show current callsign
        Serial.printf("MYcall: %s\n", tnc2Config->getMyCall().c_str());
        return;
    }
    
    // Set new callsign
    String callsign = args;
    callsign.trim();
    callsign.toUpperCase();
    
    if (tnc2Config->setMyCall(callsign)) {
        Serial.printf("MYcall set to: %s\n", callsign.c_str());
        tnc2Config->saveToNVS();
    } else {
        Serial.println("Invalid callsign format");
        Serial.println("Format: CALL or CALL-SSID (3-9 characters)");
    }
}
```

### Beacon Command Implementation

```cpp
void CommandProcessor::handleBeaconTNC2(const String& args) {
    if (args.length() == 0) {
        // Show current beacon status
        if (tnc2Config->getBeaconEnabled()) {
            Serial.printf("Beacon: EVERY %d seconds\n", tnc2Config->getBeaconInterval());
        } else {
            Serial.println("Beacon: OFF");
        }
        return;
    }
    
    // Parse beacon command: E/A interval
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex == -1) {
        Serial.println("Usage: Beacon E/A <seconds> or Beacon OFF");
        return;
    }
    
    String mode = args.substring(0, spaceIndex);
    mode.toUpperCase();
    
    if (mode == "OFF") {
        tnc2Config->setBeaconEnabled(false);
        Serial.println("Beacon disabled");
    } else if (mode == "E" || mode == "EVERY") {
        int interval = args.substring(spaceIndex + 1).toInt();
        if (interval >= 30 && interval <= 3600) { // 30 seconds to 1 hour
            tnc2Config->setBeaconEnabled(true);
            tnc2Config->setBeaconInterval(interval);
            Serial.printf("Beacon enabled: Every %d seconds\n", interval);
        } else {
            Serial.println("Invalid interval (30-3600 seconds)");
        }
    } else if (mode == "A" || mode == "AFTER") {
        // After mode - beacon after period of inactivity
        int interval = args.substring(spaceIndex + 1).toInt();
        Serial.println("AFTER mode not yet implemented - using EVERY");
        tnc2Config->setBeaconEnabled(true);
        tnc2Config->setBeaconInterval(interval);
    } else {
        Serial.println("Usage: Beacon E/A <seconds> or Beacon OFF");
        return;
    }
    
    tnc2Config->saveToNVS();
}
```

### Monitor Command Implementation

```cpp
void CommandProcessor::handleMonitorTNC2(const String& args) {
    if (args.length() == 0) {
        // Show current monitor status
        Serial.printf("Monitor: %s\n", tnc2Config->getMonitorEnabled() ? "ON" : "OFF");
        return;
    }
    
    String setting = args;
    setting.toUpperCase();
    setting.trim();
    
    if (setting == "ON" || setting == "1") {
        tnc2Config->setMonitorEnabled(true);
        Serial.println("Monitor: ON");
    } else if (setting == "OFF" || setting == "0") {
        tnc2Config->setMonitorEnabled(false);
        Serial.println("Monitor: OFF");
    } else {
        Serial.println("Usage: Monitor ON/OFF");
        return;
    }
    
    tnc2Config->saveToNVS();
}
```

### MHeard Command Implementation

```cpp
void CommandProcessor::handleMheard(const String& args) {
    if (args.length() > 0) {
        String cmd = args;
        cmd.toUpperCase();
        if (cmd == "CLEAR") {
            heardList->clearAll();
            Serial.println("Heard list cleared");
            return;
        }
    }
    
    heardList->printHeardList(tnc2Config->getTimestampEnabled());
}
```

### Integration with Main Loop

```cpp
// In LoRaTNC::handleLoRaReceive() - add station to heard list
void LoRaTNC::handleLoRaReceive(uint8_t *payload, uint16_t size, int16_t rssi, float snr) {
    // Existing receive handling...
    
    // Extract callsign from payload (assuming AX.25-like format)
    String receivedData = String((char*)payload);
    String callsign = extractCallsign(receivedData); // Implement this function
    
    if (!callsign.isEmpty() && tnc2Config->getMonitorEnabled()) {
        // Add to heard list
        heardList->addStation(callsign, rssi, snr, loraRadio->getConfig().spreadingFactor, receivedData);
        
        // Display if monitoring enabled
        if (tnc2Config->getMonitorEnabled()) {
            String timestamp = tnc2Config->getTimestampEnabled() ? getTimestamp() + " " : "";
            Serial.printf("%s%s: %s (RSSI:%ddBm SNR:%.1fdB SF%d)\n", 
                         timestamp.c_str(), callsign.c_str(), receivedData.c_str(),
                         rssi, snr, loraRadio->getConfig().spreadingFactor);
        }
    }
    
    // Continue with existing processing...
}
```

This implementation provides a solid foundation for TNC-2 compatibility while maintaining the modern LoRa advantages of LoRaTNCX. The code follows the existing architecture and can be integrated incrementally.