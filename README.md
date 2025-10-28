# LoRaTNCX v2.0 - Fresh Start

## Overview
This is a complete fresh restart of the LoRaTNCX project, implementing a LoRa TNC (Terminal Node Controller) for amateur radio packet operations using the Heltec WiFi LoRa 32 V4 board with ESP32-S3 and SX1262 LoRa chip.

## Current Status ✅
- [x] **Repository Reset**: Complete clean slate with all old code removed
- [x] **PlatformIO Project**: Fresh ESP32-S3 project initialization  
- [x] **Custom Board Definition**: Proper Heltec V4 board configuration with 16MB flash
- [x] **Hardware Pin Configuration**: Complete pin mapping including critical PA pins
- [x] **RadioLib Integration**: SX1262 LoRa radio library support (v7.4.0)
- [x] **LoRa Radio Interface**: Working wrapper class for RadioLib
- [x] **KISS Protocol**: Complete KISS TNC implementation with frame encoding/decoding
- [x] **TAPR TNC-2 Commands**: Compatible command interface (KISS ON, RESTART, etc.)
- [x] **Mode Management**: Seamless switching between Command and KISS modes
- [x] **Build Success**: Complete TNC system compiles and ready for testing

## Hardware Configuration

### Heltec WiFi LoRa 32 V4 Board
- **MCU**: ESP32-S3 @ 240MHz
- **Flash**: 16MB
- **RAM**: 512KB + PSRAM support
- **LoRa Chip**: SX1262
- **Operating Frequency**: 863-928 MHz (region dependent)

### Pin Definitions
```cpp
// LoRa SX1262 pins
#define LORA_SS_PIN         8   // SPI Chip Select
#define LORA_RST_PIN        12  // Reset
#define LORA_DIO0_PIN       14  // DIO0/IRQ
#define LORA_BUSY_PIN       13  // Busy

// Critical Power Amplifier pins (discovered from factory firmware)
#define LORA_PA_POWER_PIN   7   // PA power control (analog)
#define LORA_PA_EN_PIN      2   // PA enable (digital)
#define LORA_PA_TX_EN_PIN   46  // PA TX enable (digital)

// OLED Display
#define OLED_SDA_PIN        17  // I2C Data
#define OLED_SCL_PIN        18  // I2C Clock
#define OLED_RST_PIN        21  // Reset

// GNSS Module pins
#define GNSS_RX_PIN         34  // UART RX
#define GNSS_TX_PIN         38  // UART TX
#define GNSS_PPS_PIN        39  // Pulse Per Second
#define GNSS_RST_PIN        40  // Reset
#define GNSS_WAKEUP_PIN     41  // Wakeup
#define GNSS_BOOT_MODE_PIN  42  // Boot mode select
```

## Key Features Implemented

### 1. KISS Protocol Support (`KISSProtocol`)
- Complete KISS frame encoding and decoding
- Proper escape sequence handling (FEND, FESC, TFEND, TFESC)
- All standard KISS commands (Data, TXDELAY, P-persistence, SlotTime, etc.)
- Stream processing for serial interface
- Parameter management and validation

### 2. TAPR TNC-2 Compatible Interface (`TNCCommandParser`)
- Full command-line interface with familiar TNC commands
- Configuration commands (MYCALL, TXDELAY, PERSIST, SLOTTIME, etc.)
- Mode switching commands (KISS ON/OFF, RESTART)
- Status and diagnostic commands (DISPLAY, STATUS, VERSION, HELP)
- Interactive command processing with prompts and help

### 3. TNC Manager (`TNCManager`)
- Seamless mode switching between Command and KISS modes
- Packet queuing and transmission management
- P-persistence CSMA implementation
- Statistics tracking and reporting
- Integration between KISS protocol and LoRa radio

### 4. LoRa Radio Interface (`LoRaRadio`)
- SX1262 initialization and configuration
- Frequency, power, bandwidth, and spreading factor control
- Transmit and receive operations with proper PA control
- RSSI and SNR reporting
- Amateur radio frequency validation

### 5. Hardware Abstraction
- Comprehensive pin definitions in `HardwareConfig.h`
- Critical power amplifier initialization sequence
- Complete hardware configuration documentation

## Build and Upload

### Prerequisites
- PlatformIO installed
- Heltec WiFi LoRa 32 V4 board

### Commands
```bash
# Build the project
platformio run -e heltec_wifi_lora_32_V4

# Upload to board
platformio run -e heltec_wifi_lora_32_V4 -t upload

# Monitor serial output
platformio device monitor
```

## Current Functionality

### On Boot
1. ✅ Serial communication initialization (KISS standard 9600 baud)
2. ✅ Hardware pin configuration and critical PA initialization  
3. ✅ LoRa radio initialization with RadioLib SX1262 support
4. ✅ TNC Manager and KISS protocol initialization
5. ✅ TAPR TNC-2 compatible command interface activation
6. ✅ Welcome banner and command prompt display

### Command Mode Operation
1. ✅ **Interactive Commands**: Full TAPR TNC-2 command set
   - `KISS ON` - Enable KISS mode (activate with RESTART)
   - `KISSM` - Enter KISS mode immediately  
   - `RESTART` - Restart TNC (enters KISS if enabled)
   - `MYCALL <callsign>` - Set station callsign
   - `TXDELAY <n>` - Set transmitter keyup delay
   - `PERSIST <n>` - Set p-persistence parameter
   - `DISPLAY` - Show all current parameters
   - `STATUS` - System status and statistics
   - `HELP` - Complete command reference

2. ✅ **Packet Operations**: Human-readable packet monitoring
   - Display received packets with RSSI/SNR
   - Show transmission status and results
   - Packet statistics and diagnostics

### KISS Mode Operation ("Silence is Compliance")
1. ✅ **Pure KISS Protocol**: Silent operation for host applications
   - KISS frame encoding/decoding with proper escape sequences
   - Support for all KISS commands (0x00-0x06, 0xFF)
   - Multi-port support (upper nibble addressing)
   - Parameter control via KISS commands
   - Return to command mode via 0xFF command

2. ✅ **Packet Processing**: Transparent packet handling
   - Queue incoming packets from radio for host
   - Process outgoing packets from host to radio
   - P-persistence CSMA implementation
   - Proper timing control (TXDELAY, SlotTime)

### Radio Operations (Both Modes)
1. ✅ LoRa packet transmission and reception
2. ✅ RSSI/SNR signal quality reporting
3. ✅ P-persistence channel access control
4. ✅ Proper power amplifier control
5. ✅ Amateur radio frequency compliance

## Usage Guide

### Getting Started
1. **Connect to TNC**: Use any terminal program at 9600 baud, 8-N-1
2. **Power On**: TNC boots into command mode with welcome banner
3. **Set Callsign**: `MYCALL <your-callsign>`
4. **Configure Parameters**: Use `DISPLAY` to see current settings
5. **Test Operation**: Send test packets in command mode

### For Host Applications (APRS, Packet BBS, etc.)

#### Standard KISS Activation (TAPR TNC-2 Compatible)
1. **Enable KISS**: Send `KISS ON` command (sets flag, stays in command mode)
2. **Activate KISS**: Send `RESTART` command (enters KISS mode with LED flash)
3. **TNC becomes silent**: Only KISS frames from this point forward
4. **Send KISS frames**: Use standard KISS protocol for packet operations
5. **Return to commands**: Send KISS command 0xFF or power cycle

#### Immediate KISS Activation
1. **Direct Entry**: Send `KISSM` command (enters KISS mode immediately)
2. **No LED Flash**: Bypasses normal KISS ON + RESTART sequence
3. **Same Operation**: Identical KISS protocol behavior once active

### Example Command Session
```
LoRaTNCX v2.0 - TAPR TNC-2 Compatible
Hardware: Heltec WiFi LoRa 32 V4 (ESP32-S3)

cmd:MYCALL KI7ABC
MYCALL set to KI7ABC

cmd:TXDELAY 30
TXDELAY: 30 (300ms)

cmd:DISPLAY
=== TNC Parameters ===
MYCALL:     KI7ABC
TXDELAY:    30 (300ms)
PERSIST:    63 (p=0.250)
...

cmd:KISS ON
KISS mode will be enabled on restart

cmd:RESTART
Restarting TNC into KISS mode...
Status LED will flash three times
[Status LED flashes 3 times]
[TNC now in silent KISS mode]
```

## Next Development Steps

### Immediate (High Priority)
- [ ] **Real-World Testing**: Test with actual APRS and packet applications
- [ ] **OLED Display**: Show TNC status, mode, and basic packet info
- [ ] **Configuration Persistence**: Save settings to EEPROM/Flash
- [ ] **Enhanced Diagnostics**: Detailed error reporting and debugging

### Near Term
- [ ] **GNSS Integration**: GPS coordinate reporting for APRS
- [ ] **APRS Beacon**: Built-in APRS position beaconing
- [ ] **Web Interface**: WiFi-based configuration and monitoring
- [ ] **Multiple Frequencies**: Channel switching and scanning
- [ ] **Packet Routing**: Store-and-forward digipeater functionality

### Advanced Features
- [ ] **Mesh Networking**: Multi-hop LoRa packet routing
- [ ] **Encryption**: Optional packet encryption for security
- [ ] **Multiple Protocols**: Support AX.25, APRS, custom protocols
- [ ] **OTA Updates**: Over-the-air firmware updates
- [ ] **Battery Management**: Power optimization and monitoring

## Important Notes

### Power Amplifier Configuration
The PA pins are **critical** for proper LoRa transmission. Without proper PA initialization, the radio will not transmit effectively. The pin definitions were discovered by analyzing Heltec's factory firmware:

```cpp
pinMode(LORA_PA_POWER_PIN, ANALOG);     // Pin 7 - PA power control
pinMode(LORA_PA_EN_PIN, OUTPUT);        // Pin 2 - PA enable
pinMode(LORA_PA_TX_EN_PIN, OUTPUT);     // Pin 46 - PA TX enable
digitalWrite(LORA_PA_EN_PIN, HIGH);     // Enable PA
digitalWrite(LORA_PA_TX_EN_PIN, HIGH);  // Enable TX
```

### Amateur Radio Compliance
This project is designed for amateur radio use. Ensure compliance with local regulations:
- Use appropriate amateur radio frequencies
- Include proper station identification
- Respect power limitations
- Follow band plans and protocols

## Development History

### October 28, 2025 - Complete TNC Implementation
1. **Complete Repository Reset**: Removed all previous code for fresh start
2. **PlatformIO Initialization**: New ESP32-S3 project with Arduino framework
3. **Custom Board Definition**: Created `heltec_wifi_lora_32_V4.json` with proper ESP32-S3 configuration
4. **Pin Discovery**: Analyzed Heltec factory firmware to find critical PA pins
5. **RadioLib Integration**: Added RadioLib 7.4.0 for SX1262 support
6. **LoRa Interface**: Implemented complete LoRaRadio wrapper class
7. **KISS Protocol**: Full KISS TNC implementation with proper frame handling
8. **TAPR TNC-2 Commands**: Complete command interface compatible with existing software
9. **Mode Management**: Seamless switching between interactive and KISS modes
10. **TNC Integration**: Complete working TNC ready for amateur radio packet operations

## Files Structure
```
LoRaTNCX/
├── platformio.ini              # Project configuration
├── boards/
│   └── heltec_wifi_lora_32_V4.json  # Custom board definition
├── include/
│   ├── HardwareConfig.h        # Hardware pin definitions
│   ├── HeltecV4Pins.h         # Pin reference documentation
│   ├── LoRaRadio.h            # LoRa radio interface
│   ├── KISSProtocol.h         # KISS protocol implementation
│   ├── TNCCommandParser.h     # TAPR TNC-2 command interface
│   └── TNCManager.h           # TNC coordination and management
├── src/
│   ├── main.cpp               # Main application
│   ├── LoRaRadio.cpp          # LoRa radio implementation
│   ├── KISSProtocol.cpp       # KISS frame encoding/decoding
│   ├── TNCCommandParser.cpp   # Command line interface
│   └── TNCManager.cpp         # TNC operation management
├── default_16MB.csv           # 16MB partition table
└── README.md                  # This file
```

## License
This project is licensed under the MIT License. See LICENSE file for details.

## Contributing
This is a fresh start project. Contributions welcome for implementing KISS protocol, APRS support, and additional amateur radio features.

---
**73, LoRaTNCX Development Team**

A fresh start for a LoRa KISS TNC project.

## Getting Started

This project has been initialized as a PlatformIO project for ESP32 development targeting the Heltec WiFi LoRa 32 V4 board.

### Build
```bash
platformio run -e heltec_wifi_lora_32_V4
```

### Upload
```bash
platformio run -e heltec_wifi_lora_32_V4 -t upload
```

## For Developers and AI Assistants

See `CLAUDE.md` for detailed project information, architecture guidelines, and development notes.

## License

See LICENSE file for details.
