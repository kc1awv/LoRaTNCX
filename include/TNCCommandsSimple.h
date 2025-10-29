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
        // Station configuration
        String myCall;
        uint8_t mySSID;
        String beaconText;
        bool idEnabled;
        bool cwidEnabled;
        float latitude;
        float longitude;
        int altitude;
        String gridSquare;
        String licenseClass;
        
        // Radio parameters
        float frequency;
        int8_t txPower;
        uint8_t spreadingFactor;
        float bandwidth;
        uint8_t codingRate;
        uint16_t syncWord;
        uint8_t preambleLength;
        bool paControl;
        
        // Protocol stack
        uint16_t txDelay;
        uint16_t txTail;
        uint8_t persist;
        uint16_t slotTime;
        uint16_t respTime;
        uint8_t maxFrame;
        uint16_t frack;
        uint8_t retry;
        
        // Operating modes
        bool echoEnabled;
        bool promptEnabled;
        bool monitorEnabled;
        
        // Beacon and digi
        bool beaconEnabled;
        uint16_t beaconInterval;
        bool digiEnabled;
        uint8_t digiPath;
        
        // Amateur radio
        String band;
        String region;
        bool emergencyMode;
        bool aprsEnabled;
        String aprsSymbol;
        
        // Network
        String unprotoAddr;
        String unprotoPath;
        bool uidWait;
        bool mconEnabled;
        uint8_t maxUsers;
        bool flowControl;
        
        // System
        uint8_t debugLevel;
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
    
    // Station configuration commands
    TNCCommandResult handleMYSSID(const String args[], int argCount);
    TNCCommandResult handleBCON(const String args[], int argCount);
    TNCCommandResult handleBTEXT(const String args[], int argCount);
    TNCCommandResult handleID(const String args[], int argCount);
    TNCCommandResult handleCWID(const String args[], int argCount);
    TNCCommandResult handleLOCATION(const String args[], int argCount);
    TNCCommandResult handleGRID(const String args[], int argCount);
    TNCCommandResult handleLICENSE(const String args[], int argCount);
    
    // Extended radio parameter commands
    TNCCommandResult handlePREAMBLE(const String args[], int argCount);
    TNCCommandResult handlePRESET(const String args[], int argCount);
    TNCCommandResult handlePACTL(const String args[], int argCount);
    
    // Protocol stack commands
    TNCCommandResult handleTXTAIL(const String args[], int argCount);
    TNCCommandResult handlePERSIST(const String args[], int argCount);
    TNCCommandResult handleRETRY(const String args[], int argCount);
    
    // Operating mode commands
    TNCCommandResult handleTERMINAL(const String args[], int argCount);
    TNCCommandResult handleTRANSPARENT(const String args[], int argCount);
    TNCCommandResult handleECHO(const String args[], int argCount);
    TNCCommandResult handlePROMPT(const String args[], int argCount);
    TNCCommandResult handleCONNECT(const String args[], int argCount);
    TNCCommandResult handleDISCONNECT(const String args[], int argCount);
    
    // Extended monitoring commands
    TNCCommandResult handleMONITOR(const String args[], int argCount);
    TNCCommandResult handleMHEARD(const String args[], int argCount);
    TNCCommandResult handleTEMPERATURE(const String args[], int argCount);
    TNCCommandResult handleVOLTAGE(const String args[], int argCount);
    TNCCommandResult handleMEMORY(const String args[], int argCount);
    TNCCommandResult handleUPTIME(const String args[], int argCount);
    
    // LoRa-specific commands
    TNCCommandResult handleLORASTAT(const String args[], int argCount);
    TNCCommandResult handleTOA(const String args[], int argCount);
    TNCCommandResult handleRANGE(const String args[], int argCount);
    TNCCommandResult handleLINKTEST(const String args[], int argCount);
    TNCCommandResult handleSENSITIVITY(const String args[], int argCount);
    
    // Amateur radio specific commands
    TNCCommandResult handleBAND(const String args[], int argCount);
    TNCCommandResult handleREGION(const String args[], int argCount);
    TNCCommandResult handleCOMPLIANCE(const String args[], int argCount);
    TNCCommandResult handleEMERGENCY(const String args[], int argCount);
    TNCCommandResult handleAPRS(const String args[], int argCount);
    
    // Network and routing commands
    TNCCommandResult handleUNPROTO(const String args[], int argCount);
    TNCCommandResult handleUIDWAIT(const String args[], int argCount);
    TNCCommandResult handleUIDFRAME(const String args[], int argCount);
    TNCCommandResult handleMCON(const String args[], int argCount);
    TNCCommandResult handleUSERS(const String args[], int argCount);
    TNCCommandResult handleFLOW(const String args[], int argCount);
    
    // System configuration commands
    TNCCommandResult handleDEFAULT(const String args[], int argCount);
    TNCCommandResult handleQUIT(const String args[], int argCount);
    TNCCommandResult handleCALIBRATE(const String args[], int argCount);
    TNCCommandResult handleSELFTEST(const String args[], int argCount);
    TNCCommandResult handleDEBUG(const String args[], int argCount);
    TNCCommandResult handleSIMPLEX(const String args[], int argCount);
    
    // Utility functions
    String formatTime(unsigned long ms);
    String formatBytes(size_t bytes);
};

// Command mode constants
#define TNC_COMMAND_PROMPT "CMD:"
#define TNC_OK_RESPONSE "OK"
#define TNC_ERROR_RESPONSE "ERROR"

#endif // TNC_COMMANDS_SIMPLE_H