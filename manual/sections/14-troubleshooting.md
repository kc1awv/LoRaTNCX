# 14. Troubleshooting

## Overview

This section provides comprehensive troubleshooting guidance for common LoRaTNCX issues. Problems are organized by category with step-by-step diagnostic procedures and solutions. Most issues can be resolved through systematic troubleshooting.

## Common Issues

### Device Won't Power On

**Symptoms:**
- No LED activity
- Device doesn't respond to serial commands
- Web interface not accessible

**Possible Causes & Solutions:**

**1. Power Supply Issues:**
- **Low Voltage:** Ensure input voltage is 5V (USB) or 3.3-5V (external)
- **Current Limiting:** LoRa transmission requires 150-300mA peaks
- **Cable Quality:** Use short, thick USB cables for reliable power delivery

**2. Hardware Damage:**
- **ESD Damage:** Handle boards carefully, use anti-static precautions
- **Overvoltage:** Never apply more than 5.5V to any pin
- **Short Circuit:** Check for solder bridges or component damage

**3. Firmware Corruption:**
- **Corrupt Flash:** Perform factory reset and re-flash firmware
- **Bootloader Issues:** Try different USB ports or cables
- **Watchdog Reset:** May indicate power supply instability

**Diagnostic Steps:**
```bash
# Check if device enumerates on USB
lsusb | grep -i esp32

# Monitor serial output during boot
screen /dev/ttyACM0 115200

# Check power consumption (if multimeter available)
# Should draw ~80mA in receive mode, ~150mA during TX
```

### No Serial Communication

**Symptoms:**
- Device powers on but no serial response
- Terminal programs can't connect
- Command-line tools fail with "port not found"

**Serial Port Issues:**

**1. Permission Problems:**
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
# Logout and login again

# Or run commands with sudo
sudo python3 tools/loratncx_config.py /dev/ttyACM0 --get-config
```

**2. Port Detection:**
```bash
# List available serial ports
ls -l /dev/tty{USB,ACM,S}*

# Check system messages for device detection
dmesg | tail -20 | grep tty

# Try different USB ports/cables
```

**3. Driver Issues:**
- **Linux:** Usually works out-of-the-box with cdc_acm driver
- **Windows:** Install CP210x USB-to-UART drivers from Silicon Labs
- **macOS:** Usually works out-of-the-box

**4. Port Conflicts:**
```bash
# Check if port is already in use
sudo lsof /dev/ttyACM0

# Kill conflicting processes
sudo fuser -k /dev/ttyACM0

# Reset USB subsystem (Linux)
sudo modprobe -r cdc_acm
sudo modprobe cdc_acm
```

### OLED Display Problems

**Symptoms:**
- Blank display
- Garbled text
- Display doesn't update

**Display Issues:**

**1. Connection Problems:**
- **I2C Bus Issues:** Check SDA/SCL pin connections
- **Power Supply:** OLED requires stable 3.3V supply
- **Address Conflict:** Ensure no other I2C devices use 0x3C

**2. Firmware Issues:**
- **U8g2 Library:** Verify correct pin definitions in variants
- **Initialization Failure:** Check serial logs for OLED errors
- **Memory Issues:** May fail if ESP32 RAM is heavily used

**3. Hardware Problems:**
- **Defective Display:** Try known working display
- **Cable Damage:** Check ribbon cable connections
- **ESD Damage:** Handle display carefully

**Diagnostic Commands:**
```bash
# Check I2C bus (requires i2c-tools)
sudo apt install i2c-tools
sudo i2cdetect -y 1  # Check I2C bus 1

# Monitor serial output for OLED errors
screen /dev/ttyACM0 115200
```

## Firmware Update Problems

### Upload Fails

**Symptoms:**
- PlatformIO build succeeds but upload fails
- "Failed to connect" or timeout errors
- Device not found during upload

**Upload Issues:**

**1. Bootloader Mode:**
- **Manual Boot:** Hold BOOT button while plugging in USB
- **Auto Reset:** Ensure RTS/DTR signals work (some USB-serial adapters lack this)
- **Timing Issues:** Try slower upload speeds

**2. Serial Connection:**
```bash
# Test basic serial connectivity
screen /dev/ttyACM0 115200
# Should see bootloader messages or firmware output

# Check upload port
pio run --target upload --upload-port /dev/ttyACM0 --verbose
```

**3. PlatformIO Configuration:**
- **Board Selection:** Verify correct board (V3 vs V4) in platformio.ini
- **Environment:** Use `heltec_wifi_lora_32_V3` or `heltec_wifi_lora_32_V4`
- **Dependencies:** Ensure all libraries are installed

**4. USB Issues:**
- **Power Delivery:** Some USB ports don't provide enough current
- **Cable Quality:** Use USB 2.0 cables, avoid USB 3.0 ports
- **Hub Issues:** Connect directly to computer, avoid USB hubs

### Build Errors

**Symptoms:**
- Compilation fails with errors
- Library dependency issues
- PlatformIO environment problems

**Build Problems:**

**1. Library Issues:**
```bash
# Clean and rebuild dependencies
pio lib update
pio run --clean

# Check library versions in platformio.ini
# RadioLib@^7.4.0
# U8g2@^2.35.30
# ESPAsyncWebServer@^3.6.0
```

**2. PlatformIO Setup:**
```bash
# Update PlatformIO
pio upgrade

# Check Python version (should be 3.6+)
python3 --version

# Reinstall PlatformIO (if issues persist)
pip uninstall platformio
pip install platformio
```

**3. Code Issues:**
- **Variant Files:** Ensure correct pins defined in variants/ directory
- **Build Flags:** Check platformio.ini for correct board-specific flags
- **RAM Issues:** Reduce features if getting "out of memory" errors

### Filesystem Upload Issues

**Symptoms:**
- Firmware uploads but web interface shows errors
- SPIFFS filesystem not accessible
- Configuration not persisting

**SPIFFS Issues:**

**1. Upload Process:**
```bash
# Upload filesystem separately
pio run --target uploadfs --environment heltec_wifi_lora_32_V4

# Check filesystem size
pio run --environment heltec_wifi_lora_32_V4 | grep "SPIFFS"
```

**2. Filesystem Corruption:**
- **Factory Reset:** Erase flash and re-upload both firmware and filesystem
- **Size Mismatch:** Ensure data/ directory contents fit in SPIFFS partition
- **Upload Timing:** Wait for filesystem upload to complete (can take time)

**3. Recovery:**
```bash
# Erase entire flash
pio run --target erase --environment heltec_wifi_lora_32_V4

# Re-upload firmware
pio run --target upload --environment heltec_wifi_lora_32_V4

# Re-upload filesystem
pio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

## Communication Failures

### No LoRa Transmission

**Symptoms:**
- LED doesn't flash during transmission attempts
- Other devices don't receive packets
- Serial monitor shows no TX activity

**Transmission Issues:**

**1. Configuration Mismatch:**
```bash
# Verify LoRa parameters match between devices
python3 tools/loratncx_config.py /dev/ttyACM0 --get-config
python3 tools/loratncx_config.py /dev/ttyACM1 --get-config

# Check: Frequency, Bandwidth, SF, CR, Sync Word must match
```

**2. Power Amplifier (V4):**
- **PA Control:** Ensure PA is enabled for transmission
- **Power Setting:** Check output power (10-20dBm typical)
- **Antenna Connection:** Verify antenna is connected and matched

**3. Hardware Issues:**
- **SX1262 Module:** May be damaged or not initialized
- **SPI Bus:** Check SPI pin connections
- **Clock Issues:** Verify crystal oscillator is working

**4. Duty Cycle Limits:**
- **Regulatory Limits:** Check local regulations (ETSI, FCC)
- **Thermal Issues:** High power transmission may cause overheating

### No LoRa Reception

**Symptoms:**
- Transmissions work but no packets received
- RSSI readings are very low or negative
- High packet loss rate

**Reception Issues:**

**1. Sensitivity Problems:**
- **Antenna Issues:** Check antenna connection and orientation
- **Frequency Offset:** Recalibrate frequency if using non-standard crystals
- **Interference:** Scan for interfering signals on frequency

**2. Configuration Issues:**
- **Bandwidth Mismatch:** Ensure RX bandwidth matches TX
- **SF/Rate Mismatch:** Coding rate must match between devices
- **Sync Word:** Must be identical for communication

**3. Range Issues:**
- **Path Loss:** Calculate free space path loss
- **Obstructions:** Line-of-sight required for long range
- **Multipath Fading:** Avoid reflective surfaces near antennas

**4. Hardware Problems:**
- **LNA Issues:** Low noise amplifier may be damaged
- **Filter Problems:** RX filters may be misaligned
- **AGC Issues:** Automatic gain control malfunction

### Packet Corruption

**Symptoms:**
- Packets received but data is garbled
- CRC errors in received packets
- Intermittent communication

**Data Integrity Issues:**

**1. Interference:**
- **Co-channel:** Other LoRa devices on same frequency
- **Adjacent Channel:** Strong signals on nearby frequencies
- **Broadband Noise:** Electrical noise from nearby equipment

**2. Timing Issues:**
- **Clock Drift:** Crystal accuracy affects symbol timing
- **Preamble Detection:** Insufficient preamble length
- **Frame Timing:** TX/RX timing synchronization problems

**3. Protocol Issues:**
- **KISS Framing:** Incorrect frame boundaries
- **Escaping:** FESC/TFESC sequences corrupted
- **Buffer Overflows:** Packet too large for buffers

## Power Issues

### Battery Problems (V3)

**Symptoms:**
- Incorrect battery voltage readings
- Rapid battery drain
- Device shuts down unexpectedly

**Battery Issues:**

**1. Voltage Reading Errors:**
- **V3.2+ Boards:** Use standard configuration
- **Original V3 Boards:** Uncomment V3_ORIGINAL_BATTERY_LOGIC in platformio.ini
- **Calibration:** Battery readings may need offset adjustment

**2. Power Consumption:**
- **High Current Draw:** Check for short circuits
- **Sleep Mode:** Ensure device enters sleep when inactive
- **Radio Duty Cycle:** High transmit power increases consumption

**3. Battery Health:**
- **Capacity Loss:** LiPo batteries degrade over time
- **Internal Resistance:** High resistance causes voltage drops
- **Charging Issues:** Use proper LiPo charger with balancing

### Power Supply Problems

**Symptoms:**
- Device resets during transmission
- Unstable operation
- Watchdog timer resets

**Power Supply Issues:**

**1. Voltage Stability:**
- **Ripple:** Switching power supplies may have AC ripple
- **Dropout:** Voltage sags during high current draw
- **Noise:** Electrical noise on power lines

**2. Current Capacity:**
- **Peak Current:** LoRa TX requires 150-300mA peaks
- **Average Current:** 80-100mA in receive mode
- **USB Limits:** Some USB ports current-limit at 500mA

**3. Cable and Connector Issues:**
- **Resistance:** Long/thin cables have high resistance
- **Contact Problems:** Poor connections cause voltage drops
- **Connector Damage:** Bent pins or corrosion

**Diagnostic Commands:**
```bash
# Monitor voltage during operation (requires multimeter)
# Check voltage at device pins during TX/RX

# Monitor serial for watchdog resets
screen /dev/ttyACM0 115200
# Look for "ets_main.c" or watchdog messages
```

## Configuration Problems

### Settings Not Persisting

**Symptoms:**
- Configuration changes lost after reboot
- Web interface shows old values
- Command-line tools show unexpected values

**Persistence Issues:**

**1. SPIFFS Problems:**
```bash
# Check filesystem health
# Web interface should show configuration status

# Re-upload filesystem
pio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

**2. Save Operations:**
- **Manual Save:** Use `--save` flag with command-line tool
- **Web Interface:** Click "Save Configuration" button
- **Hardware Command:** Send HW_SAVE_CONFIG via KISS

**3. Corruption:**
- **Factory Reset:** Reset to defaults and reconfigure
- **Flash Erase:** Complete flash erase and re-upload
- **Backup Config:** Save working configuration before changes

### Invalid Configuration Values

**Symptoms:**
- Device accepts invalid parameters
- Communication fails with "valid" configuration
- Unexpected behavior with certain settings

**Parameter Validation:**

**1. Frequency Limits:**
- **433 MHz Band:** 433.05 - 434.79 MHz (Europe)
- **868 MHz Band:** 868.0 - 868.6 MHz (Europe)
- **915 MHz Band:** 902.0 - 928.0 MHz (USA)
- **Check Local Regulations:** Verify legal frequencies in your area

**2. Parameter Combinations:**
- **Bandwidth/SF Trade-off:** Higher SF allows narrower bandwidth
- **Coding Rate Impact:** Higher CR improves robustness but reduces throughput
- **Power Limits:** Respect regulatory limits (typically 14-20dBm ERP)

**3. Sync Word Conflicts:**
- **Default:** 0x1424 (avoid conflicts with other networks)
- **Private Networks:** Use unique sync words
- **Standard Values:** Some applications expect specific sync words

### Network Configuration Issues

**Symptoms:**
- WiFi connection fails
- TCP KISS server not accessible
- IP address conflicts

**Network Problems:**

**1. WiFi Issues:**
```bash
# Check WiFi scan results
nmcli device wifi list

# Test connection manually
nmcli device wifi connect "LoRaTNCX" password "password"

# Check IP assignment
ip addr show wlan0
```

**2. TCP Server Problems:**
```bash
# Test TCP connectivity
telnet 192.168.4.1 8001

# Check port binding
netstat -tlnp | grep 8001

# Firewall rules
sudo ufw status
sudo ufw allow 8001
```

**3. IP Address Issues:**
- **DHCP Problems:** Static IP may be needed in some networks
- **Subnet Conflicts:** Ensure unique IP in network range
- **Gateway Issues:** Check default route configuration

## GNSS Issues

### No GNSS Fix

**Symptoms:**
- No satellite lock after extended period
- "ANTENNA OPEN" messages in NMEA data
- Position data shows zeros or invalid values

**GNSS Problems:**

**1. Antenna Issues:**
- **Connection:** Verify antenna cable is securely connected
- **Type:** Use active GNSS antenna with proper voltage
- **Placement:** Clear sky view, away from metal objects
- **Cable Damage:** Check for cuts or shorts in antenna cable

**2. Module Configuration:**
```bash
# Check GNSS configuration
python3 tools/loratncx_config.py /dev/ttyACM0 --get-gnss

# Verify correct pins (V3 boards)
# RX: GNSS TX → MCU RX, TX: MCU TX → GNSS RX
```

**3. Environmental Factors:**
- **Indoor Operation:** Limited satellite visibility
- **Urban Canyon:** Buildings block satellite signals
- **Weather:** Generally not affected, but heavy cloud cover may reduce signal
- **Jamming:** Check for GPS jamming sources

**4. Hardware Issues:**
- **Module Damage:** Try different GNSS module
- **Power Supply:** GNSS needs clean 3.3V supply
- **UART Issues:** Check serial communication at 9600 baud

### Incorrect Position Data

**Symptoms:**
- Position readings are inaccurate
- Coordinates jump erratically
- Altitude readings wrong

**Position Accuracy Issues:**

**1. Satellite Geometry:**
- **HDOP:** High dilution of precision (>5) indicates poor geometry
- **Satellite Count:** Need at least 4 satellites for 3D fix
- **Elevation Angles:** Low elevation satellites less accurate

**2. Multipath Errors:**
- **Reflections:** Signals bouncing off buildings/ground
- **Antenna Placement:** Mount antenna away from reflective surfaces
- **Choke Ring:** Advanced antennas reduce multipath

**3. Timing Issues:**
- **Clock Drift:** GNSS module clock accuracy
- **Leap Seconds:** UTC timing corrections
- **Coordinate System:** Ensure correct datum (WGS84)

### NMEA Data Problems

**Symptoms:**
- No NMEA sentences received
- Garbled or incomplete sentences
- TCP server not providing data

**NMEA Issues:**

**1. Serial Communication:**
```bash
# Monitor raw NMEA data
screen /dev/ttyACM0 115200
# Look for $GNGGA, $GNRMC, etc.

# Check baud rate (default 9600)
# May need to configure module for different rate
```

**2. TCP Server Issues:**
```bash
# Test NMEA TCP server
nc 192.168.4.1 10110

# Check server status via web interface
curl http://192.168.4.1/api/gnss/status
```

**3. Sentence Parsing:**
- **Talker IDs:** GN (multi-constellation), GP (GPS), BD (BeiDou)
- **Checksums:** Verify NMEA checksums are valid
- **Field Format:** Check coordinate and time formats

## Advanced Diagnostics

### Serial Debug Output

**Enable Debug Logging:**
```cpp
// In main.cpp, change LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
```

**Monitor Debug Output:**
```bash
# Capture all debug information
screen /dev/ttyACM0 115200 | tee debug.log

# Look for error messages, initialization status, and state changes
```

### Web Interface Diagnostics

**API Status Endpoints:**
```bash
# System status
curl http://192.168.4.1/api/status

# LoRa radio status
curl http://192.168.4.1/api/lora/status

# GNSS status
curl http://192.168.4.1/api/gnss/status

# Network status
curl http://192.168.4.1/api/wifi/status
```

### Hardware Testing

**Factory Test Mode:**
```bash
# Upload factory test firmware
pio run --target upload --environment heltec_wifi_lora_32_V4_factory_test

# Run comprehensive hardware tests
# Tests OLED, serial, LED, WiFi, LoRa, GNSS
```

### Performance Monitoring

**Radio Statistics:**
- **RSSI:** Received signal strength indicator
- **SNR:** Signal-to-noise ratio
- **PER:** Packet error rate
- **Throughput:** Bytes per second

**System Resources:**
- **RAM Usage:** Free heap memory
- **CPU Usage:** Task execution times
- **Temperature:** ESP32 junction temperature
- **Uptime:** System runtime statistics

## Recovery Procedures

### Factory Reset

**Software Reset:**
```bash
# Via command-line tool
python3 tools/loratncx_config.py /dev/ttyACM0 --reset

# Via web interface
# Navigate to configuration page and click "Reset to Defaults"
```

**Hardware Reset:**
- **Full Flash Erase:** `pio run --target erase`
- **Re-upload:** Firmware and filesystem
- **Reconfigure:** All settings from defaults

### Backup and Restore

**Configuration Backup:**
```bash
# Save current configuration
python3 tools/loratncx_config.py /dev/ttyACM0 --get-all > config_backup.txt
```

**Configuration Restore:**
```bash
# Apply saved configuration
python3 tools/loratncx_config.py /dev/ttyACM0 --frequency 433.775
python3 tools/loratncx_config.py /dev/ttyACM0 --save
```

### Emergency Recovery

**Bootloader Access:**
1. Hold BOOT button while plugging in USB
2. Device enters bootloader mode
3. Use `esptool.py` for manual flash operations

**Minimal Firmware:**
- Upload basic firmware without web interface
- Use serial commands for configuration
- Gradually add features back

## Getting Help

### Diagnostic Information to Provide

When seeking help, include:

**System Information:**
- Board version (V3/V4)
- Firmware version
- PlatformIO version
- Operating system and version

**Configuration Details:**
- LoRa parameters (frequency, SF, BW, CR, power)
- WiFi mode and settings
- GNSS configuration
- Serial port and baud rate

**Error Messages:**
- Exact error text from serial monitor
- PlatformIO build/upload output
- Test script results
- Web interface error responses

**Test Results:**
- Automated test suite output
- Range test data
- Performance measurements
- Environmental conditions

### Community Resources

**Documentation:**
- [LoRaTNCX GitHub Repository](https://github.com/kc1awv/LoRaTNCX)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Technical Reference](https://www.espressif.com/en/products/socs/esp32)

**Community Support:**
- GitHub Issues for bug reports
- Amateur radio forums (LoRa, APRS discussions)
- ESP32 development communities

**Professional Support:**
- Heltec support for hardware issues
- Semtech LoRa module support
- Quectel GNSS module support

---

**Next:** [Section 15: Advanced Topics](15-advanced-topics.md) | **Previous:** [Section 13: Testing and Validation](13-testing-validation.md) | **Main Index:** [User Manual](../user_manual.md)