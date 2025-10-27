# Heltec WiFi LoRa 32 V4 Custom Board Definition

## Overview

This document describes the custom board definition created for the Heltec WiFi LoRa 32 V4 boards to properly support their 16MB flash configuration and ESP32-S3R2 specifications.

## Problem

The original configuration was using the V3 board definition (`heltec_wifi_lora_32_V3`) which assumes an 8MB flash configuration. This caused the 16MB flash space of the V4 boards to be underutilized, with only 8MB being recognized and available for use.

## Solution

### 1. Custom Board Definition

Created a new board definition file: `boards/heltec_wifi_lora_32_V4.json`

**Key differences from V3:**
- **Name**: "Heltec WiFi LoRa 32 (V4)"
- **Flash Size**: 16MB (vs 8MB in V3)
- **RAM**: 512KB (vs ~327KB in V3) - reflecting ESP32-S3R2 specifications
- **Maximum Size**: 16,777,216 bytes (16MB)
- **Board Flag**: `-DARDUINO_heltec_wifi_lora_32_V4`
- **Default Partition**: `default_16MB.csv`
- **PSRAM Support**: Built-in with `-DBOARD_HAS_PSRAM`

**ESP32-S3R2 Specifications Supported:**
- 384KB ROM
- 512KB SRAM  
- 16KB RTC SRAM
- 2MB PSRAM (via `-DBOARD_HAS_PSRAM`)
- 16MB Flash

### 2. 16MB Partition Table

Created `default_16MB.csv` with optimized partition layout:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,    # 20KB for NVS
otadata,  data, ota,     0xe000,  0x2000,    # 8KB for OTA data
app0,     app,  ota_0,   0x10000, 0x300000,  # 3MB for App partition 0
app1,     app,  ota_1,   0x310000,0x300000,  # 3MB for App partition 1
spiffs,   data, spiffs,  0x610000,0x9F0000,  # ~10MB for SPIFFS filesystem
```

**Benefits:**
- **3MB app partitions**: Large enough for complex applications with web interfaces
- **~10MB SPIFFS**: Ample space for web assets, configurations, and data storage
- **OTA support**: Dual app partitions enable over-the-air updates

### 3. Updated PlatformIO Configuration

Modified `platformio.ini`:

```ini
[env:heltec_wifi_lora_32_V4]
board = heltec_wifi_lora_32_V4          # Now uses custom board definition
board_build.flash_size = 16MB           # Correctly specifies 16MB
board_build.partitions = default_16MB.csv # Uses 16MB partition table
```

## Hardware Differences Between V3 and V4

The custom board definition accounts for these key differences:

### V3 vs V4 Specifications:
| Feature | V3                  | V4                                          |
| ------- | ------------------- | ------------------------------------------- |
| MCU     | ESP32-S3            | ESP32-S3R2                                  |
| Flash   | 8MB                 | 16MB                                        |
| PSRAM   | External (optional) | 2MB built-in                                |
| RAM     | ~327KB              | 512KB                                       |
| LoRa PA | No                  | Yes (handled in code with `HELTEC_V4` flag) |

### Code-Level Differences (Already Handled):
- **Power Enable Pins**: Different peripheral power control pins
- **LoRa PA**: V4 includes power amplifier support  
- **PSRAM**: V4 has built-in 2MB PSRAM vs optional external on V3

## Verification

The build output confirms proper configuration:
```
HARDWARE: ESP32S3 240MHz, 512KB RAM, 16MB Flash
board_build.flash_size: 16MB
board_build.partitions: default_16MB.csv
```

## File Structure

```
LoRaTNCX/
├── boards/
│   └── heltec_wifi_lora_32_V4.json    # Custom board definition
├── default_16MB.csv                   # 16MB partition table
├── platformio.ini                     # Updated configuration
└── [existing files...]
```

## Benefits

1. **Full Flash Utilization**: All 16MB of flash memory is now accessible
2. **Larger App Partitions**: 3MB per app vs 2MB, supporting more complex applications
3. **Expanded SPIFFS**: ~10MB vs ~4MB for web assets and data storage
4. **Proper Hardware Recognition**: Correct RAM and flash specifications
5. **Future-Proof**: Dedicated V4 board definition for ongoing development

## Build Verification

The configuration has been tested and confirmed working:
- ✅ Build completes successfully
- ✅ 16MB flash properly recognized  
- ✅ 512KB RAM correctly allocated
- ✅ PSRAM support enabled
- ✅ V4-specific flags properly set

This implementation ensures the Heltec WiFi LoRa 32 V4 boards can fully utilize their enhanced hardware capabilities while maintaining compatibility with the existing codebase.