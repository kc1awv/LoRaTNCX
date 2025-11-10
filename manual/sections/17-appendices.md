# 17. Appendices

## 17.1. KISS Protocol Reference

### Frame Format

LoRaTNCX implements the standard KISS (Keep It Simple, Stupid) protocol for TNC communication. KISS frames use byte stuffing to ensure reliable transmission over serial/TCP connections.

#### Frame Structure
```
FEND + Command/Data + FEND
```

Where:
- **FEND** (`0xC0`): Frame delimiter
- **Command/Data**: Frame payload
- **Data frames** contain packet data for transmission
- **Command frames** contain protocol control information

#### Byte Stuffing
KISS uses byte stuffing to prevent FEND bytes from appearing in data:

| Original Byte | Stuffed Sequence         |
| ------------- | ------------------------ |
| `0xC0` (FEND) | `0xDB 0xDC` (FESC TFEND) |
| `0xDB` (FESC) | `0xDB 0xDD` (FESC TFESC) |

### KISS Commands

#### Standard Commands
| Command           | Value  | Description                                  |
| ----------------- | ------ | -------------------------------------------- |
| `CMD_DATA`        | `0x00` | Data frame for packet transmission/reception |
| `CMD_TXDELAY`     | `0x01` | Not used (LoRa has instant TX)               |
| `CMD_P`           | `0x02` | Not used (LoRa uses CAD, not CSMA)           |
| `CMD_SLOTTIME`    | `0x03` | Not used (LoRa uses CAD, not CSMA)           |
| `CMD_TXTAIL`      | `0x04` | Not used (No squelch tail in LoRa)           |
| `CMD_FULLDUPLEX`  | `0x05` | Not used (SX1262 is half-duplex only)        |
| `CMD_SETHARDWARE` | `0x06` | Hardware-specific configuration              |
| `CMD_GETHARDWARE` | `0x07` | Query hardware status                        |
| `CMD_RETURN`      | `0xFF` | Exit KISS mode                               |

#### Data Frames
Data frames contain raw packet data for transmission:

```
FEND + 0x00 + [packet data] + FEND
```

**Example:** Transmit "HELLO" packet
```
C0 00 48 45 4C 4C 4F C0
```

#### Command Frames
Command frames use the format:

```
FEND + Command + Subcommand + [parameters] + FEND
```

## 17.2. SETHARDWARE Command Reference

SETHARDWARE commands configure LoRa radio parameters and system settings.

### Command Format
```
FEND + 0x06 + Subcommand + [parameters] + FEND
```

### Subcommands

#### Radio Configuration
| Subcommand           | Value  | Parameters                   | Description                     |
| -------------------- | ------ | ---------------------------- | ------------------------------- |
| `HW_SET_FREQUENCY`   | `0x01` | 4 bytes (float, MHz)         | Set center frequency            |
| `HW_SET_BANDWIDTH`   | `0x02` | 1 byte                       | Set bandwidth (see table below) |
| `HW_SET_SPREADING`   | `0x03` | 1 byte (7-12)                | Set spreading factor            |
| `HW_SET_CODINGRATE`  | `0x04` | 1 byte (5-8)                 | Set coding rate (4/5 to 4/8)    |
| `HW_SET_POWER`       | `0x05` | 1 byte (dBm)                 | Set transmit power              |
| `HW_SET_SYNCWORD`    | `0x08` | 2 bytes                      | Set sync word (SX126x format)   |
| `HW_SET_GNSS_ENABLE` | `0x09` | 1 byte (0=disable, 1=enable) | Enable/disable GNSS             |

#### System Commands
| Subcommand        | Value  | Parameters | Description                                         |
| ----------------- | ------ | ---------- | --------------------------------------------------- |
| `HW_GET_CONFIG`   | `0x06` | None       | Get current configuration (returns via GETHARDWARE) |
| `HW_SAVE_CONFIG`  | `0x07` | None       | Save configuration to flash memory                  |
| `HW_RESET_CONFIG` | `0xFF` | None       | Reset to factory defaults                           |

### Bandwidth Values
| Value | Bandwidth | Description                      |
| ----- | --------- | -------------------------------- |
| `0`   | 7.8 kHz   | Maximum range, minimum data rate |
| `1`   | 10.4 kHz  | Good range, moderate data rate   |
| `2`   | 15.6 kHz  | Balanced performance             |
| `3`   | 20.8 kHz  | Higher data rate, reduced range  |
| `4`   | 31.25 kHz | Fast transmission, shorter range |
| `5`   | 41.7 kHz  | Very fast, limited range         |
| `6`   | 62.5 kHz  | High speed, urban ranges         |
| `7`   | 125 kHz   | Maximum speed, short range       |
| `8`   | 250 kHz   | Ultra-fast, very short range     |

### Examples

**Set Frequency to 915 MHz:**
```
C0 06 01 42 5C 00 00 C0
```
(0x42 0x5C 0x00 0x00 = 915.0 as IEEE 754 float)

**Set Spreading Factor to 12:**
```
C0 06 03 0C C0
```

**Set Power to 20 dBm:**
```
C0 06 05 14 C0
```

**Save Configuration:**
```
C0 06 07 C0
```

## 17.3. GETHARDWARE Command Reference

GETHARDWARE commands query system status and configuration.

### Command Format
```
FEND + 0x07 + Subcommand + FEND
```

### Subcommands

| Subcommand         | Value  | Returns             | Description                            |
| ------------------ | ------ | ------------------- | -------------------------------------- |
| `HW_QUERY_CONFIG`  | `0x01` | Radio configuration | Query current LoRa settings            |
| `HW_QUERY_BATTERY` | `0x02` | Battery voltage     | Query battery voltage (4 bytes, float) |
| `HW_QUERY_BOARD`   | `0x03` | Board information   | Query board type and name              |
| `HW_QUERY_GNSS`    | `0x04` | GNSS status         | Query GNSS configuration and status    |
| `HW_QUERY_ALL`     | `0xFF` | Everything          | Query all hardware information         |

### Response Format
Responses use the same frame format as commands:

```
FEND + 0x07 + Subcommand + [response data] + FEND
```

### Response Data Formats

#### Configuration Query (HW_QUERY_CONFIG)
Returns 16 bytes:
```
[4 bytes frequency] [1 byte bandwidth] [1 byte SF] [1 byte CR] [1 byte power] [2 bytes syncword] [1 byte GNSS enabled] [4 bytes reserved]
```

#### Battery Query (HW_QUERY_BATTERY)
Returns 4 bytes (IEEE 754 float, volts)

#### Board Query (HW_QUERY_BOARD)
Returns variable length string (board name)

#### GNSS Query (HW_QUERY_GNSS)
Returns 8 bytes:
```
[1 byte enabled] [1 byte satellites] [4 bytes latitude] [2 bytes longitude]
```

### Examples

**Query Configuration:**
```
C0 07 01 C0
```
Response: `C0 07 01 [16 bytes of config data] C0`

**Query Battery Voltage:**
```
C0 07 02 C0
```
Response: `C0 07 02 [4 bytes voltage as float] C0`

## 17.4. REST API Reference

LoRaTNCX provides a comprehensive REST API for configuration and monitoring.

### Base URL
```
http://[device-ip]:80/api/
```

### Endpoints

#### System Status
**GET /api/status**
Returns overall system status.

**Response:**
```json
{
  "uptime": 12345,
  "version": "1.0.0",
  "board": "Heltec WiFi LoRa 32 V4",
  "wifi": {
    "mode": "AP",
    "ssid": "LoRaTNCX-1234",
    "ip": "192.168.4.1"
  },
  "battery": {
    "voltage": 3.85,
    "percentage": 78
  }
}
```

**GET /api/system**
Returns detailed system information.

**Response:**
```json
{
  "firmware": {
    "version": "1.0.0",
    "build_date": "2025-01-01",
    "esp32": {
      "chip_model": "ESP32-S3",
      "chip_revision": 1,
      "cpu_freq": 240,
      "flash_size": 8388608
    }
  },
  "memory": {
    "free_heap": 245760,
    "min_free_heap": 234567,
    "total_psram": 4194304,
    "free_psram": 4194304
  },
  "network": {
    "tcp_clients": 2,
    "max_tcp_clients": 4
  }
}
```

#### LoRa Configuration
**GET /api/lora/config**
Returns current LoRa radio configuration.

**Response:**
```json
{
  "frequency": 915.0,
  "bandwidth": 125.0,
  "spreadingFactor": 12,
  "codingRate": 7,
  "power": 20,
  "syncWord": 5130,
  "preambleLength": 8
}
```

**POST /api/lora/config**
Updates LoRa radio configuration.

**Request Body:**
```json
{
  "frequency": 868.0,
  "bandwidth": 125.0,
  "spreadingFactor": 9,
  "codingRate": 7,
  "power": 17,
  "syncWord": 29099
}
```

**Response:**
```json
{
  "success": true,
  "message": "Configuration updated",
  "reboot_required": false
}
```

#### WiFi Configuration
**GET /api/wifi/config**
Returns current WiFi configuration.

**Response:**
```json
{
  "mode": "AP",
  "ap": {
    "ssid": "LoRaTNCX-1234",
    "password": "loratncx",
    "channel": 1,
    "hidden": false
  },
  "sta": {
    "ssid": "",
    "password": "",
    "ip": "",
    "gateway": "",
    "subnet": ""
  }
}
```

**POST /api/wifi/config**
Updates WiFi configuration.

**Request Body:**
```json
{
  "mode": "STA",
  "sta": {
    "ssid": "MyWiFi",
    "password": "mypassword"
  }
}
```

#### GNSS Configuration
**GET /api/gnss/config**
Returns GNSS configuration.

**Response:**
```json
{
  "enabled": true,
  "tcp_port": 10110,
  "baud_rate": 9600
}
```

**POST /api/gnss/config**
Updates GNSS configuration.

**Request Body:**
```json
{
  "enabled": true,
  "tcp_port": 10110
}
```

**GET /api/gnss/status**
Returns GNSS status and position information.

**Response:**
```json
{
  "enabled": true,
  "fix": true,
  "satellites": 8,
  "latitude": 42.3601,
  "longitude": -71.0589,
  "altitude": 10.5,
  "hdop": 1.2,
  "speed": 0.0,
  "course": 0.0,
  "last_update": 1234567890
}
```

### HTTP Status Codes

| Code | Meaning                          |
| ---- | -------------------------------- |
| 200  | Success                          |
| 400  | Bad Request (invalid parameters) |
| 404  | Not Found                        |
| 500  | Internal Server Error            |

### Authentication
Currently, no authentication is required. All endpoints are publicly accessible.

## 17.5. Pin Configurations

### Heltec WiFi LoRa 32 V3

| Function           | GPIO | Notes                            |
| ------------------ | ---- | -------------------------------- |
| **LoRa Radio**     |      |                                  |
| SCLK               | 9    | SPI clock                        |
| MISO               | 11   | SPI master in slave out          |
| MOSI               | 10   | SPI master out slave in          |
| CS                 | 8    | SPI chip select                  |
| DIO0               | 14   | Radio interrupt 0                |
| RST                | 12   | Radio reset                      |
| DIO1               | 14   | Radio interrupt 1 (same as DIO0) |
| BUSY               | 13   | Radio busy indicator             |
| **Display**        |      |                                  |
| SDA                | 17   | I2C data (OLED)                  |
| SCL                | 18   | I2C clock (OLED)                 |
| RST                | 21   | OLED reset                       |
| **Battery**        |      |                                  |
| ADC                | 1    | Battery voltage measurement      |
| CTRL               | 37   | Battery measurement control      |
| **User Interface** |      |                                  |
| BUTTON             | 0    | User button (boot button)        |
| **GNSS**           |      | Not available (external only)    |

### Heltec WiFi LoRa 32 V4

| Function            | GPIO | Notes                             |
| ------------------- | ---- | --------------------------------- |
| **LoRa Radio**      |      |                                   |
| SCLK                | 9    | SPI clock                         |
| MISO                | 11   | SPI master in slave out           |
| MOSI                | 10   | SPI master out slave in           |
| CS                  | 8    | SPI chip select                   |
| DIO0                | 14   | Radio interrupt 0                 |
| RST                 | 12   | Radio reset                       |
| DIO1                | 14   | Radio interrupt 1 (same as DIO0)  |
| BUSY                | 13   | Radio busy indicator              |
| **Power Amplifier** |      |                                   |
| PA_EN               | 2    | PA enable                         |
| PA_TX_EN            | 46   | PA transmit enable                |
| PA_POWER            | 7    | PA power control                  |
| **Display**         |      |                                   |
| SDA                 | 17   | I2C data (OLED)                   |
| SCL                 | 18   | I2C clock (OLED)                  |
| RST                 | 21   | OLED reset                        |
| **Battery**         |      |                                   |
| ADC                 | 1    | Battery voltage measurement       |
| CTRL                | 37   | Battery measurement control       |
| **GNSS**            |      |                                   |
| RX                  | 39   | GNSS TX → MCU RX                  |
| TX                  | 38   | MCU TX → GNSS RX                  |
| CTRL                | 34   | GNSS power control (LOW = enable) |
| WAKE                | 40   | GNSS wake signal                  |
| PPS                 | 41   | GNSS pulse per second             |
| RST                 | 42   | GNSS reset                        |
| **User Interface**  |      |                                   |
| BUTTON              | 0    | User button (boot button)         |

### Arduino Pin Mapping

The firmware uses Arduino pin numbers, which map to ESP32 GPIO numbers as follows:

| Arduino Pin | ESP32 GPIO | Function          |
| ----------- | ---------- | ----------------- |
| 0           | GPIO0      | User button       |
| 1           | GPIO1      | Battery ADC       |
| 2           | GPIO2      | PA enable (V4)    |
| 7           | GPIO7      | PA power (V4)     |
| 8           | GPIO8      | LoRa CS           |
| 9           | GPIO9      | LoRa SCLK         |
| 10          | GPIO10     | LoRa MOSI         |
| 11          | GPIO11     | LoRa MISO         |
| 12          | GPIO12     | LoRa RST          |
| 13          | GPIO13     | LoRa BUSY         |
| 14          | GPIO14     | LoRa DIO0/DIO1    |
| 17          | GPIO17     | OLED SDA          |
| 18          | GPIO18     | OLED SCL          |
| 21          | GPIO21     | OLED RST          |
| 34          | GPIO34     | GNSS CTRL (V4)    |
| 37          | GPIO37     | Battery CTRL      |
| 38          | GPIO38     | GNSS TX (V4)      |
| 39          | GPIO39     | GNSS RX (V4)      |
| 40          | GPIO40     | GNSS WAKE (V4)    |
| 41          | GPIO41     | GNSS PPS (V4)     |
| 42          | GPIO42     | GNSS RST (V4)     |
| 46          | GPIO46     | PA TX enable (V4) |

## 17.6. Factory Test Procedures

### Hardware Test Requirements

**Equipment Needed:**
- Heltec WiFi LoRa 32 V3 or V4 board
- USB cable
- Two SMA antennas (for range testing)
- GNSS antenna (V4 only)
- Multimeter (for voltage testing)
- Oscilloscope (optional, for signal analysis)

**Software Requirements:**
- PlatformIO IDE or Core
- Python 3.7+ with pyserial
- Terminal emulator (minicom, screen, or PuTTY)

### Basic Functionality Tests

#### 1. Power and Boot Test
```bash
# Connect board via USB
# Monitor serial output
platformio device monitor --environment heltec_wifi_lora_32_V4

# Expected output:
# === LoRaTNCX Starting ===
# Initializing SPIFFS...
# Board: Heltec WiFi LoRa 32 V4
# Radio initialized successfully
# WiFi AP mode started: LoRaTNCX-XXXX
# Web server started on port 80
# Ready for KISS connections
```

#### 2. OLED Display Test
- Verify display powers on and shows LoRaTNCX splash screen
- Check that status information updates correctly
- Test button functionality (if available)

#### 3. Battery Voltage Test
```bash
# Query battery voltage
python3 tools/loratncx_config.py /dev/ttyACM0 --get-battery

# Expected: Battery Voltage: X.XXV (XX% estimated)
# Verify voltage is reasonable (3.0-4.2V range)
```

#### 4. WiFi Functionality Test
```bash
# Scan for WiFi networks
nmcli device wifi list | grep LoRaTNCX

# Connect to AP
nmcli device wifi connect LoRaTNCX-XXXX password loratncx

# Test web interface
curl http://192.168.4.1/api/status
```

### Radio Functionality Tests

#### 5. LoRa Configuration Test
```bash
# Test configuration commands
python3 tools/loratncx_config.py /dev/ttyACM0 --get-config

# Test parameter changes
python3 tools/loratncx_config.py /dev/ttyACM0 --frequency 915.0
python3 tools/loratncx_config.py /dev/ttyACM0 --power 10
python3 tools/loratncx_config.py /dev/ttyACM0 --save
```

#### 6. KISS Protocol Test
```bash
# Test basic KISS connectivity
python3 test/kiss_test.py --port /dev/ttyACM0 --test-basic

# Test parameter validation
python3 test/kiss_test.py --port /dev/ttyACM0 --test-config
```

#### 7. Range and Performance Test
**Setup:**
1. Connect two LoRaTNCX devices
2. Configure identical parameters (frequency, SF, BW, CR)
3. Place devices 10-100 meters apart (line of sight)

**Test Commands:**
```bash
# Device 1: Send test packets
python3 test/kiss_test.py --port /dev/ttyACM0 --send-test

# Device 2: Receive and verify
python3 test/kiss_test.py --port /dev/ttyACM1 --receive-test

# Expected: 100% packet success rate at close range
# Expected: >90% success rate at maximum range
```

### GNSS Test (V4 Only)

#### 8. GNSS Functionality Test
```bash
# Enable GNSS
python3 tools/loratncx_config.py /dev/ttyACM0 --gnss-enable true

# Wait 2-3 minutes for satellite acquisition
# Check GNSS status
python3 tools/loratncx_config.py /dev/ttyACM0 --get-gnss

# Test NMEA output
telnet 192.168.4.1 10110
# Should see NMEA sentences like:
# $GPGGA,123456.00,4236.0100,N,07105.8900,W,1,08,1.2,10.5,M,,,,*47
```

### Comprehensive Test Suite

#### Automated Test Script
```bash
#!/bin/bash
# comprehensive_test.sh

echo "=== LoRaTNCX Comprehensive Factory Test ==="

# Test 1: Basic connectivity
echo "Test 1: Basic connectivity..."
platformio run --target upload --environment heltec_wifi_lora_32_V4
sleep 5
if platformio device monitor --environment heltec_wifi_lora_32_V4 --quiet --exit-char 'q' | grep -q "Ready for KISS"; then
    echo "✓ Basic connectivity test passed"
else
    echo "✗ Basic connectivity test failed"
    exit 1
fi

# Test 2: Configuration
echo "Test 2: Configuration test..."
python3 tools/loratncx_config.py /dev/ttyACM0 --get-config > /dev/null
if [ $? -eq 0 ]; then
    echo "✓ Configuration test passed"
else
    echo "✗ Configuration test failed"
    exit 1
fi

# Test 3: KISS protocol
echo "Test 3: KISS protocol test..."
python3 test/kiss_test.py --port /dev/ttyACM0 --test-basic > /dev/null
if [ $? -eq 0 ]; then
    echo "✓ KISS protocol test passed"
else
    echo "✗ KISS protocol test failed"
    exit 1
fi

# Test 4: Web interface
echo "Test 4: Web interface test..."
curl -s http://192.168.4.1/api/status | jq .version > /dev/null
if [ $? -eq 0 ]; then
    echo "✓ Web interface test passed"
else
    echo "✗ Web interface test failed"
    exit 1
fi

echo "=== All tests passed! ==="
```

### Test Result Documentation

**Test Report Template:**
```
LoRaTNCX Factory Test Report
============================

Board Information:
- Board Type: Heltec WiFi LoRa 32 V4
- Serial Number: [board serial]
- Firmware Version: [version]
- Test Date: [date]

Test Results:
- [ ] Power and boot test
- [ ] OLED display test  
- [ ] Battery voltage test
- [ ] WiFi functionality test
- [ ] LoRa configuration test
- [ ] KISS protocol test
- [ ] Range and performance test
- [ ] GNSS functionality test (V4 only)

Performance Metrics:
- Transmit Power: [dBm]
- Receive Sensitivity: [dBm]
- Range Test Distance: [meters]
- Packet Success Rate: [%]
- Battery Life: [hours]

Issues Found:
- [List any issues or deviations from expected behavior]

Tested By: [Name]
Approved: [ ] Yes [ ] No
```

## 17.7. Documentation Changelog

### Version 1.0.0 - November 2025
- Initial release of LoRaTNCX User Manual (AI generated)

## 17.8. License and Credits

### License

Copyright (c) 2025 S. Miller KC1AWV

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### Credits and Acknowledgments

**Project Author:**
- S. Miller KC1AWV - Primary human oversight and maintainer

**Hardware:**
- Heltec Automation(TM) - WiFi LoRa 32 V3/V4 boards
- Semtech - SX1262 LoRa radio chipset

**Software Libraries:**
- **RadioLib** (v7.4.0) - LoRa radio control and modulation
- **U8g2** (v2.35.30) - OLED display driver
- **ESPAsyncWebServer** (v3.6.0) - Asynchronous HTTP server
- **ArduinoJson** (v7.0.0) - JSON parsing and generation
- **TinyGPSPlus** (v1.1.0) - NMEA sentence parsing

**Development Tools:**
- **PlatformIO** - Build system and IDE integration
- **Arduino Framework** - ESP32 development framework
- **FreeRTOS** - Real-time operating system

**Testing and Validation:**
- Python serial communication libraries
- KISS protocol test frameworks
- Amateur radio community testing

**Documentation:**
- Comprehensive user manual with 17 detailed sections
- API reference and protocol documentation
- Factory test procedures and troubleshooting guides

### Contributing

LoRaTNCX is an open-source project released under the MIT License. Contributions are welcome and encouraged. Please see Section 16 (Development and Customization) for detailed contribution guidelines.

**Ways to Contribute:**
- Bug reports and feature requests
- Code contributions and improvements
- Documentation updates and translations
- Hardware testing and validation
- Community support and tutorials

### Disclaimer

This software is provided for experimental and educational purposes in the Amateur Radio service. Users are responsible for compliance with all applicable laws, regulations, and licensing requirements in their jurisdiction.

**Important Safety Notice:**
- Amateur Radio operation requires appropriate licensing
- LoRaTNCX is not certified for commercial or critical applications
- Users assume all responsibility for proper operation and compliance

---

**This concludes the LoRaTNCX User Manual.** | **Main Index:** [User Manual](../user_manual.md)