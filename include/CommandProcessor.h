#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>

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
    
    // Command parsing helpers
    String getCommand(const String& input);
    String getArguments(const String& input);
    
    // Command handlers
    bool handleSystemCommand(const String& cmd, const String& args);
    bool handleLoRaCommand(const String& cmd, const String& args);
    bool handleTncCommand(const String& cmd, const String& args);
    
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
};

#endif