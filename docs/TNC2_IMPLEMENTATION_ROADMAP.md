# TNC-2 Command Implementation Roadmap for LoRaTNCX

## Overview

This document outlines the implementation plan for integrating classic TNC-2 commands into LoRaTNCX while maintaining compatibility with modern LoRa operations and existing functionality.

## Current State Analysis

### Existing Commands (Already Implemented)
- `help` - System help
- `status` - System status (needs TNC-2 compatibility)  
- `clear` - Clear screen
- `reset` - System reset
- `config` - Show TNC configuration
- `kiss` / `K` - Enter KISS mode ✅
- `beacon` - Basic beacon support (needs TNC-2 enhancement)
- `lora` subcommands - LoRa-specific configuration

### Missing Core TNC-2 Features
- Station callsign management (`MYcall`)
- Connection management (`Connect`, `CONOk`, `CStatus`)
- Monitor mode (`Monitor`, `MStamp`, `MHeard`)
- Converse/Transparent modes (`CONVers`, `TRANS`)
- Digipeater functionality (`DIGipeat`)
- Terminal control features (`Echo`, `Xflow`, etc.)

## Implementation Phases

### Phase 1: Core Station Identity & Basic TNC-2 Compatibility
**Priority: High | Estimated Time: 2-3 days**

#### 1.1 Station Configuration
```cpp
// Add to CommandProcessor.h
void handleMycall(const String& args);
void handleMyAlias(const String& args);
void handleBtext(const String& args);

// Add to LoRaTNC.h
class TNC2Config {
private:
    String myCall;
    String myAlias;
    String beaconText;
    bool echo;
    bool monitor;
    uint8_t maxFrame;
    uint8_t retry;
    uint16_t paclen;
    
public:
    // Getters/setters for all parameters
    void setMyCall(const String& call);
    String getMyCall() const { return myCall; }
    
    void loadFromNVS();
    void saveToNVS();
};
```

#### 1.2 Enhanced Beacon System
```cpp
// Upgrade existing beacon to TNC-2 style
enum BeaconMode {
    BEACON_OFF,
    BEACON_EVERY,    // Every N seconds
    BEACON_AFTER     // After N seconds of activity
};

void setBeaconMode(BeaconMode mode, uint16_t interval);
```

#### 1.3 Command Abbreviation System
```cpp
// Add command alias resolution
class CommandAliases {
private:
    struct Alias {
        String full;
        String abbrev;
    };
    std::vector<Alias> aliases;
    
public:
    void registerAlias(const String& full, const String& abbrev);
    String resolveCommand(const String& input);
};
```

### Phase 2: Monitor Mode & Station Tracking  
**Priority: High | Estimated Time: 2-3 days**

#### 2.1 Station Heard List
```cpp
class StationHeard {
public:
    String callsign;
    unsigned long lastHeard;
    int16_t lastRSSI;
    float lastSNR;
    uint8_t lastSF;
    uint16_t packetCount;
};

class HeardList {
private:
    std::vector<StationHeard> stations;
    static const size_t MAX_STATIONS = 50;
    
public:
    void addStation(const String& call, int16_t rssi, float snr, uint8_t sf);
    void printHeardList();
    void clearHeardList();
    StationHeard* findStation(const String& call);
};
```

#### 2.2 Monitor Mode Implementation
```cpp
class MonitorMode {
private:
    bool enabled;
    bool timeStamp;
    bool showHeader;
    std::vector<String> filters;  // Callsign filters
    
public:
    void setEnabled(bool enable) { enabled = enable; }
    void setTimeStamp(bool stamp) { timeStamp = stamp; }
    void addFilter(const String& pattern);
    bool shouldDisplay(const String& callsign);
    void displayPacket(const LoRaPacket& packet);
};
```

### Phase 3: Connection Management & Link Layer
**Priority: Medium | Estimated Time: 4-5 days**

#### 3.1 Virtual Connection System
```cpp
enum ConnectionState {
    CONN_DISCONNECTED,
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_DISCONNECTING
};

class LoRaConnection {
private:
    String remoteCall;
    String viaPaths[8];  // Digipeater path
    ConnectionState state;
    uint8_t windowSize;
    uint8_t nextFrameToSend;
    uint8_t nextFrameExpected;
    unsigned long lastActivity;
    
public:
    bool connect(const String& call, const String* via = nullptr);
    void disconnect();
    bool sendData(const uint8_t* data, size_t len);
    void handleAck(uint8_t frameNum);
    void handleFrame(const LoRaPacket& packet);
};
```

#### 3.2 Multi-Stream Support
```cpp
class StreamManager {
private:
    static const uint8_t MAX_STREAMS = 4;
    LoRaConnection streams[MAX_STREAMS];
    uint8_t activeStream;
    
public:
    LoRaConnection* getStream(uint8_t streamId);
    uint8_t allocateStream();
    void releaseStream(uint8_t streamId);
    void switchStream(uint8_t streamId);
    void printStatus();
};
```

### Phase 4: Converse & Transparent Modes
**Priority: Medium | Estimated Time: 3-4 days**

#### 4.1 Mode Manager
```cpp
enum TNCMode {
    MODE_COMMAND,
    MODE_CONVERSE,  
    MODE_TRANSPARENT,
    MODE_KISS
};

class ModeManager {
private:
    TNCMode currentMode;
    uint8_t escapeChar;      // Ctrl+C default
    String commandBuffer;
    
public:
    void setMode(TNCMode mode);
    TNCMode getMode() const { return currentMode; }
    bool processInput(char c);
    void sendToActiveConnection(const String& data);
};
```

#### 4.2 Flow Control System
```cpp
class FlowControl {
private:
    bool xonXoffEnabled;
    bool rtsEnabled;
    uint8_t xonChar;   // Ctrl+Q
    uint8_t xoffChar;  // Ctrl+S
    bool flowStopped;
    
public:
    void enableXonXoff(bool enable);
    void sendXon();
    void sendXoff(); 
    bool canSend() const;
    bool processControlChar(char c);
};
```

### Phase 5: Digipeater & Routing
**Priority: Low | Estimated Time: 3-4 days**

#### 5.1 Digipeater Engine
```cpp
class DigipeaterEngine {
private:
    bool enabled;
    String myAlias;
    uint8_t maxHops;
    std::vector<String> denyList;
    uint32_t packetsRepeated;
    uint32_t packetsDropped;
    
public:
    void setEnabled(bool enable) { enabled = enable; }
    bool shouldRepeat(const LoRaPacket& packet);
    void repeatPacket(LoRaPacket& packet);
    void printStatistics();
};
```

#### 5.2 Basic Routing Table
```cpp
struct Route {
    String destination;
    String nextHop;
    uint8_t hops;
    unsigned long lastUpdate;
    uint16_t quality;  // Based on RSSI/SNR
};

class RoutingTable {
private:
    std::vector<Route> routes;
    static const size_t MAX_ROUTES = 20;
    
public:
    void addRoute(const String& dest, const String& via, uint8_t hops);
    Route* findRoute(const String& dest);
    void updateQuality(const String& dest, int16_t rssi, float snr);
    void ageRoutes();  // Remove old routes
    void printRoutes();
};
```

### Phase 6: Advanced Features & Integration
**Priority: Low | Estimated Time: 5-7 days**

#### 6.1 APRS Integration
```cpp
class APRSProcessor {
private:
    bool enabled;
    String position;
    String symbol;
    String comment;
    uint16_t beaconInterval;
    
public:
    void parseAPRSFrame(const String& frame);
    String buildAPRSBeacon();
    void setPosition(float lat, float lon);
    void setSymbol(const String& table, char symbol);
};
```

#### 6.2 Terminal Enhancement
```cpp
class TerminalControl {
private:
    uint8_t screenWidth;
    uint8_t characterLength;  // 7 or 8 bit
    bool localEcho;
    bool autoLF;
    uint8_t deleteChar;
    uint8_t cancelLine;
    
public:
    void processTerminalChar(char c);
    void sendToTerminal(const String& text);
    void handleBackspace();
    void handleDeleteLine();
    void setScreenWidth(uint8_t width);
};
```

## Command Implementation Priority Matrix

### Critical Path (Must Have)
1. `MYcall` - Station identification
2. `KISS` - Already working ✅  
3. `Monitor` - RF monitoring
4. `Connect` - Basic connection
5. `Beacon` - Enhanced beacon system

### High Priority (Should Have)
6. `MHeard` - Station tracking
7. `MStamp` - Timestamping
8. `CONVers` - Converse mode  
9. `Echo` - Terminal control
10. `MYAlias` - Digipeater ID

### Medium Priority (Nice to Have)
11. `DIGipeat` - Digipeater function
12. `MAXframe` - Window size
13. `RETry` - Retry control
14. `FRack` - ACK timing
15. `Paclen` - Packet length

### Low Priority (Future Enhancement)
16. `TRANS` - Transparent mode
17. `Unproto` - UI frame path
18. `Xflow` - XON/XOFF control
19. `CONOk` - Connection control
20. Advanced routing features

## File Structure Changes

### New Header Files
```
include/
├── TNC2Config.h          # TNC-2 configuration management
├── StationHeard.h        # Heard station tracking  
├── ConnectionManager.h   # Multi-stream connections
├── ModeManager.h         # Command/Converse/Trans modes
├── DigipeaterEngine.h    # Digipeater functionality
├── APRSProcessor.h       # APRS frame handling
└── TerminalControl.h     # Enhanced terminal features
```

### New Source Files  
```
src/
├── TNC2Config.cpp
├── StationHeard.cpp
├── ConnectionManager.cpp
├── ModeManager.cpp
├── DigipeaterEngine.cpp
├── APRSProcessor.cpp
└── TerminalControl.cpp
```

### Enhanced Existing Files
```
CommandProcessor.h/.cpp   # Add TNC-2 command handlers
LoRaTNC.h/.cpp           # Integrate new subsystems
main.cpp                 # Mode switching, callbacks
```

## Configuration Storage

### NVS Keys for TNC-2 Parameters
```cpp
// Core identification
"tnc2_mycall"      // Station callsign
"tnc2_myalias"     // Digipeater alias
"tnc2_btext"       // Beacon text

// Operation parameters  
"tnc2_beacon_mode" // Beacon timing mode
"tnc2_beacon_int"  // Beacon interval
"tnc2_monitor"     // Monitor enable
"tnc2_mstamp"      // Monitor timestamps
"tnc2_echo"        // Terminal echo
"tnc2_maxframe"    // Window size
"tnc2_retry"       // Max retries
"tnc2_paclen"      // Packet length

// Advanced features
"tnc2_digipeat"    // Digipeater enable
"tnc2_conok"       // Allow connections
"tnc2_xflow"       // XON/XOFF flow control
```

## Testing Strategy

### Unit Tests
1. Command parsing and abbreviation resolution
2. Station heard list management
3. Connection state machine
4. Mode switching logic
5. KISS frame processing

### Integration Tests  
1. TNC-2 to LoRaTNCX communication
2. Multi-stream operation
3. Digipeater functionality
4. APRS frame compatibility
5. Terminal mode switching

### Compatibility Tests
1. Legacy packet radio software integration
2. KISS protocol compliance
3. Command abbreviation compatibility
4. Parameter range validation

## Migration Strategy

### Backward Compatibility
- All existing LoRaTNCX commands continue to work
- New TNC-2 commands are additive
- KISS mode remains fully compatible
- Configuration storage is versioned

### Default Behavior
- Ship with TNC-2 features disabled by default
- Provide configuration profiles:
  - `modern` - Current LoRaTNCX behavior
  - `tnc2` - Classic TNC-2 compatibility
  - `hybrid` - Best of both worlds

### Documentation Updates
- Update all command references
- Add TNC-2 migration guide
- Provide comparison matrix
- Include packet radio integration examples

This roadmap provides a structured approach to implementing comprehensive TNC-2 compatibility while preserving the modern LoRa advantages that make LoRaTNCX unique.