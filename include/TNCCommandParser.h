/**
 * @file TNCCommandParser.h
 * @brief TAPR TNC-2 style command interface for LoRaTNCX
 * @author LoRaTNCX Project  
 * @date October 28, 2025
 * 
 * Implements a TAPR TNC-2 compatible command interface with commands like:
 * - KISS ON / KISS OFF
 * - RESTART  
 * - Configuration parameters
 * - Status reporting
 */

#ifndef TNC_COMMAND_PARSER_H
#define TNC_COMMAND_PARSER_H

#include <Arduino.h>

// Command buffer size
#define TNC_CMD_BUFFER_SIZE     128
#define TNC_MAX_ARGS           10

// TNC Operating Modes
enum TNCMode {
    TNC_MODE_COMMAND,    // Human-readable command interface
    TNC_MODE_KISS        // Silent KISS protocol mode
};

/**
 * @brief TNC Command Parser Class
 * 
 * Provides a TAPR TNC-2 compatible command interface for configuration
 * and control of the LoRaTNCX device. Handles the transition between
 * command mode and KISS mode.
 */
class TNCCommandParser {
public:
    TNCCommandParser();
    
    // Initialization
    bool begin();
    
    // Command Processing
    void processInputStream(char c);
    bool hasCommand();
    bool processCommand();
    
    // Mode Management
    TNCMode getCurrentMode() const { return currentMode; }
    void setMode(TNCMode mode);
    bool getKissFlag() const { return kissOnFlag; }
    
    // Status
    void printBanner();
    void printHelp();
    void printStatus();
    
    // Callbacks (set by main application)
    void (*onKissModeEnter)() = nullptr;
    void (*onKissModeExit)() = nullptr;
    void (*onRestart)() = nullptr;
    
private:
    TNCMode currentMode;
    
    // Command line processing
    char cmdBuffer[TNC_CMD_BUFFER_SIZE];
    size_t cmdBufferIndex;
    bool commandReady;
    
    // Parsed command components
    String command;
    String args[TNC_MAX_ARGS];
    int argCount;
    
    // Configuration parameters
    String mycall;
    int txDelay;
    int persistence;  
    int slotTime;
    bool fullDuplex;
    int retry;
    int pacLen;
    int maxFrame;
    int respTime;
    int frack;
    
    // KISS mode flag (set by KISS ON, activated by RESTART)
    bool kissOnFlag;
    
    // Internal methods
    void parseCommand();
    void clearCommand();
    void processCmd();
    
    // Command handlers
    void handleKiss(const String& arg);
    void handleKissImmediate();
    void handleRestart();
    void handleMycall(const String& arg);
    void handleTxDelay(const String& arg);
    void handlePersist(const String& arg);
    void handleSlotTime(const String& arg);
    void handleFullDuplex(const String& arg);
    void handleRetry(const String& arg);
    void handlePacLen(const String& arg);
    void handleMaxFrame(const String& arg);
    void handleRespTime(const String& arg);
    void handleFrack(const String& arg);
    void handleDisplay();
    void handleBanner();
    void handleHelp();
    void handleVersion();
    void handleStatus();
    
    // Utility methods
    void sendResponse(const String& response);
    void sendPrompt();
    int parseInteger(const String& str, int defaultValue = 0);
    bool parseBoolean(const String& str);
};

#endif // TNC_COMMAND_PARSER_H