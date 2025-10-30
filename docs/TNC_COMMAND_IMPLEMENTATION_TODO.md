# TNC Command Implementation TODO

**Project**: LoRaTNCX - Comprehensive TNC Command System  
**Date Created**: October 29, 2025  
**Status**: Command framework complete, implementations needed  

## Overview

The comprehensive TNC command system has been successfully implemented with 73+ commands across 9 categories. All commands compile and the basic framework is operational. However, many commands currently have stub implementations or placeholder responses that need to be replaced with actual hardware integration and protocol functionality.

## Implementation Status Summary

- ✅ **Command Framework**: Complete (73+ commands)
- ✅ **Compilation**: All commands compile successfully
- ✅ **Basic Operation**: Command parsing and help system working
- ✅ **Hardware Integration**: Complete radio parameter control
- ✅ **Protocol Layer**: Core AX.25 packet handling implemented
- ✅ **Persistence**: Configuration save/load implemented

## Priority 1: Critical Hardware Integration (6 commands) ✅ COMPLETED

These commands now have full hardware integration with the `LoRaRadio` class:

### FREQ - Frequency Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~596)
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setFrequency()` method
- **Features**: 
  - Validates frequency range (902.0-928.0 MHz)
  - Updates radio hardware via reinitializing radio
  - Returns success confirmation or error message
  - Stores configuration for persistence

### POWER - TX Power Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~614)
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setTxPower()` method  
- **Features**:
  - Validates power range (-9 to 22 dBm)
  - Updates radio hardware directly (no reinit required)
  - Returns success confirmation or error message
  - Stores configuration for persistence

### SF - Spreading Factor Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~632) 
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setSpreadingFactor()` method
- **Features**:
  - Validates SF range (6-12)
  - Updates radio hardware via reinitializing radio
  - Returns success confirmation or error message
  - Stores configuration for persistence

### BW - Bandwidth Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~651)
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setBandwidth()` method
- **Features**:
  - Validates bandwidth against LoRa standard values
  - Updates radio hardware via reinitializing radio
  - Returns success confirmation or error message
  - Stores configuration for persistence

### CR - Coding Rate Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~669)
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setCodingRate()` method
- **Features**:
  - Validates coding rate range (5-8 for 4/5 to 4/8)
  - Updates radio hardware via reinitializing radio
  - Returns success confirmation or error message
  - Stores configuration for persistence

### SYNC - Sync Word Control ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~689)
- **Current Status**: ✅ **IMPLEMENTED** - Full hardware integration complete
- **Implementation**: Connected to `LoRaRadio::setSyncWord()` method
- **Features**:
  - Accepts hex (0x12) or decimal format
  - Updates radio hardware directly (no reinit required)
  - Returns success confirmation or error message
  - Stores configuration for persistence

### Priority 1 Implementation Summary:
- ✅ **Hardware Integration System**: Added `TNCCommands::setRadio()` method
- ✅ **LoRaRadio Extensions**: Added 6 setter methods and 6 getter methods
- ✅ **Parameter Storage**: All radio parameters tracked in LoRaRadio class
- ✅ **Error Handling**: Comprehensive validation and hardware error reporting
- ✅ **TNCManager Integration**: Radio reference automatically connected on startup

## Priority 2: Real-time Monitoring (2 commands) ✅ COMPLETED

Radio telemetry now provides live hardware data:

### RSSI - Signal Strength ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~930)
- **Current Status**: ✅ **IMPLEMENTED** - Real-time hardware data
- **Implementation**: Connected to existing `LoRaRadio::getRSSI()` method
- **Features**:
  - Returns actual RSSI from last received packet
  - Format: "Last RSSI: -85.2 dBm"
  - Error handling for radio unavailability
  - Direct hardware integration (no simulation)

### SNR - Signal-to-Noise Ratio ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~936)
- **Current Status**: ✅ **IMPLEMENTED** - Real-time hardware data
- **Implementation**: Connected to existing `LoRaRadio::getSNR()` method
- **Features**:
  - Returns actual SNR from last received packet
  - Format: "Last SNR: 12.5 dB"
  - Error handling for radio unavailability  
  - Direct hardware integration (no simulation)

## Priority 3: Configuration Persistence (2 commands) ✅ COMPLETED

Essential TNC configuration management now fully implemented:

### SAVE - Save Configuration ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~1079)
- **Current Status**: ✅ **IMPLEMENTED** - Full configuration persistence
- **Implementation**: Complete ESP32 Preferences-based storage system
- **Features**:
  - Saves all TNCConfig structure fields to flash memory
  - Comprehensive error handling with try/catch blocks  
  - Saves 35+ configuration parameters including station, radio, protocol, and operational settings
  - Uses ESP32 Preferences library for reliable non-volatile storage
  - Helper method `saveConfigurationToFlash()` for programmatic access

### LOAD - Load Configuration ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~1127)
- **Current Status**: ✅ **IMPLEMENTED** - Full configuration restoration with hardware sync
- **Implementation**: Complete ESP32 Preferences-based loading system with hardware integration
- **Features**:
  - Loads all configuration parameters with sensible defaults
  - Automatically applies loaded radio settings to hardware
  - Individual warnings for any hardware application failures
  - Comprehensive error handling and validation
  - Helper method `loadConfigurationFromFlash()` for programmatic access
  - Auto-detects if saved configuration exists

### Configuration Management System Features:
- ✅ **35+ Parameters**: Station info, radio settings, protocol stack, operational modes
- ✅ **Hardware Sync**: LOAD automatically applies radio settings to hardware
- ✅ **Error Recovery**: Comprehensive error handling and default fallbacks
- ✅ **Programmatic API**: Helper methods for startup auto-loading
- ✅ **Validation**: Parameter validation and hardware confirmation

## Priority 4: Packet Handling & Protocol (8 commands) ✅ COMPLETED

Core TNC functionality with complete packet protocol implementation:

### LINKTEST - Link Quality Testing ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~1851)
- **Current Status**: ✅ **IMPLEMENTED** - Complete ping/pong link testing
- **Implementation**: Full ping/pong style link quality testing with hardware integration
- **Features**:
  - Configurable test count (1-10 packets)
  - Real-time round-trip time measurement
  - RSSI and SNR reporting for received responses
  - Packet loss statistics and summary
  - 5-second timeout handling per ping
  - Leverages existing LoRaRadio transmit/receive methods

### UIDFRAME - UI Frame Transmission ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~2071)
- **Current Status**: ✅ **IMPLEMENTED** - AX.25 UI frame generation and transmission
- **Implementation**: Complete AX.25 UI frame creation and transmission system
- **Features**:
  - Simplified AX.25 UI frame format implementation
  - Supports unprotocol addressing and digipeater paths
  - Frame size validation (240 byte LoRa limit)
  - Multi-word message support
  - Statistics tracking (packets/bytes transmitted)
  - Error handling for hardware failures

### BEACON - Automatic Beaconing ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~851)
- **Current Status**: ✅ **IMPLEMENTED** - Enhanced beacon system with immediate transmission
- **Implementation**: Complete beacon transmission system with configuration management
- **Features**:
  - Beacon enable/disable with status display
  - Configurable interval (30-3600 seconds)
  - Custom beacon text configuration (up to 200 characters)
  - Manual beacon transmission (`BEACON NOW`)
  - Position reporting support (latitude/longitude/altitude)
  - APRS-style beacon format with timestamps
  - Helper method `transmitBeacon()` for programmatic access

### ROUTE - Routing Table ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~948)
- **Current Status**: ✅ **IMPLEMENTED** - Complete routing table management system
- **Implementation**: Full routing table with add/delete/display/purge functionality
- **Features**:
  - Add/delete/clear individual routes
  - Route quality tracking (0.0-1.0 scale)
  - Hop count management (1-7 hops)
  - Route aging and automatic purge functionality
  - Comprehensive route display with timing statistics
  - Maximum 32 routes supported with dynamic management
  - Route lookup and path optimization support

### NODES - Network Discovery ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~1148)
- **Current Status**: ✅ **IMPLEMENTED** - Network topology discovery and station tracking
- **Implementation**: Complete heard station database with signal quality tracking
- **Features**:
  - Heard station database (up to 64 nodes)
  - Real-time RSSI/SNR tracking per station
  - Packet count and timing statistics (first/last heard)
  - Node table clear and purge functionality (removes entries >60min old)
  - Last packet content preview (50 character truncation)
  - Helper method `updateNodeTable()` for automatic station tracking
  - Comprehensive display with formatted statistics table

### DIGI - Digipeater Functionality ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~940)
- **Current Status**: ✅ **IMPLEMENTED** - AX.25 digipeater with complete path processing
- **Implementation**: Full digipeater functionality with WIDE alias support and path processing
- **Features**:
  - Digipeater enable/disable with callsign validation
  - WIDE1-1, WIDE2-1, WIDEn-N alias support
  - Complete path parsing and hop decrementation algorithms
  - Direct callsign addressing support
  - Digipeater test functionality (`DIGI TEST <path>`)
  - Path processing methods: `shouldDigipeat()`, `processDigipeatPath()`, `shouldProcessHop()`
  - Statistics display and configuration management

### CONNECT - AX.25 Connection ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~2115)
- **Current Status**: ✅ **IMPLEMENTED** - AX.25 connection establishment protocol
- **Implementation**: Complete connection state management with multi-connection support
- **Features**:
  - Multi-connection support (up to 4 simultaneous connections)
  - Full connection state machine (DISCONNECTED/CONNECTING/CONNECTED/DISCONNECTING)
  - SABM (Set Asynchronous Balanced Mode) frame transmission
  - Connection status display with timing information
  - SSID support (0-15) for enhanced addressing
  - Connection duration tracking and activity monitoring
  - Comprehensive error handling and validation

### DISCONNECT - AX.25 Disconnection ✅ COMPLETED
- **File**: `lib/tnc_commands/src/TNCCommands.cpp` (line ~2125)
- **Current Status**: ✅ **IMPLEMENTED** - AX.25 disconnection protocol
- **Implementation**: Complete disconnection handling with proper state cleanup
- **Features**:
  - Individual station disconnect by callsign/SSID
  - Bulk disconnect functionality (disconnect all active connections)
  - DISC (Disconnect) frame transmission
  - Connection statistics reporting before disconnect
  - Proper connection state cleanup and resource management
  - Helper method `sendDisconnectFrame()` for programmatic access

### Priority 4 Implementation Summary:
- ✅ **Complete Protocol Stack**: All 8 packet handling commands fully implemented
- ✅ **Data Structures**: Added routing table (32 entries), node table (64 entries), connection table (4 connections)
- ✅ **Hardware Integration**: All commands properly interface with LoRaRadio class
- ✅ **Error Handling**: Comprehensive validation and hardware error reporting throughout
- ✅ **Statistics Integration**: Packet counts, error rates, and performance metrics
- ✅ **Memory Efficient**: Fixed-size arrays optimized for Arduino/ESP32 constraints
- ✅ **Protocol Compliance**: Simplified but functional AX.25-style implementations

## Priority 5: System Monitoring (4 commands)

Environmental and diagnostic features:

### TEMPERATURE - Hardware Temperature
- **Current Status**: Returns simulated temperature (25.0°C)
- **Implementation**: Connect to ESP32 internal temperature sensor
- **Notes**: ESP32-S3 has built-in temperature sensor via analogRead()

### VOLTAGE - Power Supply Monitoring
- **Current Status**: Returns simulated voltage (3.3V)  
- **Implementation**: Connect to hardware voltage divider on ADC pin
- **Notes**: May need hardware modification for battery voltage monitoring

### CAL - Hardware Calibration
- **Current Status**: Returns "Calibration not yet implemented"
- **Implementation**: Implement radio calibration routines
- **Notes**: May include frequency offset and power calibration

### LOG - System Logging
- **Current Status**: Returns "Logging not yet implemented"
- **Implementation**: Implement logging system with multiple levels
- **Notes**: Could use ESP32 SPIFFS or SD card for log storage

## Priority 6: Advanced Features (4+ commands)

Additional TNC capabilities for future enhancement:

### CALIBRATE - Advanced Calibration
- **Current Status**: Stub implementation
- **Implementation**: Advanced radio calibration and testing
- **Notes**: Extended calibration beyond basic CAL command

### MAILBOX - Store and Forward
- **Current Status**: Basic framework
- **Implementation**: Message store-and-forward system
- **Notes**: Requires message storage and retrieval system

### GATEWAY - Internet Gateway
- **Current Status**: Basic framework  
- **Implementation**: Bridge between LoRa and Internet protocols
- **Notes**: Requires WiFi connectivity and protocol translation

### Network Topology Enhancement
- **Implementation**: Enhanced NODES command with full topology mapping
- **Notes**: Advanced mesh networking capabilities

## Technical Notes

### Existing LoRaRadio Class Methods Available:
- `bool begin()` - Initialize with defaults
- `bool begin(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate)` - Custom init
- `bool transmit(const uint8_t *data, size_t length)` - Send packet
- `bool transmit(const String &message)` - Send string
- `bool available()` - Check for received packet
- `size_t receive(uint8_t *buffer, size_t maxLength)` - Read packet
- `bool receive(String &message)` - Read string
- `float getRSSI()` - Get RSSI of last packet
- `float getSNR()` - Get SNR of last packet
- `String getStatus()` - Get radio status

### Hardware Platform:
- **MCU**: ESP32-S3 (240MHz, 512KB RAM, 16MB Flash)
- **Radio**: SX1262 LoRa transceiver  
- **Board**: Heltec WiFi LoRa 32 V4
- **Framework**: Arduino with PlatformIO

### Key Implementation Strategies:

1. **Radio Parameter Changes**: Most Priority 1 commands will require reinitializing the radio with new parameters using the existing `begin()` method with custom parameters.

2. **Configuration Persistence**: Use ESP32 Preferences library for non-volatile storage of TNCConfig structure.

3. **Real-time Monitoring**: Leverage existing LoRaRadio getter methods that already provide RSSI/SNR data.

4. **Protocol Implementation**: AX.25 packet handling will be the most complex, requiring proper packet parsing, state machines, and protocol compliance.

5. **System Integration**: Many commands will need integration with the main TNC loop for periodic operations (beacons, logging, etc.).

## Implementation Order Recommendation

1. ✅ **Start with FREQ command** - Direct path to existing hardware interface - **COMPLETED**
2. ✅ **Complete Priority 1** - Essential radio parameter control - **COMPLETED**
3. ✅ **Add RSSI/SNR** - Quick wins with existing methods - **COMPLETED**
4. ✅ **Implement SAVE/LOAD** - Critical for configuration management - **COMPLETED**
5. ✅ **Tackle packet protocols** - Core TNC functionality - **COMPLETED**
6. **Add monitoring features** - System health and diagnostics - **NEXT**
7. **Advanced features** - Enhancement and optimization

### 🎯 **MASSIVE MILESTONE ACHIEVED** 🎯
**Complete TNC Protocol Stack Implemented!**
- **Hardware Control**: All radio parameters (frequency, power, SF, BW, CR, sync) ✅
- **Real-time Data**: RSSI and SNR monitoring from actual hardware ✅  
- **Configuration Management**: Complete save/load with flash persistence ✅
- **Packet Protocols**: Full AX.25-style packet handling implemented ✅
- **Network Functions**: Routing, digipeating, node discovery ✅
- **Connection Management**: CONNECT/DISCONNECT with state tracking ✅

**Next Phase**: System monitoring features (temperature, voltage, diagnostics)

## Files to Modify

- **Primary**: `lib/tnc_commands/src/TNCCommands.cpp` - Command implementations
- **Secondary**: `src/LoRaRadio.cpp` - May need additional methods for power/sync control
- **Configuration**: `lib/tnc_commands/include/TNCCommands.h` - May need additional data structures
- **Documentation**: `docs/COMPREHENSIVE_COMMAND_SET.md` - Update implementation status

## Success Criteria

- [x] **All Priority 1 commands control actual radio hardware** ✅ COMPLETED
  - FREQ, POWER, SF, BW, CR, SYNC all have full hardware integration
  - Commands validate parameters and update radio hardware
  - Error handling for hardware failures implemented
- [x] **RSSI/SNR commands return real-time data** ✅ COMPLETED
  - RSSI command returns actual signal strength from last received packet
  - SNR command returns actual signal-to-noise ratio from hardware
  - Both commands have error handling for radio unavailability
- [x] **Configuration persistence works across power cycles** ✅ COMPLETED
  - SAVE command stores all 35+ configuration parameters to ESP32 flash
  - LOAD command restores configuration and automatically applies to radio hardware
  - Helper methods available for automatic startup configuration loading
  - Comprehensive error handling and validation throughout  
- [x] **Complete AX.25 packet transmission/reception functional** ✅ COMPLETED
  - UIDFRAME command generates and transmits AX.25 UI frames
  - LINKTEST provides ping/pong style connectivity testing
  - BEACON supports manual and configurable automatic beaconing
  - All packet commands integrate with hardware and provide error handling
- [x] **Network protocol stack operational** ✅ COMPLETED
  - ROUTE command provides complete routing table management (32 entries)
  - NODES command tracks heard stations with signal quality (64 stations)
  - DIGI command implements digipeater functionality with WIDE alias support
  - CONNECT/DISCONNECT commands provide AX.25 connection management (4 connections)
- [ ] System monitoring provides real hardware data
- [x] **All Priority 4 commands provide meaningful responses** ✅ COMPLETED
  - No placeholder responses remain in packet handling commands
  - All commands provide comprehensive help text and status information
  - Error handling and validation implemented throughout

---

*This document should be updated as implementations are completed to track progress and maintain project momentum.*