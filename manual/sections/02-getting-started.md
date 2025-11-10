# 2. Getting Started

This section will guide you through your first experience with LoRaTNCX, from unboxing to sending your first packet.

## 2.1. Unboxing and Initial Setup

### What You'll Need

**Required Hardware:**
- Heltec WiFi LoRa 32 V3 or V4 board
- USB-C cable (for programming and power)
- Antenna suitable for your operating frequency (433MHz, 868MHz, 915MHz, or 928MHz)
- Computer with USB port

**Optional Hardware:**
- GNSS module (V4 boards only) for GPS functionality
- External power source (battery or DC supply) for field operation

**Software Prerequisites:**
- Python 3.6+ (for configuration tool)
- PlatformIO IDE or VS Code with PlatformIO extension
- Git (for cloning the repository)

### Board Identification

LoRaTNCX supports two board variants:

**Heltec WiFi LoRa 32 V3:**
- ESP32-S3 microcontroller
- SX1262 LoRa radio
- No external power amplifier
- OLED display
- No GNSS connector

**Heltec WiFi LoRa 32 V4:**
- ESP32-S3 microcontroller
- SX1262 LoRa radio with external PA
- GNSS module connector
- OLED display
- Enhanced power management

### Initial Hardware Setup

1. **Attach Antenna**: Connect a suitable LoRa antenna to the antenna connector on your board
2. **Optional GNSS**: For V4 boards, attach the Heltec GNSS module if desired
3. **Power Connection**: Connect via USB-C cable to your computer

⚠️ **Important**: Never transmit without a proper antenna attached. This can damage the radio module.

## 2.2. Firmware Installation

### Step 1: Clone the Repository

```bash
git clone https://github.com/kc1awv/LoRaTNCX.git
cd LoRaTNCX
```

### Step 2: Install PlatformIO

**Using VS Code (Recommended):**
1. Install VS Code
2. Install the PlatformIO IDE extension
3. Open the LoRaTNCX folder in VS Code

**Command Line:**
```bash
# Install PlatformIO Core
pip install platformio

# Or using the installer script
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py | python
```

### Step 3: Build and Upload Firmware

**For Heltec WiFi LoRa 32 V3:**
```bash
# Build
platformio run --environment heltec_wifi_lora_32_V3

# Upload
platformio run --target upload --environment heltec_wifi_lora_32_V3
```

**For Heltec WiFi LoRa 32 V4:**
```bash
# Build
platformio run --environment heltec_wifi_lora_32_V4

# Upload
platformio run --target upload --environment heltec_wifi_lora_32_V4
```

**Using VS Code Tasks:**
- Open Command Palette (Ctrl+Shift+P)
- Select "Tasks: Run Task"
- Choose "Upload (heltec_wifi_lora_32_V3)" or "Upload (heltec_wifi_lora_32_V4)"

### Step 4: Upload Web Interface Files

The web interface files must be uploaded separately to the board's SPIFFS filesystem:

```bash
# For V3
platformio run --target uploadfs --environment heltec_wifi_lora_32_V3

# For V4
platformio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

**VS Code Task:** "Upload Filesystem (SPIFFS) V3/V4"

## 2.3. First Power-On

### Default Behavior

After uploading firmware, the TNC will:

1. **Boot up** and show startup messages on the OLED display
2. **Create a WiFi Access Point** named `LoRaTNCX-XXXX` (where XXXX is a unique ID)
3. **Set default LoRa parameters**:
   - Frequency: 915.0 MHz
   - Bandwidth: 125 kHz
   - Spreading Factor: 12
   - Coding Rate: 4/7
   - Output Power: 20 dBm
   - Sync Word: 0x1424

### Connecting to the Web Interface

1. **Find the WiFi Network**: Look for `LoRaTNCX-XXXX` in your WiFi networks
2. **Connect**: Password is `loratncx`
3. **Open Browser**: Navigate to `http://192.168.4.1`
4. **Change Default Password**: Immediately change the WiFi password for security

### OLED Display Status

The OLED display shows:
- LoRaTNCX startup screen
- Radio configuration summary
- Current WiFi mode and status
- Battery voltage (if connected)
- GNSS status (if applicable)
- Blank screen (OLED power off mode)

### Serial Monitor

Connect to the serial port to see detailed boot messages:

```bash
# Find your serial port
ls /dev/tty* | grep -E "(USB|ACM)"

# Monitor (adjust port as needed)
platformio device monitor --environment heltec_wifi_lora_32_V4
```

## 2.4. Basic Configuration

### Using the Web Interface

1. **Access the web interface** at `http://192.168.4.1`
2. **Navigate to LoRa Settings**
3. **Configure basic parameters**:
   - Set your operating frequency
   - Adjust bandwidth and spreading factor for your use case
   - Set appropriate output power (start low, increase as needed)

### Using the Command-Line Tool

The `loratncx_config.py` tool provides command-line configuration:

```bash
# Make executable
chmod +x tools/loratncx_config.py

# Get current configuration
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config

# Set frequency (example: 433.775 MHz for European ISM)
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 433.775

# Set spreading factor for longer range
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 12

# Save configuration
python3 tools/loratncx_config.py /dev/ttyUSB0 --save
```

### Testing Basic Functionality

**Quick Test with Echo:**
1. Connect to serial port with a terminal program (minicom, screen, PuTTY)
2. Send a KISS frame (you'll need KISS-aware software for this)
3. Or use the test script: `python3 test/kiss_test.py /dev/ttyUSB0`

**Range Test:**
1. Set up two TNCs with identical configuration
2. Use the test script for bidirectional testing
3. Start with short range, gradually increase distance

### Next Steps

Once basic configuration is complete:
- [Configure WiFi settings](08-wifi-networking.md) for your network
- [Set up your KISS application](12-applications-integration.md) (Dire Wolf, etc.)
- [Test with real packet radio software](13-testing-validation.md)

---

[← Previous: Introduction](01-introduction.md) | [Back to Main Manual](../user_manual.md) | [Next: Hardware Overview →](03-hardware-overview.md)