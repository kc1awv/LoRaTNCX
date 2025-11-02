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
    
    // Access to TNC-2 objects for other classes
    TNC2Config* getTNC2Config() { return tnc2Config; }
    StationHeard* getStationHeard() { return heardList; }
    
private:
    LoRaRadio* loraRadio;
    LoRaTNC* tnc;
    TNC2Config* tnc2Config;
    StationHeard* heardList;
    
    // Command abbreviation system
    std::map<String, String> commandAliases;
    void initializeAliases();
    String resolveCommand(const String& input);
    
    // Command parsing helpers
    String getCommand(const String& input);
    String getArguments(const String& input);
    
    // Command handlers
    bool handleSystemCommand(const String& cmd, const String& args);
    bool handleLoRaCommand(const String& cmd, const String& args);
    bool handleTncCommand(const String& cmd, const String& args);
    bool handleTNC2Command(const String& cmd, const String& args);
    
    // System command implementations
    void handleHelp();
    void handleClear();
    void handleReset();
    
    // LoRa command implementations
    void handleLoRaStatus();
    void handleLoRaConfig();
    void handleLoRaStats();
    void handleLoRaSend(const String& args);
    void handleLoRaRx();
    void handleLoRaFreq(const String& args);
    void handleLoRaPower(const String& args);
    void handleLoRaSf(const String& args);
    void handleLoRaBw(const String& args);
    void handleLoRaCr(const String& args);
    void handleLoRaBands(const String& args);
    void handleLoRaBand(const String& args);
    void handleLoRaSave();
    
    // TNC command implementations
    void handleTncStatus();
    void handleTncConfig();
    void handleTncKiss();
    void handleTncBeacon(const String& args);
    void handleTncCsma(const String& args);
    void handleTncTest();
    
    // TNC-2 compatible command implementations
    void handleMycall(const String& args);
    void handleMyAlias(const String& args);
    void handleBtext(const String& args);
    void handleCtext(const String& args);
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
    void handleDisplay(const String& args);
    
    // LoRa-enhanced TNC-2 commands
    void handleRssi(const String& args);
    void handleSnr(const String& args);
    void handleLinkqual(const String& args);
    
    // Utility methods
    void printTNC2Help();
    String formatOnOff(bool value);
    bool parseOnOff(const String& input, bool& result);
};

#endif