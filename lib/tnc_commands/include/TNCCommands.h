/**
 * @file TNCCommands.h
 * @brief LoRaTNCX command system for Arduino compatibility
 * @author LoRaTNCX Project
 * @date October 29, 2025
 *
 * Arduino-compatible command system that doesn't use STL containers.
 */

#ifndef TNC_COMMANDS_H
#define TNC_COMMANDS_H

#include <Arduino.h>

// Forward declaration to avoid circular includes
class LoRaRadio;

// Command execution return codes
enum class TNCCommandResult {
    SUCCESS,
    SUCCESS_SILENT,     // Success but don't send response (for KISS mode entry)
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

// Command system that works with Arduino
class TNCCommands {
public:
    TNCCommands();
    
    // Core command processing
    TNCCommandResult processCommand(const String& commandLine);
    
    // Mode management
    void setMode(TNCMode mode);
    TNCMode getMode() const { return currentMode; }
    TNCMode getCurrentMode() const { return currentMode; }  // Add alias for compatibility
    String getModeString() const;
    bool isInConverseMode() const;
    bool sendChatMessage(const String& message);

    // User interface helpers
    uint8_t getDebugLevel() const { return config.debugLevel; }
    bool isLocalEchoEnabled() const { return echoEnabled; }
    bool isLineEndingCREnabled() const { return config.lineEndingCR; }
    bool isLineEndingLFEnabled() const { return config.lineEndingLF; }
    bool isMonitorEnabled() const { return config.monitorEnabled; }
    
    // Response handling
    void sendResponse(const String& response);
    void sendPrompt();
    
    // Hardware integration
    void setRadio(LoRaRadio* radioPtr);
    
    // GNSS control callbacks
    typedef bool (*GNSSSetEnabledCallback)(bool enable);
    typedef bool (*GNSSGetEnabledCallback)();
    void setGNSSCallbacks(GNSSSetEnabledCallback setCallback, GNSSGetEnabledCallback getCallback);

    // OLED control callbacks
    typedef bool (*OLEDSetEnabledCallback)(bool enable);
    typedef bool (*OLEDGetEnabledCallback)();
    void setOLEDCallbacks(OLEDSetEnabledCallback setCallback, OLEDGetEnabledCallback getCallback);

    // Synchronize configuration with current peripheral state
    void setPeripheralStateDefaults(bool gnssEnabled, bool oledEnabled);
    bool getStoredGNSSEnabled() const { return config.gnssEnabled; }
    bool getStoredOLEDEnabled() const { return config.oledEnabled; }

    // Wi-Fi credential helpers
    bool hasWiFiCredentials() const { return config.wifiSSID.length() > 0 && config.wifiPassword.length() > 0; }
    String getWiFiSSID() const { return config.wifiSSID; }
    String getWiFiPassword() const { return config.wifiPassword; }
    void setWiFiCredentials(const String& ssid, const String& password);
    void clearWiFiCredentials();

    // UI credential helpers
    bool hasUICredentials() const { return config.uiUsername.length() > 0 && config.uiPassword.length() > 0; }
    String getUIUsername() const { return config.uiUsername; }
    String getUIPassword() const { return config.uiPassword; }
    void setUICredentials(const String& username, const String& password);
    void clearUICredentials();

    // UI theme helpers
    String getUIThemePreference() const { return config.uiThemePreference; }
    bool isUIThemeOverrideEnabled() const { return config.uiThemeOverride; }
    void setUIThemePreference(const String& theme, bool overrideEnabled);
    
    // Configuration management
    bool loadConfigurationFromFlash();
    bool saveConfigurationToFlash();
    
    // Packet processing (called from main loop)
    void processReceivedPacket(const String& packet, float rssi, float snr);

private:
    TNCMode currentMode;
    bool echoEnabled;
    bool promptEnabled;
    
    // Hardware reference
    LoRaRadio* radio;
    
    // GNSS control callbacks
    GNSSSetEnabledCallback gnssSetEnabledCallback;
    GNSSGetEnabledCallback gnssGetEnabledCallback;
    OLEDSetEnabledCallback oledSetEnabledCallback;
    OLEDGetEnabledCallback oledGetEnabledCallback;
    
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
        bool lineEndingCR;
        bool lineEndingLF;
        
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
        bool gnssEnabled;
        bool oledEnabled;

        // Web and connectivity
        String wifiSSID;
        String wifiPassword;
        String uiUsername;
        String uiPassword;
        String uiThemePreference;
        bool uiThemeOverride;
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
    
    // Routing table structures
    struct RouteEntry {
        String destination;  // Callsign of destination
        String nextHop;      // Next hop callsign
        uint8_t hops;        // Number of hops to destination
        float quality;       // Link quality (0.0-1.0)
        unsigned long lastUsed;  // Last time route was used
        unsigned long lastUpdated; // Last time route was updated
        bool isActive;       // Route is currently valid
    };
    
    static const int MAX_ROUTES = 32;
    RouteEntry routingTable[MAX_ROUTES];
    int routeCount;
    
    // Node/station tracking structures
    struct NodeEntry {
        String callsign;     // Station callsign
        uint8_t ssid;        // SSID if applicable
        float lastRSSI;      // Last received RSSI
        float lastSNR;       // Last received SNR
        unsigned long lastHeard;  // Last time heard
        unsigned long firstHeard; // First time heard
        uint32_t packetCount;     // Number of packets heard
        String lastPacket;   // Last packet content (truncated)
        bool isBeacon;       // Last packet was a beacon
    };
    
    static const int MAX_NODES = 64;
    NodeEntry nodeTable[MAX_NODES];
    int nodeCount;
    
    // Connection state management
    enum ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };
    
    struct ConnectionInfo {
        String remoteCall;       // Remote station callsign
        uint8_t remoteSSID;      // Remote station SSID
        ConnectionState state;   // Current connection state
        unsigned long connectTime;    // Time connection established
        unsigned long lastActivity;  // Last packet exchange
        uint8_t vs;              // Send sequence number (V(S))
        uint8_t vr;              // Receive sequence number (V(R))
        uint8_t va;              // Acknowledge sequence number (V(A))
        uint8_t retryCount;      // Current retry count
        bool pollBit;            // P/F bit state
    };
    
    static const int MAX_CONNECTIONS = 4;
    ConnectionInfo connections[MAX_CONNECTIONS];
    int activeConnections;
    
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
    TNCCommandResult handleSAVED(const String args[], int argCount);
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
    TNCCommandResult handleLINECR(const String args[], int argCount);
    TNCCommandResult handleLINELF(const String args[], int argCount);
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
    TNCCommandResult handleGNSS(const String args[], int argCount);
    TNCCommandResult handleOLED(const String args[], int argCount);
    TNCCommandResult handleSIMPLEX(const String args[], int argCount);
    
    // Utility functions
    String formatTime(unsigned long ms);
    String formatBytes(size_t bytes);
    TNCCommandResult transmitBeacon();
    void updateNodeTable(const String& callsign, uint8_t ssid, float rssi, float snr, const String& packet, bool isBeacon);
    bool shouldDigipeat(const String& path);
    String processDigipeatPath(const String& path);
    bool shouldProcessHop(const String& hop);
    bool sendDisconnectFrame(int connectionIndex);
    void processIncomingFrame(const String& frame, float rssi, float snr);
    void parseCallsign(const String& callsignWithSSID, String& callsign, uint8_t& ssid);
    int findConnection(const String& remoteCall, uint8_t remoteSSID);
    bool sendUAFrame(const String& remoteCall, uint8_t remoteSSID);
};

// Command mode constants
#define TNC_COMMAND_PROMPT "CMD:"
#define TNC_OK_RESPONSE "OK"
#define TNC_ERROR_RESPONSE "ERROR"

#endif // TNC_COMMANDS_H
