# Complete TNC-2 Command Reference for LoRaTNCX

## Introduction

This document provides a comprehensive mapping of original TNC-2 commands to their proposed LoRaTNCX implementations. The goal is to maintain familiar operation for traditional packet radio operators while leveraging LoRa's unique capabilities.

## Command Classification

### 🟢 Core Commands (High Priority Implementation)
Commands essential for basic TNC-2 operation and packet radio compatibility.

### 🟡 Extended Commands (Medium Priority) 
Commands that enhance functionality but aren't critical for basic operation.

### 🔴 Advanced Commands (Low Priority)
Commands for specialized features and advanced configurations.

### ⭐ LoRa Enhanced Commands
Commands that are enhanced beyond original TNC-2 capabilities using LoRa features.

---

## Complete Command Reference

| Original | Short | Default | LoRaTNCX Implementation | Priority | Description |
|----------|-------|---------|-------------------------|----------|-------------|
| `8bitconv` | `8B` | OFF | `TErminal 8bit ON/OFF` | 🟡 | Strip high-order bit in convers mode |
| `AUtolf` | `AU` | ON | `AUtolf ON/OFF` | 🟡 | Send Linefeed after each CR |
| `AWlen` | `AW` | 7 | `CHarlen 7/8` | 🟡 | Terminal character length (7/8 bit) |
| `Ax2512v2` | `AX` | OFF | `AX25v2 ON/OFF` | 🔴 | AX.25 version compatibility |
| `AXDelay` | `AXD` | 0 | `PTTdelay <ms>` | 🟡 | Voice repeater keyup delay → LoRa preamble |
| `AXHang` | `AXH` | 0 | `PTThang <ms>` | 🟡 | Voice repeater hang time |
| `Beacon` | `B` | E 0 | `Beacon E/A <sec>` | 🟢⭐ | Enhanced: Every/After timing with LoRa power mgmt |
| `BKondel` | `BK` | ON | `BKspace ON/OFF` | 🟡 | Send BS SP BS for DELETE character |
| `BText` | `BT` | "" | `BText <text>` | 🟢 | Beacon text (120 char max) |
| `BUdlist` | `BU` | OFF | `BUdlist ON/OFF` | 🟡 | Ignore stations in LCALLS list |
| `CALibra` | `CAL` | - | `CALibrate` | 🔴⭐ | LoRa frequency calibration |
| `CALSet` | `CALS` | - | `CALset <offset>` | 🔴 | Set calibration offset |
| `CANline` | `CAN` | $18 | `CANcel <char>` | 🟡 | Line delete character (Ctrl+X) |
| `CANPac` | `CANP` | $19 | `CANpac <char>` | 🟡 | Cancel current packet (Ctrl+Y) |
| `CHech` | `CH` | 30 | `CHeck <sec>` | 🟢 | Link timeout monitoring |
| `CLKADJ` | `CLK` | 0 | `CLKadj <val>` | 🔴 | Real time clock adjustment |
| `CMDtime` | `CMD` | 1 | `CMDtime <sec>` | 🟡 | Transparent mode escape timer |
| `CMSG` | `CM` | OFF | `CMSg ON/OFF` | 🟡 | Don't send CTEXT on connect |
| `COMmand` | `COM` | $03 | `COMmand <char>` | 🟡 | Escape from CONVERS (Ctrl+C) |
| `CONMode` | `CONM` | CONVERS | `CONMode CONV/TRANS` | 🟢 | Mode after link established |
| `Connect` | `C` | - | `Connect <call> [via <path>]` | 🟢⭐ | Enhanced: LoRa path with RSSI routing |
| `CONOk` | `CONO` | ON | `CONOk ON/OFF` | 🟢 | Allow incoming connections |
| `CONPerm` | `CONP` | OFF | `CONPerm ON/OFF` | 🟡 | Keep link permanent |
| `CONStamp` | `CONS` | OFF | `CONStamp ON/OFF` | 🟡 | Timestamp connect messages |
| `CStatus` | `CS` | - | `CStatus` | 🟢 | Show all stream status |
| `CONVers` | `CONV` | - | `CONVers` | 🟢 | Enter conversation mode |
| `CPactime` | `CP` | OFF | `CPactime ON/OFF` | 🟡 | Forward based on timers |
| `CR` | - | ON | `CR ON/OFF` | 🟡 | Append CR to data packets |
| `CText` | `CT` | "" | `CText <msg>` | 🟡 | Connect acknowledgment message |
| `DAytime` | `DAY` | - | `DAYtime <datetime>` | 🔴 | Set real time clock |
| `DAYUsa` | `DAYU` | ON | `DAYusa ON/OFF` | 🔴 | US date format (mm/dd/yy) |
| `DELete` | `DEL` | OFF | `DELete BS/DEL` | 🟡 | Delete character (BS vs DEL) |
| `DIGipeat` | `DIG` | ON | `DIGipeat ON/OFF` | 🟢⭐ | LoRa digipeater with RSSI metrics |
| `Disconnect` | `D` | - | `Disconnect [stream]` | 🟢 | Disconnect link(s) |
| `Display` | `DISP` | - | `DISplay [param]` | 🟡 | Show parameters |
| `DWait` | `DW` | 16 | `DWait <ms>` | 🟡 | Digipeater repeat delay |
| `Echo` | `E` | ON | `Echo ON/OFF` | 🟢 | Terminal echo control |
| `EScape` | `ES` | OFF | `EScape ON/OFF` | 🟡 | Translate ESC to $ |
| `Flow` | `F` | ON | `Flow ON/OFF` | 🟡 | Terminal flow control |
| `FRack` | `FR` | 3 | `FRack <units>` | 🟢 | Frame ACK timeout |
| `FUlldup` | `FU` | OFF | `FUlldup ON/OFF` | 🔴 | Full duplex mode |
| `HEaderln` | `H` | OFF | `HEaderln ON/OFF` | 🟡 | Header and text same line |
| `HID` | `HI` | OFF | `HID <interval>` | 🟡 | Automatic ID transmission |
| `ID` | - | - | `ID` | 🟡 | Force ID transmission |
| `KISS` | `K` | - | `KISS` | 🟢✅ | Enter KISS mode (already implemented) |
| `LCALLS` | `LC` | - | `LCALLS <call1> [call2...]` | 🟡 | Station filter list |
| `LCok` | `LCO` | ON | `LCok ON/OFF` | 🟡 | Case conversion control |
| `LCSTREAM` | `LCS` | ON | `LCSTREAM ON/OFF` | 🟡 | Stream ID case conversion |
| `LFadd` | `LF` | OFF | `LFadd ON/OFF` | 🟡 | Add LF after CR |
| `MAll` | `MA` | ON | `MAll ON/OFF` | 🟢 | Monitor data frames |
| `MAXframe` | `MAX` | 4 | `MAXframe <count>` | 🟢 | Window size control |
| `MCOM` | `MC` | OFF | `MCOM ON/OFF` | 🟡 | Monitor only data frames |
| `MCon` | `MCON` | OFF | `MCon ON/OFF` | 🟡 | Monitor during connection |
| `MFilter` | `MF` | - | `MFilter <pattern>` | 🟡⭐ | Enhanced: regex pattern support |
| `MHClear` | `MHC` | - | `MHClear` | 🟡 | Clear heard list |
| `MHeard` | `MH` | - | `MHeard` | 🟢⭐ | Enhanced: RSSI/SNR/SF data |
| `Monitor` | `M` | ON | `Monitor ON/OFF` | 🟢 | RF monitoring mode |
| `MRpt` | `MR` | ON | `MRpt ON/OFF` | 🟡 | Show digipeater path |
| `MStamp` | `MS` | OFF | `MStamp ON/OFF` | 🟢 | Timestamp monitored frames |
| `MYAlias` | `MYA` | "" | `MYAlias <alias>` | 🟢 | Digipeater alias |
| `MYcall` | `MY` | NOCALL | `MYcall <callsign>` | 🟢 | Station callsign |
| `NEwmode` | `NEW` | OFF | `NEwmode ON/OFF` | 🔴 | TNC-1 compatibility |
| `NOmode` | `NO` | OFF | `NOmode ON/OFF` | 🔴 | Explicit mode change only |
| `NUcr` | `NUC` | OFF | `NUcr ON/OFF` | 🟡 | Send nulls after CR |
| `NULf` | `NUL` | OFF | `NULf ON/OFF` | 🟡 | Send nulls after LF |
| `NULLS` | `NU` | 0 | `NULLS <count>` | 🟡 | Number of nulls to send |
| `Paclen` | `P` | 128 | `Paclen <bytes>` | 🟢⭐ | Enhanced: LoRa packet limits |
| `PACTime` | `PAC` | After 10 | `PACTime E/A <ms>` | 🟡 | Data forwarding timer |
| `PARity` | `PAR` | 3 | `PARity 0/1/2/3` | 🟡 | Terminal parity control |
| `PASS` | `PAS` | $16 | `PASS <char>` | 🟡 | Pass-through character (Ctrl+V) |
| `PASSAll` | `PASSA` | OFF | `PASSAll ON/OFF` | 🟡 | Accept frames with bad CRC |
| `RECOnnect` | `REC` | - | `REConnect <call>` | 🟡 | Reconnect via new path |
| `REDisplay` | `RED` | $12 | `REDisplay <char>` | 🟡 | Redisplay buffer (Ctrl+R) |
| `RESET` | `RES` | - | `RESET` | 🟢✅ | Reset to defaults (implemented) |
| `RESptime` | `RESP` | 12 | `RESptime <ms>` | 🟢 | ACK response delay |
| `RESTART` | `REST` | - | `RESTART` | 🟢✅ | Power-on reset (implemented) |
| `RETry` | `R` | 10 | `RETry <count>` | 🟢 | Maximum frame retries |
| `Screenln` | `SC` | 80 | `SCreen <width>` | 🟡 | Terminal width |
| `SEndpac` | `SE` | $0D | `SEndpac <char>` | 🟡 | Force packet send (CR) |
| `STArt` | `STA` | $11 | `STArt <char>` | 🟡 | XON character (Ctrl+Q) |
| `STOp` | `STO` | $13 | `STOp <char>` | 🟡 | XOFF character (Ctrl+S) |
| `STREAMCa` | `STRC` | OFF | `STReamca ON/OFF` | 🟡 | Show callsign after stream ID |
| `STREAMDbl` | `STRD` | OFF | `STRdb ON/OFF` | 🟡 | Double stream switch char |
| `STReamsw` | `STR` | $7C | `STReamsw <char>` | 🟡 | Stream switch character (\|) |
| `TRAce` | `TR` | OFF | `TRAce ON/OFF` | 🟡 | Hex trace mode |
| `TRANS` | `T` | - | `TRANS` | 🟡 | Transparent mode |
| `TRFlow` | `TRF` | OFF | `TRFlow ON/OFF` | 🟡 | Terminal flow control |
| `TRIes` | `TRI` | - | `TRIes [count]` | 🟡 | Show/set retry counter |
| `TXdelay` | `TX` | 30 | `TXdelay <symbols>` | 🟢⭐ | Enhanced: LoRa preamble length |
| `TXFlow` | `TXF` | OFF | `TXFlow ON/OFF` | 🟡 | TNC flow control |
| `Unproto` | `UN` | CQ | `Unproto <path>` | 🟡 | UI frame destination |
| `Users` | `U` | 1 | `Users <count>` | 🟡 | Number of streams allowed |
| `Xflow` | `XF` | ON | `Xflow ON/OFF` | 🟡 | XON/XOFF flow control |
| `XMitok` | `XM` | ON | `XMitok ON/OFF` | 🟢 | Allow transmitter |
| `XOff` | `XO` | $13 | `XOff <char>` | 🟡 | XOFF character |
| `XON` | - | $11 | `XON <char>` | 🟡 | XON character |

---

## LoRa-Specific Command Extensions

### New Commands for LoRa Operation

| Command | Short | Description | Example |
|---------|-------|-------------|---------|
| `LORAfreq` | `LF` | Set LoRa frequency | `LORAfreq 915.0` |
| `LORApower` | `LP` | Set TX power | `LORApower 20` |
| `LORAsf` | `LSF` | Set spreading factor | `LORAsf 8` |
| `LORAcr` | `LCR` | Set coding rate | `LORAcr 5` |
| `LORAbw` | `LBW` | Set bandwidth | `LORAbw 125` |
| `LORAband` | `LB` | Select frequency band | `LORAband ISM_915` |
| `LORAcad` | `LCAD` | Channel Activity Detection | `LORAcad ON` |
| `LORAhop` | `LH` | Frequency hopping | `LORAhop ENABLE` |
| `RSsi` | `RS` | Show RSSI stats | `RSsi` |
| `SNr` | `SN` | Show SNR stats | `SNr` |
| `LINKqual` | `LQ` | Show link quality | `LINKqual` |
| `RANge` | `RAN` | Estimate range | `RANge KC1AWV-2` |
| `MEsh` | `MESH` | Mesh networking | `MEsh ON` |
| `ROUte` | `ROU` | Show routing table | `ROUte` |
| `TOA` | - | Time on air calculator | `TOA 50` |
| `BAnd` | `BAN` | Band information | `BAnd` |

### Enhanced Status Commands

| Command | Enhancement | Description |
|---------|-------------|-------------|
| `STatus` | LoRa metrics | Include RSSI, SNR, SF, frequency |
| `STats` | RF statistics | TX/RX counts, error rates, band usage |
| `SIgnal` | Signal analysis | Real-time signal strength monitoring |
| `LINKtest` | Link testing | Automated range/quality testing |
| `PERformance` | Performance metrics | Throughput, latency, reliability stats |

---

## Implementation Examples

### Core TNC-2 Command Implementation

```cpp
// CommandProcessor.h additions
class TNC2CommandProcessor {
private:
    // Command abbreviation resolver
    std::map<String, String> commandAliases;
    
    // TNC-2 compatible handlers
    void handleMycall(const String& args);
    void handleBeacon(const String& args);
    void handleConnect(const String& args);
    void handleMonitor(const String& args);
    void handleMheard(const String& args);
    
    // Enhanced LoRa handlers
    void handleLoraFreq(const String& args);
    void handleLoraPower(const String& args);
    void handleRssi(const String& args);
    
public:
    void registerAliases();
    String resolveCommand(const String& input);
    bool processTNC2Command(const String& cmd, const String& args);
};

void TNC2CommandProcessor::registerAliases() {
    // Register all TNC-2 command aliases
    commandAliases["MY"] = "MYcall";
    commandAliases["B"] = "Beacon"; 
    commandAliases["C"] = "Connect";
    commandAliases["M"] = "Monitor";
    commandAliases["MH"] = "MHeard";
    commandAliases["K"] = "KISS";
    commandAliases["LF"] = "LORAfreq";
    commandAliases["LP"] = "LORApower";
    // ... additional aliases
}
```

### Configuration Storage

```cpp
// TNC2Config.h
class TNC2Config {
private:
    // Core TNC-2 parameters
    String myCall = "NOCALL";
    String myAlias = "";
    String beaconText = "";
    bool monitor = true;
    bool mstamp = false;
    uint8_t maxframe = 4;
    uint8_t retry = 10;
    uint16_t paclen = 128;
    
    // LoRa enhanced parameters
    float frequency = 915.0;
    int8_t txPower = 20;
    uint8_t spreadingFactor = 8;
    float bandwidth = 125.0;
    uint8_t codingRate = 5;
    
public:
    // Getters/Setters with validation
    bool setMyCall(const String& call);
    bool setFrequency(float freq);
    bool setTxPower(int8_t power);
    
    // Persistence
    void loadFromNVS();
    void saveToNVS();
    void resetToDefaults();
    
    // Display
    void printConfiguration();
};
```

### Enhanced Monitor Mode

```cpp
// MonitorMode.h
class EnhancedMonitor {
private:
    bool enabled = true;
    bool timestamp = false;
    bool showRSSI = true;
    bool showSNR = true; 
    bool showSF = true;
    std::vector<String> filters;
    
public:
    void displayPacket(const String& from, const String& to, 
                      const String& data, int16_t rssi, float snr, uint8_t sf) {
        if (!enabled || !shouldDisplay(from)) return;
        
        String output = "";
        if (timestamp) {
            output += getTimestamp() + " ";
        }
        
        output += from + ">" + to + ": " + data;
        
        if (showRSSI || showSNR || showSF) {
            output += " (";
            if (showRSSI) output += "RSSI:" + String(rssi) + "dBm ";
            if (showSNR) output += "SNR:" + String(snr, 1) + "dB ";
            if (showSF) output += "SF" + String(sf);
            output += ")";
        }
        
        Serial.println(output);
    }
    
private:
    bool shouldDisplay(const String& callsign);
    String getTimestamp();
};
```

---

## Migration Path

### Phase 1: Core Commands (Week 1-2)
- ✅ `KISS` (already implemented)
- 🚧 `MYcall`, `BText`, `Beacon`
- 🚧 `Monitor`, `MHeard`, `MStamp`
- 🚧 `Connect`, `CONOk`, `CStatus`

### Phase 2: Extended Commands (Week 3-4)
- 🚧 `DIGipeat`, `MAXframe`, `RETry`
- 🚧 `CONVers`, `Echo`, `Xflow`
- 🚧 `Paclen`, `FRack`, `RESptime`

### Phase 3: LoRa Enhancements (Week 5-6)
- 🚧 `LORAfreq`, `LORApower`, `LORAsf`
- 🚧 `RSsi`, `SNr`, `LINKqual`
- 🚧 Enhanced monitoring with LoRa metrics

### Phase 4: Advanced Features (Week 7-8)
- 🚧 `TRANS`, `Unproto`, `LCALLS`
- 🚧 `MEsh`, `ROUte`, routing table
- 🚧 APRS integration

## Backward Compatibility

### Existing Commands Preserved
All current LoRaTNCX commands continue to work:
- `lora` subcommands remain unchanged
- `help`, `status`, `config` enhanced but compatible
- KISS protocol fully backward compatible

### New Default Behavior
- TNC-2 commands are additive, not replacing
- Original LoRaTNCX behavior is default
- TNC-2 mode can be enabled via `TNCmode TNC2`
- Command abbreviations work alongside full commands

This comprehensive reference ensures LoRaTNCX can serve both modern LoRa applications and traditional packet radio operations seamlessly.