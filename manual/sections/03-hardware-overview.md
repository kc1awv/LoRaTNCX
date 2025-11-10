# 3. Hardware Overview

This section provides detailed information about the supported hardware platforms, their capabilities, and key differences.

## 3.1. Heltec WiFi LoRa 32 V3 Board

### Overview
The Heltec WiFi LoRa 32 V3 is the original board in the series, featuring an ESP32-S3 microcontroller and SX1262 LoRa radio module.

### Key Specifications
- **Microcontroller**: ESP32-S3 (dual-core, WiFi, Bluetooth)
- **LoRa Radio**: SX1262 (direct control, no external PA)
- **Display**: 0.96" OLED (128x64 SSD1306)
- **Memory**: 8MB flash, 512KB RAM
- **Power**: 3.3V-5V input via USB-C
- **Frequency Range**: 433-928 MHz ISM bands
- **Antenna**: SMA connector
- **GNSS**: Not supported (no dedicated connector)

### Pin Configuration
```
ESP32-S3 GPIO Pinout (V3):
- GPIO 0:  User button (BOOT)
- GPIO 1:  Battery ADC input
- GPIO 8:  LoRa SPI CS
- GPIO 9:  LoRa SPI SCK
- GPIO 10: LoRa SPI MOSI
- GPIO 11: LoRa SPI MISO
- GPIO 12: LoRa RST
- GPIO 13: LoRa BUSY
- GPIO 14: LoRa DIO0/DIO1
- GPIO 35: Built-in LED
- GPIO 37: Battery ADC control
- GPIO 41: I2C SDA (OLED)
- GPIO 42: I2C SCL (OLED)
- GPIO 43: UART TX
- GPIO 44: UART RX
```

### Radio Architecture
The V3 uses direct SX1262 control without external power amplification. The radio module handles antenna switching internally, making the design simpler but with potentially lower transmit power compared to the V4.

### Battery Monitoring
- Uses GPIO 1 for ADC measurement
- GPIO 37 controls ADC circuit (active HIGH for V3.2, active LOW for original V3)
- Supports 3.3V-4.2V LiPo batteries

## 3.2. Heltec WiFi LoRa 32 V4 Board

### Overview
The Heltec WiFi LoRa 32 V4 is the enhanced version with external power amplification and GNSS support.

### Key Specifications
- **Microcontroller**: ESP32-S3 (dual-core, WiFi, Bluetooth)
- **LoRa Radio**: SX1262 with external PA
- **Display**: 0.96" OLED (128x64 SSD1306)
- **Memory**: 8MB flash, 512KB RAM
- **Power**: 3.3V-5V input via USB-C
- **Frequency Range**: 433-928 MHz ISM bands
- **Antenna**: SMA connector
- **GNSS**: Dedicated connector for Heltec GNSS module

### Pin Configuration
```
ESP32-S3 GPIO Pinout (V4):
- GPIO 0:  User button (BOOT)
- GPIO 1:  Battery ADC input
- GPIO 2:  LoRa PA enable
- GPIO 7:  LoRa PA power control
- GPIO 8:  LoRa SPI CS
- GPIO 9:  LoRa SPI SCK
- GPIO 10: LoRa SPI MOSI
- GPIO 11: LoRa SPI MISO
- GPIO 12: LoRa RST
- GPIO 13: LoRa BUSY
- GPIO 14: LoRa DIO0/DIO1
- GPIO 34: GNSS power control
- GPIO 35: Built-in LED
- GPIO 37: Battery ADC control
- GPIO 38: GNSS UART TX
- GPIO 39: GNSS UART RX
- GPIO 40: GNSS wake
- GPIO 41: GNSS PPS
- GPIO 42: GNSS reset
- GPIO 46: LoRa PA TX enable
- GPIO 3:  I2C SDA (OLED)
- GPIO 4:  I2C SCL (OLED)
- GPIO 43: UART TX
- GPIO 44: UART RX
```

### Radio Architecture
The V4 includes an external power amplifier that requires careful control sequencing. Three additional GPIO pins manage PA power, enable, and transmit states for optimal performance.

### Battery Monitoring
- Uses GPIO 1 for ADC measurement
- GPIO 37 controls ADC circuit (active HIGH)
- Supports 3.3V-4.2V LiPo batteries

## 3.3. Power Amplifier Control Differences

### V3 Board (No External PA)
- **Architecture**: Direct SX1262 control
- **Power Control**: Internal to SX1262 radio module
- **Initialization**: Simple radio initialization only
- **Transmit Power**: Limited by SX1262 internal PA (~20dBm max)
- **Complexity**: Lower hardware complexity

### V4 Board (External PA)
- **Architecture**: SX1262 + external RF power amplifier
- **Power Control**: Requires 3 GPIO pins for PA management
- **Initialization Sequence**:
  1. Configure PA control pins (GPIO 2, 7, 46)
  2. Enable PA power (GPIO 2 HIGH)
  3. Enable TX path (GPIO 46 HIGH)
  4. Initialize radio module
- **Transmit Power**: Higher output power capability
- **Complexity**: Higher hardware complexity, more precise timing required

### Firmware Handling
The firmware automatically detects board type at compile time using build flags:
- `-DARDUINO_HELTEC_WIFI_LORA_32_V3` for V3 boards
- `-DARDUINO_HELTEC_WIFI_LORA_32_V4` for V4 boards

This ensures correct PA control initialization for each hardware variant.

## 3.4. Antenna Connections

### SMA Connector
Both V3 and V4 boards feature SMA female antenna connectors for secure, professional antenna connections.

### Supported Antennas
- **Frequency Range**: Must match your operating frequency (433, 868, 915, 928 MHz)
- **Impedance**: 50Ω
- **Connector Type**: SMA male
- **Gain**: 2-5 dBi recommended for most applications

### Antenna Requirements
- **LoRa Operation**: Quarter-wave or half-wave antennas optimized for your frequency
- **VSWR**: < 2.0:1 recommended for best performance
- **Power Handling**: Must handle transmit power (up to 20dBm/100mW)

⚠️ **Important**: Never transmit without a proper antenna connected. This can damage the radio module and violate FCC regulations.

### Antenna Selection Examples
- **433 MHz**: ¼-wave whip (164mm) or ½-wave dipole (328mm)
- **868/915 MHz**: ¼-wave whip (82mm) or ½-wave dipole (164mm)
- **928 MHz**: ¼-wave whip (76mm) or ½-wave dipole (152mm)

## 3.5. GNSS Module (V4 Only)

### Hardware Interface
The V4 board includes a dedicated 6-pin GNSS connector for the Heltec GNSS module.

### Pin Assignment
```
GNSS Connector (J1):
Pin 1: VGNSS (3.3V power)
Pin 2: GND
Pin 3: GNSS_TX (module TX -> ESP32 RX)
Pin 4: GNSS_RX (ESP32 TX -> module RX)
Pin 5: PPS (Pulse Per Second output)
Pin 6: WAKE (wake-up signal)
```

### Supported Module
- **Heltec GNSS Module** [L76K GNSS Module](https://heltec.org/project/l76-gnss-module/)
- **GNSS Systems**: GPS, GLONASS, BeiDou, QZSS
- **Antenna**: Attached as part of the GNSS module
- **Power**: 3.3V, ~30-40mA current draw

### ESP32 Pin Mapping
- **GPIO 34**: VGNSS_CTRL (power control, active LOW)
- **GPIO 38**: GNSS UART TX (ESP32 → GNSS)
- **GPIO 39**: GNSS UART RX (GNSS → ESP32)
- **GPIO 40**: GNSS WAKE
- **GPIO 41**: GNSS PPS
- **GPIO 42**: GNSS RST

### Features
- **NMEA Output**: Standard NMEA-0183 sentences
- **TCP Streaming**: GNSS data available over network
- **Serial Passthrough**: Direct USB access for debugging
- **Location Services**: Position, velocity, time data
- **Multiple Clients**: Up to 4 simultaneous TCP connections

## 3.6. OLED Display

### Display Specifications
- **Type**: SSD1306 OLED controller
- **Size**: 0.96" diagonal (128x64 pixels)
- **Interface**: I2C
- **Colors**: Monochrome (blue pixels on black background)
- **Viewing Angle**: High contrast, wide viewing angle

### Pin Connections
**V3 Board:**
- SDA: GPIO 41
- SCL: GPIO 42

**V4 Board:**
- SDA: GPIO 3
- SCL: GPIO 4

### Display Content
The OLED display shows real-time status information:

1. **Boot Screen** (2 seconds):
   - LoRaTNCX logo
   - Firmware version
   - Board type (V3/V4)

2. **Status Screen** (rotating display):
   - Current LoRa frequency and parameters
   - WiFi connection status
   - Battery voltage
   - GNSS status (if applicable)
   - Blank screen to save power

### Power Management
- **Vext Control**: Automatically powered on/off to save battery
- **Low Power**: Minimal current draw when not updating
- **Configurable**: Can be disabled via web interface

### Configuration
- **Rotation**: Automatic cycling through different views
- **Timeout**: Configurable display timeout
- **Brightness**: Fixed (OLED technology)
- **Orientation**: Landscape (128x64)

### Benefits
- **Field Operation**: Check status without phone/computer
- **Configuration Verification**: Confirm settings at a glance
- **Battery Monitoring**: Real-time voltage display
- **Professional Appearance**: Status display enhances user experience

---

[← Previous: Getting Started](02-getting-started.md) | [Back to Main Manual](../user_manual.md) | [Next: Software Architecture →](04-software-architecture.md)