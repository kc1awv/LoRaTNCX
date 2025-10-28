# CLAUDE.md - Project Knowledge Base

This file contains essential information for AI assistants working on the LoRaTNCX project.

## Project Overview

**LoRaTNCX** is a fresh rewrite of a LoRa KISS Terminal Node Controller (TNC) for amateur radio communications.

### Hardware Target
- **Board**: Heltec WiFi LoRa 32 V4
- **Processor**: ESP32-S3 @ 240MHz
- **RAM**: 512KB
- **Flash**: 16MB
- **Features**: WiFi, Bluetooth, LoRa radio, PSRAM, USB CDC

## Project Status
- **Current Phase**: Fresh start - all previous code removed
- **Repository**: Clean slate with basic PlatformIO project structure
- **Branch**: `rewrite` (working branch)
- **Last Reset**: October 28, 2025

## Development Environment

### PlatformIO Configuration
- **Environment**: `heltec_wifi_lora_32_V4`
- **Platform**: espressif32
- **Framework**: Arduino
- **Board Definition**: Custom (`boards/heltec_wifi_lora_32_V4.json`)
- **Partition Table**: `default_16MB.csv` (16MB layout)

### Build System
- **Build Command**: `platformio run -e heltec_wifi_lora_32_V4`
- **Upload Command**: `platformio run -e heltec_wifi_lora_32_V4 -t upload`
- **Monitor Speed**: 115200 baud
- **Upload Speed**: 921600 baud

## Project Goals

### Primary Objectives
1. **KISS TNC Implementation**: Implement KISS (Keep It Simple, Stupid) protocol for packet radio
2. **LoRa Integration**: Support for LoRa radio communication
3. **Clean Architecture**: Well-structured, maintainable code
4. **USB CDC Interface**: Use native USB for computer communication

### Secondary Features (Future)
- APRS tracker functionality
- GNSS integration for position reporting
- Web interface for configuration
- Multiple protocol support
- Battery management

## Development Guidelines

### Code Standards
- Use Arduino framework for ESP32-S3
- Follow consistent naming conventions
- Include header comments in all files
- Document complex algorithms
- Use meaningful variable names

### Architecture Principles
- Modular design with clear separation of concerns
- Use appropriate abstractions for hardware interfaces
- Implement proper error handling
- Design for testability

### File Organization
```
src/           # Main application source
include/       # Project headers
lib/           # Project-specific libraries
boards/        # Custom board definitions
test/          # Unit tests
```

## Hardware-Specific Notes

### ESP32-S3 Features to Utilize
- **PSRAM**: Available for larger data structures
- **USB CDC**: Native USB for debugging and KISS interface
- **Dual Core**: Can utilize both cores for performance
- **WiFi/Bluetooth**: Available for future features

### LoRa Radio
- **Chip**: SX1262 (on Heltec V4)
- **Frequency**: Configurable (amateur radio bands)
- **Power**: Configurable output power
- **Antenna**: External connector

## Current Project Structure
```
LoRaTNCX/
├── boards/heltec_wifi_lora_32_V4.json  # Custom board definition
├── default_16MB.csv                     # Partition table
├── include/
│   ├── HeltecV4Pins.h                  # Pin reference documentation
│   └── HardwareConfig.h                # Hardware configuration
├── lib/                                 # Libraries
├── src/main.cpp                         # Main application
├── test/                                # Tests
├── platformio.ini                       # PlatformIO config
├── CLAUDE.md                           # This file
├── README.md                           # Project documentation
└── LICENSE                             # License file
```

## Hardware Pin Configuration
The project now includes proper pin definitions for the Heltec WiFi LoRa 32 V4:

### Key Pins:
- **LoRa Radio (SX1262)**: RST=12, BUSY=13, DIO0=14, SS=8
- **OLED Display**: SDA=17, SCL=18, RST=21
- **GNSS Module (optional)**: VCTL=34, RX=38, TX=39, Wake=40, PPS=41, RST=42
- **Status LED**: Pin 35
- **Power Control**: Vext=36
- **SPI Bus**: SCK=9, MOSI=10, MISO=11
- **Default I2C**: SDA=3, SCL=4 (note: OLED uses different pins)

## Build Status
- **Last Successful Build**: October 28, 2025
- **RAM Usage**: 3.5% (18,500 / 524,288 bytes)
- **Flash Usage**: 3.9% (253,229 / 6,553,600 bytes)
- **Build Time**: ~3 seconds

## Next Steps
1. ✅ Set up pin definitions and hardware configuration
2. Add LoRa radio library (RadioLib or similar)
3. Implement basic LoRa radio interface
4. Implement KISS protocol handling
5. Add configuration management
6. Add error handling and logging
7. Add OLED display support

## Important Reminders
- Always test builds after changes
- Update this file when adding new components
- Document any hardware-specific configurations
- Keep the README.md updated for users
- Use proper Git commit messages

## Dependencies (To Be Added)
- LoRa radio library (RadioLib or similar)
- KISS protocol implementation
- Configuration management library

---
*Last Updated: October 28, 2025*
*AI Assistant: Please read and update this file as the project evolves*