# LoRaTNCX Comprehensive Command Set

*Making your LoRa TNC as capable as a Kantronics KPC-3 or TAPR TNC-2*

## Overview

This document describes the comprehensive command set being developed for LoRaTNCX to transform it from a basic KISS TNC into a full-featured Terminal Node Controller comparable to industry standards like Kantronics and TAPR TNCs, with modern LoRa enhancements and amateur radio optimizations.

## Command Categories

### 1. Station Configuration Commands
These commands manage core station identity and amateur radio compliance:

| Command    | Parameters           | Function                         | Example                        |
| ---------- | -------------------- | -------------------------------- | ------------------------------ |
| `MYCALL`   | [callsign]           | Set/display station callsign     | `MYCALL KC1AWV`                |
| `MYSSID`   | [0-15]               | Set/display station SSID         | `MYSSID 1`                     |
| `BCON`     | [ON\|OFF] [interval] | Beacon control and interval      | `BCON ON 300`                  |
| `BTEXT`    | [text]               | Set beacon text message          | `BTEXT "LoRa TNC Test"`        |
| `ID`       | [ON\|OFF]            | Station identification control   | `ID ON`                        |
| `CWID`     | [ON\|OFF]            | CW identification enable         | `CWID OFF`                     |
| `LOCATION` | [lat] [lon] [alt]    | Set station coordinates          | `LOCATION 42.3601 -71.0589 10` |
| `GRID`     | [maidenhead]         | Set location via grid square     | `GRID FN42ni`                  |
| `LICENSE`  | [class]              | Set license class for compliance | `LICENSE GENERAL`              |

### 2. Radio Parameter Commands
Control LoRa radio settings and RF parameters:

| Command    | Parameters | Function                       | Example             |
| ---------- | ---------- | ------------------------------ | ------------------- |
| `FREQ`     | [MHz]      | Operating frequency            | `FREQ 915.0`        |
| `POWER`    | [dBm]      | Transmit power level           | `POWER 20`          |
| `SF`       | [5-12]     | LoRa spreading factor          | `SF 7`              |
| `BW`       | [kHz]      | LoRa bandwidth                 | `BW 125`            |
| `CR`       | [5-8]      | LoRa coding rate (4/5 to 4/8)  | `CR 5`              |
| `SYNC`     | [hex]      | Sync word                      | `SYNC 0x12`         |
| `PREAMBLE` | [symbols]  | Preamble length                | `PREAMBLE 8`        |
| `PRESET`   | [name]     | Load LoRa configuration preset | `PRESET HIGH_SPEED` |
| `PACTL`    | [ON\|OFF]  | Power amplifier control        | `PACTL ON`          |

### 3. Protocol Stack Commands
Configure packet protocol parameters:

| Command    | Parameters | Function                     | Example        |
| ---------- | ---------- | ---------------------------- | -------------- |
| `TXDELAY`  | [0-255]    | TX key-up delay (10ms units) | `TXDELAY 30`   |
| `TXTAIL`   | [0-255]    | TX tail time (10ms units)    | `TXTAIL 5`     |
| `PERSIST`  | [0-255]    | Persistence parameter P      | `PERSIST 63`   |
| `SLOTTIME` | [0-255]    | Slot time (10ms units)       | `SLOTTIME 10`  |
| `DUPLEX`   | [ON\|OFF]  | Full/half duplex mode        | `DUPLEX OFF`   |
| `MAXFRAME` | [bytes]    | Maximum frame size           | `MAXFRAME 255` |
| `RETRY`    | [count]    | Retry attempts               | `RETRY 10`     |
| `FRACK`    | [seconds]  | Frame acknowledgment timeout | `FRACK 3`      |
| `RESPTIME` | [seconds]  | Response time                | `RESPTIME 10`  |

### 4. Operating Mode Commands
Control TNC operating modes and interfaces:

| Command       | Parameters               | Function                   | Example            |
| ------------- | ------------------------ | -------------------------- | ------------------ |
| `MODE`        | [KISS\|CMD\|TERM\|TRANS] | Set operating mode         | `MODE KISS`        |
| `KISS`        | -                        | Enter KISS mode            | `KISS`             |
| `CMD`         | -                        | Enter command mode         | `CMD`              |
| `TERMINAL`    | -                        | Enter terminal mode        | `TERMINAL`         |
| `TRANSPARENT` | -                        | Enter transparent mode     | `TRANSPARENT`      |
| `ECHO`        | [ON\|OFF]                | Command echo               | `ECHO ON`          |
| `PROMPT`      | [ON\|OFF]                | Command prompt display     | `PROMPT ON`        |
| `CONNECT`     | [callsign]               | Connect to station         | `CONNECT KC1AWV-1` |
| `DISCONNECT`  | -                        | Disconnect current session | `DISCONNECT`       |

### 5. Monitoring and Statistics Commands
Monitor RF conditions and system performance:

| Command       | Parameters | Function                 | Example       |
| ------------- | ---------- | ------------------------ | ------------- |
| `MONITOR`     | [ON\|OFF]  | Packet monitoring        | `MONITOR ON`  |
| `MHEARD`      | -          | List stations heard      | `MHEARD`      |
| `STAT`        | -          | General statistics       | `STAT`        |
| `STATUS`      | -          | System status summary    | `STATUS`      |
| `RSSI`        | -          | Received signal strength | `RSSI`        |
| `SNR`         | -          | Signal-to-noise ratio    | `SNR`         |
| `TEMPERATURE` | -          | Radio temperature        | `TEMPERATURE` |
| `VOLTAGE`     | -          | Supply voltage           | `VOLTAGE`     |
| `MEMORY`      | -          | Memory usage             | `MEMORY`      |
| `UPTIME`      | -          | System uptime            | `UPTIME`      |
| `VERSION`     | -          | Firmware version         | `VERSION`     |

### 6. LoRa-Specific Commands
Advanced LoRa radio management and optimization:

| Command       | Parameters         | Function                      | Example                |
| ------------- | ------------------ | ----------------------------- | ---------------------- |
| `LORASTAT`    | -                  | LoRa-specific statistics      | `LORASTAT`             |
| `TOA`         | [bytes]            | Time-on-air calculation       | `TOA 100`              |
| `RANGE`       | -                  | Estimated communication range | `RANGE`                |
| `LINKTEST`    | [callsign] [count] | Perform link test             | `LINKTEST KC1AWV-2 10` |
| `SENSITIVITY` | -                  | Receiver sensitivity          | `SENSITIVITY`          |
| `PACAL`       | -                  | Power amplifier calibration   | `PACAL`                |
| `RFTEST`      | [mode]             | RF test modes                 | `RFTEST CW`            |
| `SPECTRUM`    | -                  | Spectrum analysis             | `SPECTRUM`             |
| `HOPSCAN`     | -                  | Frequency hopping scan        | `HOPSCAN`              |

### 7. Amateur Radio Specific Commands
Amateur radio band plans and regulatory compliance:

| Command      | Parameters           | Function                      | Example                    |
| ------------ | -------------------- | ----------------------------- | -------------------------- |
| `BAND`       | [70CM\|33CM\|23CM]   | Select amateur band           | `BAND 70CM`                |
| `REGION`     | [US\|EU\|JA\|etc]    | Set regulatory region         | `REGION US`                |
| `COMPLIANCE` | -                    | Check regulatory compliance   | `COMPLIANCE`               |
| `EMERGENCY`  | [ON\|OFF]            | Emergency communications mode | `EMERGENCY ON`             |
| `APRS`       | [ON\|OFF]            | APRS mode configuration       | `APRS ON`                  |
| `APRSSYM`    | [symbol] [table]     | APRS symbol selection         | `APRSSYM Y /`              |
| `WINLINK`    | [gateway]            | Winlink gateway setup         | `WINLINK KC1AWV-10`        |
| `DIGIPEAT`   | [ON\|OFF] [path]     | Digipeater configuration      | `DIGIPEAT ON WIDE1-1`      |
| `BEACON`     | [dest] [path] [text] | Beacon configuration          | `BEACON CQ WIDE1-1 "Test"` |

### 8. Network and Routing Commands
Packet routing and network configuration:

| Command    | Parameters           | Function               | Example                  |
| ---------- | -------------------- | ---------------------- | ------------------------ |
| `UNPROTO`  | [destination] [path] | Unprotocol address     | `UNPROTO CQ VIA WIDE1-1` |
| `DIGIPEAT` | [ON\|OFF]            | Enable digipeating     | `DIGIPEAT ON`            |
| `DIGI`     | [alias]              | Set digipeater alias   | `DIGI RELAY`             |
| `UIDWAIT`  | [ON\|OFF]            | Wait for channel clear | `UIDWAIT ON`             |
| `UIDFRAME` | [text]               | Send UI frame          | `UIDFRAME "Hello World"` |
| `MCON`     | [ON\|OFF]            | Multiple connections   | `MCON OFF`               |
| `USERS`    | [count]              | Maximum users          | `USERS 1`                |
| `FLOW`     | [ON\|OFF]            | Flow control           | `FLOW ON`                |

### 9. System Configuration Commands
System-level configuration and maintenance:

| Command     | Parameters | Function                      | Example     |
| ----------- | ---------- | ----------------------------- | ----------- |
| `SAVE`      | -          | Save configuration to flash   | `SAVE`      |
| `LOAD`      | -          | Load configuration from flash | `LOAD`      |
| `DEFAULT`   | -          | Restore factory defaults      | `DEFAULT`   |
| `RESET`     | -          | System restart                | `RESET`     |
| `HELP`      | [command]  | Display help                  | `HELP FREQ` |
| `QUIT`      | -          | Exit command mode             | `QUIT`      |
| `CALIBRATE` | -          | System calibration            | `CALIBRATE` |
| `SELFTEST`  | -          | Built-in self test            | `SELFTEST`  |
| `DEBUG`     | [level]    | Debug output level            | `DEBUG 2`   |

## Command Mode Operation

### Entering Command Mode
- **From KISS Mode**: Send escape sequence `+++` followed by `CR`
- **From any mode**: Hardware reset or power cycle defaults to command mode
- **Startup**: TNC starts in command mode by default

### Command Mode Prompt
```
CMD:
```

### Command Syntax
- Commands are case-insensitive
- Parameters separated by spaces
- Quoted strings supported: `BTEXT "Hello World"`
- Numeric parameters accept decimal or hex (0x prefix)

### Mode Switching
```
CMD: MODE KISS        # Switch to KISS mode for host applications
CMD: MODE TERMINAL    # Switch to terminal/chat mode
CMD: MODE TRANSPARENT # Switch to transparent/connected mode
CMD: KISS            # Quick switch to KISS mode
```

## Extended KISS Commands

Beyond standard KISS commands (0x00-0x06), LoRaTNCX supports extended commands:

| Code | Command            | Function                   |
| ---- | ------------------ | -------------------------- |
| 0x10 | Set Frequency      | Extended frequency control |
| 0x11 | Set Power          | Extended power control     |
| 0x12 | Set LoRa SF        | Spreading factor           |
| 0x13 | Set LoRa BW        | Bandwidth                  |
| 0x14 | Set LoRa CR        | Coding rate                |
| 0x20 | Get Status         | Request status packet      |
| 0x21 | Get Statistics     | Request statistics         |
| 0x30 | Beacon Control     | Beacon on/off              |
| 0xFF | Enter Command Mode | Switch to command mode     |

## Configuration Persistence

All configuration parameters are automatically saved to flash memory and persist across power cycles. Manual save/load commands available:

```
CMD: SAVE       # Save current config to flash
CMD: LOAD       # Load saved config from flash  
CMD: DEFAULT    # Reset to factory defaults
```

## Amateur Radio Compliance Features

### Time-on-Air Monitoring
```
CMD: TOA 100           # Calculate time-on-air for 100-byte packet
Time-on-air: 45.6 ms   # Automatic compliance checking
```

### Band Plan Integration
```
CMD: BAND 70CM         # Select 70cm amateur band
CMD: FREQ 432.100      # Frequency automatically validated
CMD: COMPLIANCE        # Check current configuration compliance
```

### Station Identification
```
CMD: ID ON             # Enable periodic station ID
CMD: CWID ON           # Enable CW identification
CMD: BCON ON 600       # Beacon every 10 minutes with ID
```

## Integration with Existing Code

This command system integrates with your existing LoRaTNCX components:

1. **ConfigurationManager**: Radio parameter commands use existing presets
2. **KISSProtocol**: Extended KISS commands alongside standard ones  
3. **LoRaRadio**: Direct radio control and monitoring
4. **TNCManager**: Coordinated system management

## Implementation Status

- ✅ **Command Architecture**: Complete framework design
- ✅ **Command Parser**: Robust parsing with validation
- 🔄 **Station Config**: Core station management (in progress)  
- ⏳ **Radio Commands**: Integration with existing ConfigurationManager
- ⏳ **Protocol Stack**: AX.25 and KISS enhancements
- ⏳ **Monitoring**: Statistics and RF monitoring
- ⏳ **Amateur Radio**: Band plans and compliance checking

## Future Enhancements

1. **AX.25 Layer 2**: Full connected mode protocol
2. **Mesh Networking**: LoRa mesh protocols
3. **Digital Signal Processing**: Advanced modulation schemes
4. **Remote Control**: Web interface and API
5. **AI Integration**: Automatic parameter optimization
6. **Multi-Protocol**: Support for other digital modes

## Conclusion

This comprehensive command set transforms LoRaTNCX from a basic KISS TNC into a professional-grade Terminal Node Controller with capabilities exceeding traditional TNCs. The combination of proven amateur radio protocols with modern LoRa technology and AI-assisted development creates a powerful platform for amateur radio digital communications.

---

**Status**: 🚧 **In Development**  
**Target Completion**: End of 2025  
**Compatibility**: Kantronics KPC-3, TAPR TNC-2, and extended LoRa features