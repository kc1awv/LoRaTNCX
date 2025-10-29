/**
 * @file TNCCommandsSimple.h
 * @brief Simplified TNC Command System for Arduino compatibility
 * @author LoRaTNCX Project
 * @date October 29, 2025
 *
 * Arduino-compatible command system that doesn't use STL containers.
 */

#ifndef TNC_COMMANDS_SIMPLE_H
#define TNC_COMMANDS_SIMPLE_H

#include <Arduino.h>

// Command execution return codes
enum class TNCCommandResult {
    SUCCESS,
    ERROR_UNKNOWN_COMMAND,
    ERROR_INVALID_PARAMETER,
    ERROR_SYSTEM_ERROR,
    ERROR_NOT_IMPLEMENTED,
    ERROR_INSUFFICIENT_ARGS,
    ERROR_TOO_MANY_ARGS,
    ERROR_INVALID_VALUE,
    ERROR_HARDWARE_ERROR
};

// TNC Operating Modes
enum class TNCMode {
    KISS_MODE,      // Binary KISS protocol mode
    COMMAND_MODE,   // Text command mode (CMD:)
    TERMINAL_MODE,  // Terminal/chat mode
    TRANSPARENT_MODE // Transparent/connected mode
};

// Simple command system that works with Arduino
class SimpleTNCCommands {
public:
    SimpleTNCCommands();
    
    // Core command processing
    TNCCommandResult processCommand(const String& commandLine);
    
    // Mode management
    void setMode(TNCMode mode);
    TNCMode getMode() const { return currentMode; }
    TNCMode getCurrentMode() const { return currentMode; }  // Add alias for compatibility
    String getModeString() const;
    
    // Response handling
    void sendResponse(const String& response);
    void sendPrompt();

private:
    TNCMode currentMode;
    bool echoEnabled;
    bool promptEnabled;
    
    // Configuration storage (Arduino-compatible)
    struct TNCConfig {
        String myCall;
        float frequency;
        int8_t txPower;
        uint8_t spreadingFactor;
        float bandwidth;
        uint8_t codingRate;
        uint16_t syncWord;
        uint16_t txDelay;
        uint16_t slotTime;
        uint16_t respTime;
        uint8_t maxFrame;
        uint16_t frack;
        bool beaconEnabled;
        uint16_t beaconInterval;
        bool digiEnabled;
        uint8_t digiPath;
        bool autoSave;
    } config;
    
    // Statistics storage
    struct TNCStats {
        uint32_t packetsTransmitted;
        uint32_t packetsReceived;
        uint32_t packetErrors;
        uint32_t bytesTransmitted;
        uint32_t bytesReceived;
        float lastRSSI;
        float lastSNR;
        unsigned long uptime;
    } stats;
    
    // Command parsing
    int parseCommandLine(const String& line, String args[], int maxArgs);
    String toUpperCase(const String& str);
    
    // Basic command handlers
    TNCCommandResult handleHELP(const String args[], int argCount);
    TNCCommandResult handleSTATUS(const String args[], int argCount);
    TNCCommandResult handleVERSION(const String args[], int argCount);
    TNCCommandResult handleMODE(const String args[], int argCount);
    TNCCommandResult handleMYCALL(const String args[], int argCount);
    
    // Radio configuration commands
    TNCCommandResult handleFREQ(const String args[], int argCount);
    TNCCommandResult handlePOWER(const String args[], int argCount);
    TNCCommandResult handleSF(const String args[], int argCount);
    TNCCommandResult handleBW(const String args[], int argCount);
    TNCCommandResult handleCR(const String args[], int argCount);
    TNCCommandResult handleSYNC(const String args[], int argCount);
    
    // Network and routing commands
    TNCCommandResult handleBEACON(const String args[], int argCount);
    TNCCommandResult handleDIGI(const String args[], int argCount);
    TNCCommandResult handleROUTE(const String args[], int argCount);
    TNCCommandResult handleNODES(const String args[], int argCount);
    
    // Protocol commands
    TNCCommandResult handleTXDELAY(const String args[], int argCount);
    TNCCommandResult handleSLOTTIME(const String args[], int argCount);
    TNCCommandResult handleRESPTIME(const String args[], int argCount);
    TNCCommandResult handleMAXFRAME(const String args[], int argCount);
    TNCCommandResult handleFRACK(const String args[], int argCount);
    
    // Statistics and monitoring
    TNCCommandResult handleSTATS(const String args[], int argCount);
    TNCCommandResult handleRSSI(const String args[], int argCount);
    TNCCommandResult handleSNR(const String args[], int argCount);
    TNCCommandResult handleLOG(const String args[], int argCount);
    
    // Configuration management
    TNCCommandResult handleSAVE(const String args[], int argCount);
    TNCCommandResult handleLOAD(const String args[], int argCount);
    TNCCommandResult handleRESET(const String args[], int argCount);
    TNCCommandResult handleFACTORY(const String args[], int argCount);
    
    // Testing and diagnostic commands
    TNCCommandResult handleTEST(const String args[], int argCount);
    TNCCommandResult handleCAL(const String args[], int argCount);
    TNCCommandResult handleDIAG(const String args[], int argCount);
    TNCCommandResult handlePING(const String args[], int argCount);
    
    // Utility functions
    String formatTime(unsigned long ms);
    String formatBytes(size_t bytes);
};

// Command mode constants
#define TNC_COMMAND_PROMPT "CMD:"
#define TNC_OK_RESPONSE "OK"
#define TNC_ERROR_RESPONSE "ERROR"

#endif // TNC_COMMANDS_SIMPLE_H