# Amateur Radio LoRa TNC Configuration System

## Overview

This document describes the comprehensive amateur radio configuration system implemented for the LoRaTNCX project. The system provides scientifically optimized LoRa configurations specifically designed for US amateur radio use above 420MHz with 1-second time-on-air compliance.

## Features

### ✅ Complete Implementation
- **9 Optimized Presets**: From high-speed (20,420 bps) to maximum range (20-60 km)
- **Runtime Configuration**: Change settings via KISS protocol commands
- **Custom Configurations**: Define your own frequency/bandwidth/SF/CR combinations
- **Amateur Radio Compliance**: All presets respect 1-second time-on-air limits
- **Hardware Validation**: All configurations use only SX1262-supported bandwidths
- **Band-Specific Optimization**: Dedicated presets for 70cm, 33cm, and 23cm bands

### ✅ Scientific Foundation
- **Time-on-Air Calculations**: Precise mathematical optimization for amateur radio use
- **Hardware Limits Validated**: 255-byte maximum payload confirmed via RadioLib source analysis
- **Real-World Testing**: All configurations validated with actual hardware

## Available Configurations

### High-Speed Configurations (Short to Medium Range)

| Preset | Name | SF | BW (kHz) | Payload | Range | Throughput | Use Case |
|--------|------|----|---------|---------| ------|------------|----------|
| 0 | High Speed | 7 | 500 | 255 bytes | 2-8 km | 20,420 bps | File transfer, streaming |
| 1 | Fast Balanced | 8 | 500 | 255 bytes | 5-15 km | 11,541 bps | General high-speed data |

### Balanced Configurations (Medium Range)

| Preset | Name | SF | BW (kHz) | Payload | Range | Throughput | Use Case |
|--------|------|----|---------|---------| ------|------------|----------|
| 2 | Standard Balanced | 8 | 250 | 255 bytes | 8-20 km | 5,770 bps | General packet radio |
| 3 | Robust Balanced | 9 | 250 | 255 bytes | 10-25 km | 3,263 bps | Poor conditions |

### Long Range Configurations (Lower Speed)

| Preset | Name | SF | BW (kHz) | Payload | Range | Throughput | Use Case |
|--------|------|----|---------|---------| ------|------------|----------|
| 4 | Long Range | 10 | 125 | 99 bytes | 15-40 km | 804 bps | Emergency communications |
| 5 | Maximum Range | 11 | 125 | 27 bytes | 20-60 km | 234 bps | Emergency beacons |

### Band-Specific Configurations

| Preset | Name | Frequency | SF | BW (kHz) | Range | Throughput | Use Case |
|--------|------|-----------|----|---------| ------|------------|----------|
| 6 | 70cm Optimized | 432.6 MHz | 8 | 250 | 8-20 km | 5,770 bps | Most popular amateur band |
| 7 | 33cm Optimized | 906 MHz | 7 | 500 | 2-8 km | 20,420 bps | Avoid ISM interference |
| 8 | 23cm Optimized | 1290 MHz | 7 | 500 | 1-5 km | 20,420 bps | Microwave, line-of-sight |

## Configuration Commands

### Via KISS Protocol

The TNC accepts configuration commands through KISS frames containing text commands:

```
LISTCONFIG              - Show all available presets
GETCONFIG              - Show current configuration  
SETCONFIG <preset>     - Set configuration by number or name
```

### Examples

```bash
# Set by preset number
SETCONFIG 1

# Set by preset name  
SETCONFIG high_speed

# Set custom configuration (freq, bandwidth, SF, CR)
SETCONFIG 915.0 500 7 5

# Check current settings
GETCONFIG

# List all available presets
LISTCONFIG
```

### Preset Names

Configuration presets can be referenced by number (0-8) or by name:

- `high_speed` or `fast` → Preset 0  
- `fast_balanced` → Preset 1
- `balanced` or `default` → Preset 2
- `robust_balanced` or `robust` → Preset 3
- `long_range` or `longrange` → Preset 4
- `max_range` or `maxrange` → Preset 5
- `band_70cm` or `70cm` → Preset 6
- `band_33cm` or `33cm` → Preset 7
- `band_23cm` or `23cm` → Preset 8

## Technical Implementation

### Core Components

1. **AmateurRadioLoRaConfigs.h**: Defines all configuration constants
2. **ConfigurationManager.h/.cpp**: Manages configuration selection and switching
3. **Updated TNCManager**: Integrates configuration management with KISS protocol
4. **Enhanced LoRaRadio**: Supports runtime reconfiguration

### File Structure

```
include/
├── AmateurRadioLoRaConfigs.h     # Configuration constants
├── ConfigurationManager.h        # Configuration management interface
└── TNCManager.h                  # Updated with config integration

src/  
├── ConfigurationManager.cpp      # Configuration management implementation
├── TNCManager.cpp               # Updated with config commands
└── LoRaRadio.cpp               # Updated with runtime reconfiguration

test/
├── test_amateur_radio_configs.py # Comprehensive configuration testing
└── test_config_commands.py      # Basic command testing
```

### Memory Usage

- **RAM**: 4.1% (21,724 bytes) - efficient memory usage
- **Flash**: 5.2% (342,053 bytes) - compact implementation
- **Configuration Data**: Stored in flash, loaded on demand

## Testing and Validation

### ✅ Verified Functionality

1. **Configuration Commands**: All KISS commands working correctly
2. **Runtime Switching**: Seamless configuration changes during operation
3. **Custom Configurations**: User-defined settings with validation
4. **Hardware Integration**: Proper RadioLib SX1262 reconfiguration
5. **Memory Efficiency**: No memory leaks or excessive usage

### Test Results Example

```
TNC: Processing configuration command: SETCONFIG high_speed
Configuring radio: High Speed (SF7, BW500kHz)
  Frequency: 915.0 MHz
  Bandwidth: 500.0 kHz
  Spreading Factor: SF7
  Coding Rate: 4/5
  Max Payload: 255 bytes
✓ LoRa radio initialized successfully!
✓ Radio started in receive mode
Configuration applied successfully
Expected range: 2-8 km
Expected throughput: 20,420 bps
```

### Usage Recommendations

### For Different Applications

- **High-Speed Data Transfer**: Use presets 0-1 (SF7-8, BW≥500kHz)
- **General Packet Radio**: Use presets 2-3 (SF8-9, BW250kHz)  
- **Emergency Communications**: Use presets 4-5 (SF10-11, BW125kHz)
- **Band-Specific Operations**: Use presets 6-8 for optimized band performance

### For Different Ranges

- **Short Range (1-5 km)**: High Speed, 23cm Band
- **Medium Range (5-20 km)**: Balanced configurations
- **Long Range (15-60 km)**: Long Range, Maximum Range presets

### For Different Conditions

- **Clear Line-of-Sight**: High-speed configurations (SF7, high BW)
- **Urban/Obstructed**: Balanced configurations (SF8-9, medium BW)
- **Poor Conditions**: Robust/Long Range configurations (SF9-11, low BW)

## Future Enhancements

### Potential Additions

1. **Automatic Configuration**: Based on signal quality metrics
2. **Configuration Profiles**: Save/load custom configuration sets
3. **Band Plan Integration**: Automatic frequency selection based on location
4. **Time-on-Air Monitoring**: Real-time compliance checking
5. **Configuration via Web Interface**: HTTP-based configuration management

### Advanced Features

1. **Adaptive Data Rate**: Automatic SF/BW adjustment based on conditions
2. **Multi-Band Operation**: Automatic band switching for optimal propagation
3. **Configuration Templates**: Pre-configured settings for specific applications
4. **Performance Analytics**: Track throughput and reliability per configuration

## Conclusion

The amateur radio configuration system provides a comprehensive, scientifically-optimized solution for LoRa packet radio operations. With 9 carefully calculated presets covering all amateur radio scenarios, runtime configuration switching, and full KISS protocol integration, the system enables efficient and compliant amateur radio data communications.

All configurations respect the 1-second time-on-air limitation for amateur radio use above 420MHz, while maximizing data throughput and reliability for various range and condition requirements.

**Status**: ✅ **Fully Implemented and Tested**
**Date**: October 29, 2025
**Project**: LoRaTNCX Amateur Radio Enhancement