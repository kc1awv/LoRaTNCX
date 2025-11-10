# 5. Installation and Setup

This section provides detailed instructions for setting up your development environment and building/uploading LoRaTNCX firmware to your Heltec board.

## 5.1. PlatformIO Environment Setup

### System Requirements

**Operating Systems:**
- Windows 10/11
- macOS 10.12 or later
- Linux (Ubuntu 18.04+, CentOS 7+, etc.)

**Hardware Requirements:**
- 2GB RAM minimum (4GB recommended)
- 1GB free disk space
- USB port for board connection

**Software Prerequisites:**
- Python 3.6 or later
- Git
- USB-to-serial drivers (usually automatic on Linux/macOS)

### Installation Methods

#### Method 1: VS Code with PlatformIO Extension (Recommended)

1. **Install Visual Studio Code**
   ```bash
   # Windows: Download from https://code.visualstudio.com/
   # macOS: brew install --cask visual-studio-code
   # Linux: sudo snap install code --classic
   ```

2. **Install PlatformIO Extension**
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Install and reload VS Code

3. **Clone the Repository**
   ```bash
   git clone https://github.com/kc1awv/LoRaTNCX.git
   cd LoRaTNCX
   ```

4. **Open in VS Code**
   - File → Open Folder → Select LoRaTNCX directory
   - PlatformIO should automatically detect the project

#### Method 2: PlatformIO Core (Command Line)

1. **Install PlatformIO Core**
   ```bash
   # Using pip (recommended)
   pip install platformio

   # Or using installer script
   curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py | python
   ```

2. **Clone Repository**
   ```bash
   git clone https://github.com/kc1awv/LoRaTNCX.git
   cd LoRaTNCX
   ```

3. **Verify Installation**
   ```bash
   pio --version
   # Should show: PlatformIO Core, version x.x.x
   ```

### Project Structure

After setup, your project should have this structure:
```
LoRaTNCX/
├── platformio.ini          # PlatformIO configuration
├── src/                    # Source code
│   ├── main.cpp
│   ├── kiss.cpp
│   └── ...
├── include/                # Header files
├── lib/                    # Local libraries (if any)
├── data/                   # SPIFFS web interface files
├── test/                   # Test files
├── boards/                 # Custom board definitions
├── variants/               # Board-specific pin configurations
└── .vscode/                # VS Code configuration
```

### Initial Configuration

1. **Select Target Board**
   - The project supports two environments:
     - `heltec_wifi_lora_32_V3` for Heltec WiFi LoRa 32 V3
     - `heltec_wifi_lora_32_V4` for Heltec WiFi LoRa 32 V4

2. **Verify Dependencies**
   ```bash
   pio lib list
   # Should show installed libraries:
   # RadioLib, U8g2, ESPAsyncWebServer, ArduinoJson, TinyGPSPlus
   ```

## 5.2. Building the Firmware

### Build Process Overview

The build process:
1. Compiles C/C++ source files
2. Links with required libraries
3. Generates firmware binary
4. Prepares for upload

### Building for V3 Board

**Using VS Code:**
1. Open PlatformIO sidebar (alien icon)
2. Select "heltec_wifi_lora_32_V3" environment
3. Click the "Build" button (✓)

**Using Command Line:**
```bash
# Build firmware
pio run --environment heltec_wifi_lora_32_V3

# Expected output:
# Processing heltec_wifi_lora_32_V3 (board: heltec_wifi_lora_32_V3; framework: arduino)
# ----------------------------------------------------------------------------------
# Verbose mode can be enabled via `-v, --verbose` option
# CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/heltec_wifi_lora_32_V3.html
# PLATFORM: Espressif 32 (6.4.0) > Heltec WiFi LoRa 32 (V3)
# ...
# Building in release mode
# ...
# Firmware size: 1.2 MB
# ============== [SUCCESS] Took 45.67 seconds ==============
```

### Building for V4 Board

**Using VS Code:**
1. Open PlatformIO sidebar
2. Select "heltec_wifi_lora_32_V4" environment
3. Click the "Build" button

**Using Command Line:**
```bash
# Build firmware
pio run --environment heltec_wifi_lora_32_V4
```

### Build Configuration

The `platformio.ini` file defines build settings:

```ini
[env:heltec_wifi_lora_32_V3]
board = heltec_wifi_lora_32_V3
board_build.filesystem = spiffs
build_flags =
    -DARDUINO_HELTEC_WIFI_LORA_32_V3
```

**Key Build Flags:**
- `-DARDUINO_HELTEC_WIFI_LORA_32_V3/V4`: Board identification
- Watchdog timer configuration for stability
- SPIFFS filesystem support

### Troubleshooting Builds

**Common Issues:**

1. **Missing Dependencies**
   ```bash
   pio lib install  # Install all required libraries
   ```

2. **Board Not Recognized**
   ```bash
   pio boards espressif32 | grep heltec
   # Should list available Heltec boards
   ```

3. **Compilation Errors**
   - Check ESP32 board support in PlatformIO
   - Update PlatformIO: `pio upgrade`
   - Clean build: `pio run --target clean`

4. **Memory Issues**
   - Firmware size should be under 8MB
   - Check build output for memory usage warnings

## 5.3. Uploading Firmware

### Upload Requirements

**Hardware Setup:**
- Connect board via USB-C cable
- Ensure board is in bootloader mode (automatic)
- No antenna needed for upload

**Serial Port Detection:**
```bash
# Linux/macOS
ls /dev/tty* | grep -E "(USB|ACM)"

# Windows
# Check Device Manager for COM ports
```

### Uploading to V3 Board

**Using VS Code:**
1. Select "heltec_wifi_lora_32_V3" environment
2. Click "Upload" button (→)

**Using Command Line:**
```bash
# Upload firmware
pio run --target upload --environment heltec_wifi_lora_32_V3

# Or specify serial port
pio run --target upload --environment heltec_wifi_lora_32_V3 --upload-port /dev/ttyACM0
```

### Uploading to V4 Board

**Using VS Code:**
1. Select "heltec_wifi_lora_32_V4" environment
2. Click "Upload" button

**Using Command Line:**
```bash
# Upload firmware
pio run --target upload --environment heltec_wifi_lora_32_V4
```

### Upload Process Details

1. **Bootloader Mode**: PlatformIO automatically resets board into bootloader
2. **Flash Programming**: ESP32 flash memory is programmed
3. **Verification**: Firmware integrity is checked after upload
4. **Reset**: Board automatically reboots with new firmware

### Upload Troubleshooting

**Common Issues:**

1. **Port Not Found**
   ```bash
   # List available ports
   pio device list

   # Manual port specification
   pio run --target upload --upload-port /dev/ttyUSB0
   ```

2. **Permission Denied (Linux)**
   ```bash
   # Add user to dialout group
   sudo usermod -a -G dialout $USER
   # Logout and login again
   ```

3. **Upload Fails**
   - Check USB cable (data cable, not charge-only)
   - Try different USB port
   - Reset board manually (press and release RST button)

4. **Bootloader Issues**
   - Hold BOOT button while plugging in USB
   - Or press RST while holding BOOT

## 5.4. Uploading Filesystem (SPIFFS)

### SPIFFS Overview

SPIFFS (SPI Flash File System) stores the web interface files:
- `index.html` - Main web interface
- CSS and JavaScript files
- Static assets (fonts, icons)

### Upload Process

**Important:** Upload firmware first, then filesystem.

**Using VS Code:**
1. Select environment (V3 or V4)
2. Run "Upload Filesystem Image" task

**Using Command Line:**

For V3:
```bash
pio run --target uploadfs --environment heltec_wifi_lora_32_V3
```

For V4:
```bash
pio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

### Filesystem Contents

The `data/` directory contains:
```
data/
├── index.html          # Main web interface
├── style.css           # Styling
├── app.js             # JavaScript functionality
├── popper.min.js      # UI library
├── tippy-bundle.umd.js # Tooltip library
├── tippy-light-theme.css # Tooltip styling
└── webfonts/          # Font Awesome icons
    ├── fa-solid-900.woff2
    └── ...
```

### SPIFFS Operations

**File Management:**
- Files are read-only at runtime
- Upload overwrites entire filesystem
- No file modification after upload

**Size Limits:**
- Total filesystem size: ~1.5MB (depends on partition)
- Individual files: Limited by available space
- Compression: Files are stored uncompressed

### Troubleshooting SPIFFS Upload

1. **Upload Order**: Always upload firmware before filesystem
2. **Space Issues**: Check file sizes in data/ directory
3. **Corruption**: Re-upload if web interface doesn't load
4. **Verification**: Web interface should be accessible after upload

## 5.5. Factory Reset and Erase Flash

### When to Erase

**Scenarios requiring flash erase:**
- Corrupted configuration
- Testing with clean state
- Preparing for new firmware version

### Erase Process

**Using VS Code:**
1. Select environment
2. Run "Erase Flash" task

**Using Command Line:**

For V3:
```bash
pio run --target erase --environment heltec_wifi_lora_32_V3
```

For V4:
```bash
pio run --target erase --environment heltec_wifi_lora_32_V4
```

### What Gets Erased

**Complete Flash Erase:**
- All firmware code
- SPIFFS filesystem (web interface)
- NVS configuration storage
- WiFi credentials
- LoRa radio settings
- All user data

**Result:** Board returns to factory state, requires full setup.

### Post-Erase Setup

After erase:
1. **Upload Firmware** (Section 5.3)
2. **Upload Filesystem** (Section 5.4)
3. **Configure Device** (Section 6)
4. **Test Operation** (Section 13)

### Partial Reset Options

**Configuration Reset Only:**
- Use web interface "Factory Reset" button
- Or send KISS command: `0xC0 0x06 0xFF 0xC0`
- Preserves firmware and filesystem

**WiFi Reset:**
- Web interface: WiFi → "Clear Settings"
- Removes saved WiFi credentials only

### Safety Precautions

⚠️ **Backup Important Data:**
- Document your LoRa configuration
- Save WiFi network settings
- Note any custom configurations

⚠️ **Complete Process:**
- Erase → Upload Firmware → Upload Filesystem → Configure
- Skipping steps may result in non-functional device

---

[← Previous: Software Architecture](04-software-architecture.md) | [Back to Main Manual](../user_manual.md) | [Next: Configuration →](06-configuration.md)