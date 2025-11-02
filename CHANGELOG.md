# LoRaTNCX Changelog

## Version 1.3 - KISS Protocol LoRa Extensions

### Major Features
- **Extended KISS Protocol Support**
  - LoRa-specific KISS commands for radio configuration
  - Compatible with packet radio software (APRS, Winlink, etc.)
  - Hardware-specific commands via SETHARDWARE (0x06)
  - Multi-byte parameter support for complex settings

### New KISS Commands
- `SET_FREQ` (0x10) - Set frequency (4-byte Hz parameter)
- `SET_TXPOWER` (0x12) - Set TX power in dBm
- `SET_BANDWIDTH` (0x13) - Set bandwidth (encoded index)
- `SET_SF` (0x14) - Set spreading factor (6-12)
- `SET_CR` (0x15) - Set coding rate (5-8 for 4/5 to 4/8)
- `SET_PREAMBLE` (0x16) - Set preamble length (2-byte parameter)
- `SET_SYNCWORD` (0x17) - Set sync word
- `SET_CRC` (0x18) - Enable/disable CRC
- `SELECT_BAND` (0x19) - Select predefined frequency band
- `GET_CONFIG` (0x1A) - Get current configuration
- `SAVE_CONFIG` (0x1B) - Save configuration to flash
- `RESET_CONFIG` (0x1C) - Reset to defaults

### KISS Protocol Features
- **Standard TNC Compatibility**
  - TXDELAY, PERSISTENCE, SLOTTIME, TXTAIL, FULLDUPLEX
  - RETURN command to exit KISS mode
  - TNC-2 compatible silent operation
- **Response System**
  - ACK/NAK responses for configuration commands
  - Multi-frame responses for GET_CONFIG
  - Error handling for invalid parameters

### Integration Support
- **Packet Radio Applications**
  - APRS software compatibility
  - Winlink integration potential
  - Custom KISS client support
- **Programming Examples**
  - Python KISS client example
  - Raw hex command documentation
  - Frame format specifications

### Documentation Updates
- Added KISS command reference to README.md
- Created docs/KISS_TESTING.md for validation
- Extended EXAMPLES.md with KISS usage scenarios
- Complete command encoding tables

## Version 1.2 - Frequency Band Management System

### Major Features
- **Runtime Frequency Band Configuration**
  - Complete replacement of build-time frequency flags
  - Support for ISM and Amateur Radio bands
  - Regional band configurations via JSON files
  - Legal compliance awareness with license tracking

### New Commands
- `lora bands` - Show available frequency bands
- `lora bands ism` - Filter ISM bands (no license required)  
- `lora bands amateur` - Filter amateur radio bands
- `lora band` - Show current band configuration
- `lora band <id>` - Select frequency band (e.g., ISM_915, AMATEUR_70CM)

### Supported Bands
#### ISM Bands (No License Required)
- **ISM_433**: 433.05-433.92 MHz (Global)
- **ISM_470_510**: 470-510 MHz (China/Asia)
- **ISM_863_870**: 863-870 MHz (Europe)
- **ISM_902_928**: 902-928 MHz (North America)

#### Amateur Radio Bands (License Required)  
- **AMATEUR_70CM**: 420-450 MHz (Global)
- **AMATEUR_33CM**: 902-928 MHz (US)
- **AMATEUR_23CM**: 1240-1300 MHz (Global)
- **AMATEUR_FREE**: 144-1300 MHz (Free selection)

### Breaking Changes
- **Removed build environments**: `*_433` environments no longer needed
- **Removed build flags**: `FREQ_BAND_433`, `FREQ_BAND_868` flags removed
- **Default behavior**: All builds default to ISM_902_928 (North American ISM)

### Migration Guide
- Old: Compile with `heltec_wifi_lora_32_V4_433` for 433 MHz
- New: Use any build, then `lora band ISM_433` at runtime

## Version 1.1 - RadioLib Integration

### Major Changes
- **Migrated from LoRaWan_APP to RadioLib 7.4.0**
  - Modern, well-maintained library with active development
  - Better SX1262 support and performance
  - Improved error handling and status reporting
  - More flexible configuration options

### Breaking Changes
- **Frequency units changed from Hz to MHz**
  - Old: `lora freq 868000000` (Hz)
  - New: `lora freq 868.0` (MHz)
- **Bandwidth units changed to kHz**
  - Old: `lora bw 0` (index)
  - New: `lora bw 125` (kHz) or `lora bw 0` (legacy support)
- **Coding rate format updated**
  - Old: 1-4 (for 4/5 to 4/8)
  - New: 5-8 (denominator) with legacy 1-4 support
- **SNR values now float instead of integer**
  - Better precision for signal quality measurements

### New Features
- **Automatic PA Control for V4**
  - RadioLib handles PA enable/disable during TX/RX automatically
  - Improved power efficiency and reliability
- **Better Error Reporting**
  - Configuration methods return error codes
  - More detailed error messages for troubleshooting
- **Enhanced Statistics**
  - Floating point SNR values for better precision
  - Improved RSSI reporting accuracy

### Hardware Support Improvements
- **SX1262 Native Support**
  - Direct SX1262 driver instead of generic LoRa abstraction
  - Better performance and feature access
- **Improved Pin Configuration**
  - Automatic SPI initialization
  - Proper interrupt handling
- **Power Management**
  - More accurate power level control
  - Better PA management for V4

### Library Dependencies
- **Added**: RadioLib 7.4.0
- **Removed**: LoRaWan_APP dependency

### Build System Updates
- **Simplified platformio.ini**
  - Common configuration section for all environments
  - Automatic RadioLib dependency management
- **Six Build Environments**
  - V3 and V4 variants for 433, 868, and 915 MHz bands
  - Automatic frequency band selection via build flags

### Documentation Updates
- Updated README.md with RadioLib information
- Revised EXAMPLES.md with new command syntax
- Added this CHANGELOG.md for version tracking

### Bug Fixes
- Fixed compilation errors with missing LoRaWan_APP library
- Fixed `getIrqStatus()` method error - updated to proper RadioLib interrupt handling
- Improved interrupt handling reliability with proper flag management
- Better memory management and buffer handling

## Version 1.0 - Initial Release

### Features
- Basic LoRa communication using LoRaWan_APP library
- Serial console interface
- Support for Heltec WiFi LoRa 32 V3 and V4
- Multiple frequency band configurations
- Hardware-specific PA control for V4

### Known Issues (Fixed in v1.1)
- LoRaWan_APP library dependency issues
- Limited configuration flexibility
- Integer-only SNR reporting