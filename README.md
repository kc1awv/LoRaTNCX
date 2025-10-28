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
- [x] **Build Success**: Project compiles without errors

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

### 1. LoRa Radio Class (`LoRaRadio`)
- SX1262 initialization and configuration
- Frequency, power, bandwidth, and spreading factor control
- Transmit and receive operations
- RSSI and SNR reporting
- Power amplifier control

### 2. Hardware Abstraction
- Comprehensive pin definitions in `HardwareConfig.h`
- Power amplifier initialization sequence
- Pin documentation in `HeltecV4Pins.h`

### 3. Test Application
- Automatic LoRa radio initialization
- Periodic test beacon transmission
- Receive polling with signal quality reporting
- Status LED indication

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
1. ✅ Serial communication initialization (115200 baud)
2. ✅ Hardware pin configuration and PA initialization
3. ✅ LoRa radio initialization with RadioLib
4. ✅ Test beacon transmission
5. ✅ Status reporting and pin configuration display

### Runtime Operation
1. ✅ Periodic test beacon transmission (every 30 seconds)
2. ✅ Continuous receive polling
3. ✅ RSSI/SNR reporting for received packets
4. ✅ Status LED blinking (system alive indicator)

## Next Development Steps

### Immediate (Ready to Implement)
- [ ] **KISS Protocol**: Implement KISS TNC protocol for packet handling
- [ ] **Serial Interface**: Add KISS frame encoding/decoding
- [ ] **Packet Routing**: Basic store-and-forward functionality
- [ ] **OLED Display**: Status and packet information display

### Near Term
- [ ] **GNSS Integration**: GPS/GNSS coordinate reporting
- [ ] **APRS Support**: Automatic Packet Reporting System
- [ ] **Web Interface**: Configuration and monitoring via WiFi
- [ ] **OTA Updates**: Over-the-air firmware updates

### Future Enhancements
- [ ] **Mesh Networking**: Multi-hop packet routing
- [ ] **Encryption**: Secure packet transmission
- [ ] **Multiple Protocols**: Support for additional packet protocols
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

### October 28, 2025
1. **Complete Repository Reset**: Removed all previous code for fresh start
2. **PlatformIO Initialization**: New ESP32-S3 project with Arduino framework
3. **Custom Board Definition**: Created `heltec_wifi_lora_32_V4.json` with proper ESP32-S3 configuration
4. **Pin Discovery**: Analyzed Heltec factory firmware to find critical PA pins
5. **RadioLib Integration**: Added RadioLib 7.4.0 for SX1262 support
6. **LoRa Interface**: Implemented complete LoRaRadio wrapper class
7. **Test Application**: Working TX/RX test with signal quality reporting

## Files Structure
```
LoRaTNCX/
├── platformio.ini              # Project configuration
├── boards/
│   └── heltec_wifi_lora_32_V4.json  # Custom board definition
├── include/
│   ├── HardwareConfig.h        # Hardware pin definitions
│   ├── HeltecV4Pins.h         # Pin reference documentation
│   └── LoRaRadio.h            # LoRa radio interface
├── src/
│   ├── main.cpp               # Main application
│   └── LoRaRadio.cpp          # LoRa radio implementation
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
