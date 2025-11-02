# LoRaTNCX - LoRa Terminal Node Controller

A terminal node controller implementation for Heltec WiFi LoRa 32 V3 and V4 development boards using the RadioLib library.

## Dependencies

This project uses the [RadioLib](https://github.com/jgromes/RadioLib) library version 7.4.0 for LoRa communication, providing a modern and well-maintained interface to the SX1262 LoRa transceiver.

## Hardware Support

### Heltec WiFi LoRa 32 V3
- **LoRa Chip**: SX1262
- **Frequency Bands**: 433-510 MHz, 863-928 MHz
- **Maximum TX Power**: 22 dBm
- **Default TX Power**: 10 dBm
- **PA Control**: Not available (direct radio control)

### Heltec WiFi LoRa 32 V4
- **LoRa Chip**: SX1262
- **Frequency Bands**: 433-510 MHz, 863-928 MHz  
- **Maximum TX Power**: 22 dBm
- **Default TX Power**: 14 dBm
- **PA Control**: Available (automatic TX/RX switching)

## Frequency Band System

LoRaTNCX features a comprehensive runtime frequency band management system:

### Supported Bands
- **ISM Bands**: 433MHz, 470-510MHz, 863-870MHz, 902-928MHz (No license required)
- **Amateur Radio**: 70cm (420-450MHz), 33cm (902-928MHz), 23cm (1240-1300MHz)
- **Custom Bands**: User-defined regional configurations

### Key Features
- **Runtime Selection**: Change bands without recompiling
- **Legal Compliance**: Built-in frequency validation and licensing awareness
- **Regional Support**: Extensible configuration files for local regulations
- **Hardware Validation**: Automatic compatibility checking

### Getting Started
- **Quick Start**: [Quick Start Guide](docs/QUICK_START_BANDS.md) - Get up and running fast
- **Detailed Docs**: [Frequency Band System](docs/FREQUENCY_BAND_SYSTEM.md) - Complete documentation
- **Custom Regions**: [SPIFFS Upload Guide](docs/SPIFFS_UPLOAD_GUIDE.md) - Add regional bands

## Build Environments

The project provides multiple build environments for different hardware versions:

### Available Environments
- `heltec_wifi_lora_32_V3` - Heltec WiFi LoRa 32 V3
- `heltec_wifi_lora_32_V4` - Heltec WiFi LoRa 32 V4  

**Note**: All frequency bands are available at runtime regardless of build environment. Use `lora band` commands to select your desired frequency band.

## Building

To build for a specific hardware version:
```bash
# Build for Heltec V3
platformio run -e heltec_wifi_lora_32_V3

# Build for Heltec V4  
platformio run -e heltec_wifi_lora_32_V4

# Build all environments
platformio run
```

## Serial Console Commands

The device provides a serial console interface with the following commands:

### System Commands
- `help` - Show available commands
- `status` - Display system information (uptime, memory, etc.)
- `clear` - Clear the terminal screen
- `reset` - Restart the device

### LoRa Commands
- `lora status` - Show current LoRa radio state
- `lora config` - Display LoRa configuration
- `lora stats` - Show transmission/reception statistics
- `lora send <message>` - Transmit a LoRa message
- `lora rx` - Start continuous receive mode
- `lora freq <mhz>` - Set frequency in MHz (validates against current band)
- `lora power <dbm>` - Set TX power in dBm
- `lora sf <sf>` - Set spreading factor (7-12)
- `lora bw <khz>` - Set bandwidth in kHz (125/250/500) or shorthand (0/1/2)
- `lora cr <cr>` - Set coding rate (5-8 for 4/5-4/8) or legacy (1-4)

### Frequency Band Commands
- `lora bands` - Show all available frequency bands
- `lora bands ism` - Show ISM bands (no license required)
- `lora bands amateur` - Show amateur radio bands
- `lora band` - Show current band configuration  
- `lora band <id>` - Select frequency band (e.g., ISM_915, AMATEUR_70CM)

## LoRa Configuration

### Default Settings
- **Bandwidth**: 125 kHz
- **Spreading Factor**: SF7
- **Coding Rate**: 4/5
- **Preamble Length**: 8 symbols
- **CRC**: Enabled
- **IQ Inversion**: Disabled

### Frequency Configuration
Frequency bands are configured at runtime using the new band management system. The device defaults to the North American ISM band (902-928 MHz) and users can select any available band using console commands. See the Frequency Band Commands section for usage details.

### Power Settings
Power levels are automatically limited based on hardware:
- **V3**: -9 to 22 dBm
- **V4**: -9 to 22 dBm

The V4 hardware automatically enables the PA (Power Amplifier) for transmit operations and disables it for receive operations through GPIO control.

## Usage Examples

### Basic Communication Test
1. Flash two devices with the same frequency band configuration
2. Connect to serial console (115200 baud)
3. On device 1: `lora send Hello World`
4. On device 2: You should see the received message
5. Both devices automatically return to receive mode after transmission

### Changing Configuration
```
> lora freq 434.0
[LoRa] Frequency set to 434.0 MHz

> lora power 15
[LoRa] TX power set to 15 dBm

> lora sf 10
[LoRa] Spreading factor set to SF10

> lora bw 250
[LoRa] Bandwidth set to 250.0 kHz
```

### Monitoring
```
> lora stats
[LoRa] Statistics:
  TX Count: 5
  RX Count: 12
  Last RSSI: -45 dBm
  Last SNR: 8 dB
  Current State: RX
```

## Hardware Differences

The main differences between V3 and V4 that affect software:

1. **Power Amplifier Control**: V4 has dedicated PA control pins that are automatically managed during TX/RX operations
2. **Default Power**: V4 uses higher default power (14 dBm vs 10 dBm) due to PA availability
3. **OLED I2C Pins**: Different I2C pin assignments for the display (not yet implemented)

The LoRaRadio class automatically handles these differences based on compile-time board detection using RadioLib's hardware abstraction.

## License

This project is open source. Please refer to the license file for details.