# LoRaTNCX

A full-featured Terminal Node Controller (TNC) firmware for Heltec WiFi LoRa 32 V3/V4 boards implementing AX.25 packet radio protocol over LoRa.

## Features

- **AX.25 Protocol**: Full TNC-2 command set implementation (57/57 commands)
- **KISS Mode**: Binary frame protocol for use with packet radio applications
- **Multiple Operating Modes**: Command, Converse, Transparent, and KISS
- **LoRa Radio**: RadioLib-based SX1262 LoRa transceiver support
- **Persistent Settings**: NVS-based configuration storage
- **Monitoring**: Packet monitoring with SSID masking, dupe suppression
- **Digipeating**: Configurable digipeat support
- **Beaconing**: Periodic beacon transmission
- **Connection Management**: Layer 2 state scaffolding for future AX.25 connected mode

## Hardware Support

- **Heltec WiFi LoRa 32 V3** (ESP32-S3, SX1262)
- **Heltec WiFi LoRa 32 V4** (ESP32-S3, SX1262)

## Build & Upload

### Prerequisites
- PlatformIO IDE (VS Code extension) or PlatformIO Core
- USB cable for programming

### Build Commands

```bash
# Build for V3
platformio run --environment heltec_wifi_lora_32_V3

# Build for V4
platformio run --environment heltec_wifi_lora_32_V4

# Build all environments
platformio run

# Upload to V3
platformio run --environment heltec_wifi_lora_32_V3 --target upload

# Upload to V4
platformio run --environment heltec_wifi_lora_32_V4 --target upload
```

### PlatformIO Tasks
Use VS Code tasks for common operations:
- "Build LoRaTNCX V3"
- "Build LoRaTNCX V4"
- "Build All Environments"
- "Clean and Build V3"
- "Clean and Build V4"
- "Upload Filesystem (SPIFFS) V3"
- "Upload Filesystem (SPIFFS) V4"

## Usage

### Getting Started

1. Connect board via USB
2. Open serial terminal at 115200 baud
3. Type `HELP` to see available commands
4. Configure station callsign: `MYCALL N0CALL-1`
5. Set LoRa parameters: `RF 433.500` (frequency in MHz)

### Operating Modes

- **Command Mode**: TNC-2 command prompt (default)
- **Converse Mode**: Enter with `CONVERSE` or Ctrl+C, exit with `/EXIT`
- **Transparent Mode**: Enter with `TRANS`, exit with `cmd:` prompt
- **KISS Mode**: Enter with `KISS ON` then `RESTART`, exit with ESC or CMD_RETURN frame

### Example Commands

```
MYCALL N0CALL-1        # Set station callsign
DEST IDENT             # Set default destination
MYALIAS WIDE1-1        # Set digipeater alias
RF 433.500             # Set frequency
BTEXT Hello World!     # Set beacon text
EVERY 600              # Beacon every 10 minutes
MONITOR ON             # Enable packet monitoring
DISPLAY                # Show all settings
```

See `docs/LoRaTNCX_Commands.md` for complete command reference.

## Documentation

- **docs/LoRaTNCX_Commands.md** - Complete TNC-2 command reference
- **docs/COMMAND_STATUS.md** - Implementation status (57/57 complete)
- **docs/KISS_REFACTORING.md** - KISS protocol architecture
- **docs/tnc2_commands.txt** - Original TNC-2 command specifications

## Architecture

The firmware is organized into focused modules:

- **LoRaTNCX**: Main TNC class, command handlers, settings management
- **CommandProcessor**: Terminal I/O, mode switching, line editing
- **KISSProtocol**: KISS binary frame protocol (separate from terminal handling)
- **LoRaRadio**: RadioLib wrapper for SX1262 LoRa transceiver
- **AX25**: AX.25 frame encoding/decoding, address parsing

Key design principles:
- **Single Responsibility**: Each class has one clear purpose (see KISS_REFACTORING.md)
- **Separation of Concerns**: Protocol logic separate from I/O handling
- **Struct-Based Organization**: Settings grouped into 10 logical structs
- **Persistent Configuration**: All settings saved to NVS automatically

## Resource Usage

**Flash**: ~400KB (12% of 3.3MB)  
**RAM**: ~21KB (6.6% of 320KB)  
**Plenty of headroom for future features!**

## Development Status

✅ **All 57 TNC-2 commands implemented** (January 2025)  
✅ KISS mode fully functional  
✅ Clean architecture with separated concerns  
✅ Comprehensive documentation  

### Future Enhancements
- AX.25 Layer 2 connected mode (scaffolding in place)
- KISS parameter persistence (TXDELAY, PERSISTENCE, etc.)
- Display support for OLED
- WiFi/Bluetooth connectivity
- APRS encoding/decoding

## Dependencies

- **RadioLib 7.4.0**: LoRa radio driver
- **Preferences 2.0.0**: ESP32 NVS wrapper
- **PlatformIO Platform**: espressif32 6.12.0
- **Framework**: Arduino for ESP32

## License

MIT License (see LICENSE file)

## Contributing

Contributions welcome! Please:
1. Follow existing code style
2. Update documentation for new features
3. Test on both V3 and V4 hardware when possible
4. Keep commits focused and well-described

## References

- AX.25 Protocol: https://www.ax25.net/
- KISS Protocol: https://en.wikipedia.org/wiki/KISS_(TNC)
- RadioLib: https://github.com/jgromes/RadioLib
- Heltec Automation: https://heltec.org/
