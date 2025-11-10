# 6. Configuration

LoRaTNCX provides multiple configuration methods to suit different user preferences and use cases. This section covers all available configuration approaches and their appropriate applications.

## 6.1. Using the Command-Line Tool

The `loratncx_config.py` script provides comprehensive command-line configuration capabilities, ideal for scripting, automation, and users who prefer terminal interfaces.

### Installation and Setup

```bash
# Make executable
chmod +x tools/loratncx_config.py

# Verify Python dependencies (usually pre-installed)
python3 --version  # Should be 3.6+
```

### Basic Usage

```bash
# Syntax
python3 tools/loratncx_config.py <serial_port> [options]

# Example
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config
```

### Query Commands

**Get Current Configuration:**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config
# Output:
# ================================================================
# LoRaTNCX Current Configuration
# ================================================================
#   Frequency:        915.000 MHz
#   Bandwidth:        125.0 kHz
#   Spreading Factor: SF12
#   Coding Rate:      4/7
#   Output Power:     20 dBm
#   Sync Word:        0x1424
# ================================================================
```

**Get Battery Voltage:**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-battery
# Output: Battery: 3.85V
```

**Get Board Information:**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-board
# Output: Board: Heltec WiFi LoRa 32 V4
```

**Get GNSS Status (V4 only):**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-gnss
# Output: GNSS: enabled, satellites: 8, position: 42.3601°N 71.0589°W
```

**Get All Information:**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-all
# Combines config + battery + board + gnss info
```

### Configuration Commands

**Set Frequency:**
```bash
# Valid ranges: 433.0 - 928.0 MHz
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 915.0
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 433.775
```

**Set Bandwidth:**
```bash
# Valid values: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500 kHz
python3 tools/loratncx_config.py /dev/ttyUSB0 --bandwidth 125.0
python3 tools/loratncx_config.py /dev/ttyUSB0 --bandwidth 62.5  # Long range
```

**Set Spreading Factor:**
```bash
# Valid range: 6-12 (higher = longer range, slower speed)
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 12  # Maximum range
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 7   # Fastest speed
```

**Set Coding Rate:**
```bash
# Valid range: 5-8 (4/5 through 4/8, higher = better error correction)
python3 tools/loratncx_config.py /dev/ttyUSB0 --coding-rate 7  # 4/7 (default)
python3 tools/loratncx_config.py /dev/ttyUSB0 --coding-rate 8  # 4/8 (best FEC)
```

**Set Output Power:**
```bash
# Valid range: 2-20 dBm (V3 limited by internal PA, V4 can use full range)
python3 tools/loratncx_config.py /dev/ttyUSB0 --power 20   # Maximum power
python3 tools/loratncx_config.py /dev/ttyUSB0 --power 10   # Reduced power
```

**Set Sync Word:**
```bash
# 2-byte hex value (default: 0x1424 for public networks)
python3 tools/loratncx_config.py /dev/ttyUSB0 --syncword 0x1424
python3 tools/loratncx_config.py /dev/ttyUSB0 --syncword 0x3444  # LoRaWAN
```

**GNSS Control (V4 only):**
```bash
# Enable/disable GNSS
python3 tools/loratncx_config.py /dev/ttyUSB0 --gnss-enable true
python3 tools/loratncx_config.py /dev/ttyUSB0 --gnss-enable false
```

### Management Commands

**Save Configuration:**
```bash
# Save current settings to non-volatile storage
python3 tools/loratncx_config.py /dev/ttyUSB0 --save
```

**Factory Reset:**
```bash
# Reset all settings to factory defaults
python3 tools/loratncx_config.py /dev/ttyUSB0 --reset
```

### Advanced Usage

**Batch Configuration:**
```bash
# Configure for long-range operation
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --frequency 915.0 \
    --bandwidth 62.5 \
    --spreading-factor 12 \
    --coding-rate 8 \
    --power 20 \
    --save
```

**Scripting Example:**
```bash
#!/bin/bash
# Configure multiple TNCs identically
PORTS=("/dev/ttyUSB0" "/dev/ttyUSB1" "/dev/ttyACM0")

for port in "${PORTS[@]}"; do
    echo "Configuring $port..."
    python3 tools/loratncx_config.py $port --frequency 915.0 --save
done
```

### Troubleshooting

**Port Not Found:**
```bash
# List available serial ports
python3 -c "import serial.tools.list_ports; print([p.device for p in serial.tools.list_ports.comports()])"
```

**Permission Denied:**
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
# Logout and login again
```

**No Response:**
- Verify correct serial port
- Check baud rate (default 115200)
- Ensure TNC is powered and firmware uploaded
- Try resetting the TNC

## 6.2. Web Interface Configuration

The web interface provides an intuitive, browser-based configuration experience suitable for most users and all device types.

### Accessing the Interface

**Default Access:**
1. Connect to LoRaTNCX WiFi network: `LoRaTNCX-XXXX`
2. Password: `loratncx`
3. Open browser to: `http://192.168.4.1`

**Station Mode Access:**
- Connect to same network as TNC
- Access via TNC's IP address
- Default: `http://192.168.4.1` (may vary in station mode)

### Interface Overview

The web interface consists of several main sections:

**Status Dashboard:**
- Real-time system information
- Battery voltage and WiFi status
- Current LoRa configuration
- GNSS status (V4 only)

**LoRa Configuration:**
- Frequency, bandwidth, spreading factor
- Coding rate and output power
- Sync word settings
- Configuration save/load

**WiFi Configuration:**
- Network mode selection (AP/STA/AP+STA)
- Access point settings
- Station mode network selection
- Security settings

**GNSS Configuration (V4):**
- GNSS enable/disable
- TCP port configuration
- Serial passthrough settings
- Real-time position display

### Configuration Workflow

1. **Connect** to TNC's WiFi or network
2. **Navigate** to configuration sections
3. **Adjust** parameters using form controls
4. **Save** changes to take effect
5. **Verify** settings on status dashboard

### Form Validation

The web interface includes client-side validation:
- Parameter range checking
- Real-time feedback
- Error highlighting
- Confirmation dialogs for critical changes

### Real-time Updates

- **AJAX Polling**: Status updates every 5 seconds
- **Live Feedback**: Immediate parameter validation
- **Connection Monitoring**: Network status indicators

### Mobile Compatibility

- **Responsive Design**: Works on phones/tablets
- **Touch Controls**: Optimized for mobile interaction
- **Simplified Layout**: Mobile-friendly navigation

## 6.3. KISS Protocol Configuration

For advanced users and applications that need programmatic configuration, LoRaTNCX supports direct KISS protocol commands.

### KISS Frame Format

All configuration uses standard KISS framing:
```
┌───────┬───────┬─────────┬─────────────────┐
│ FEND  │ CMD   │ SUBCMD  │ DATA...         │ FEND
│ 0xC0  │ 0x06  │ 0x01    │ <frequency>     │ 0xC0
└───────┴───────┴─────────┴─────────────────┘
```

### SETHARDWARE Commands (0x06)

| Subcommand | Data Format  | Description                 |
| ---------- | ------------ | --------------------------- |
| 0x01       | 4-byte float | Set frequency (MHz)         |
| 0x02       | 4-byte float | Set bandwidth (kHz)         |
| 0x03       | 1-byte uint  | Set spreading factor (7-12) |
| 0x04       | 1-byte uint  | Set coding rate (5-8)       |
| 0x05       | 1-byte int   | Set TX power (2-20 dBm)     |
| 0x06       | -            | Get current configuration   |
| 0x07       | -            | Save to NVS                 |
| 0x08       | 2-byte uint  | Set sync word               |
| 0x09       | 1-byte bool  | Enable/disable GNSS         |
| 0xFF       | -            | Factory reset               |

### GETHARDWARE Queries (0x07)

| Subcommand | Response Format | Description          |
| ---------- | --------------- | -------------------- |
| 0x01       | Config struct   | Radio configuration  |
| 0x02       | 4-byte float    | Battery voltage      |
| 0x03       | String          | Board information    |
| 0x04       | GNSS struct     | GNSS status/position |
| 0xFF       | All data        | Complete status      |

### Example KISS Commands

**Set Frequency to 915 MHz:**
```
C0 06 01 42 F6 00 00 C0
# 0x42F60000 = 915.0 as IEEE 754 float
```

**Get Configuration:**
```
C0 06 06 C0
# Response: C0 07 01 <config_data> C0
```

**Save Configuration:**
```
C0 06 07 C0
```

### Integration with Applications

KISS configuration works with any terminal program or custom application that can send/receive serial data.

**Python Example:**
```python
import serial
import time

def send_kiss_command(port, command_bytes):
    ser = serial.Serial(port, 115200, timeout=1)
    # Add FEND framing
    frame = b'\xC0' + command_bytes + b'\xC0'
    ser.write(frame)
    time.sleep(0.1)
    response = ser.read(ser.in_waiting)
    ser.close()
    return response

# Set frequency to 915 MHz
freq_cmd = b'\x06\x01' + (915.0).to_bytes(4, 'big', signed=False)
send_kiss_command('/dev/ttyUSB0', freq_cmd)
```

## 6.4. Persistent Settings

LoRaTNCX automatically saves configuration changes to non-volatile storage, ensuring settings persist across power cycles.

### Storage Mechanism

**NVS (Non-Volatile Storage):**
- ESP32's built-in flash storage system
- Key-value storage with wear leveling
- Survives firmware updates
- Limited write cycles (100,000+)

### Configuration Persistence

**Automatic Saving:**
- Web interface changes saved immediately
- Command-line tool requires explicit `--save` flag
- KISS commands require explicit save subcommand (0x07)

**What Gets Saved:**
- LoRa radio parameters (frequency, BW, SF, CR, power, sync word)
- WiFi configuration (mode, credentials, settings)
- GNSS settings (enable/disable, TCP port)
- System preferences

**What Doesn't Get Saved:**
- Current battery voltage (real-time only)
- GNSS position data (temporary)
- Network scan results (temporary)

### Configuration Management

**Backup Current Settings:**
```bash
# Use command-line tool to document settings
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-all > config_backup.txt
```

**Restore from Backup:**
```bash
# Manual restoration using saved values
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 915.0 --save
```

**Factory Reset:**
- Web interface: System → Factory Reset
- Command line: `loratncx_config.py --reset`
- KISS command: `C0 06 FF C0`

### Configuration Conflicts

**Multiple Configuration Methods:**
- Changes from any method are immediately effective
- Last change wins (no method priority)
- Save operations commit current runtime settings

**Network Considerations:**
- WiFi settings changes may require reconnection
- IP address changes require interface restart
- Port changes require service restart

### Best Practices

1. **Test Configuration**: Verify settings work as expected
2. **Save Regularly**: Use save commands after changes
3. **Document Changes**: Keep records of working configurations
4. **Backup Critical Settings**: Document important configurations
5. **Test After Reset**: Verify operation after factory reset

---

[← Previous: Installation and Setup](05-installation-setup.md) | [Back to Main Manual](../user_manual.md) | [Next: LoRa Radio Configuration →](07-lora-radio-config.md)