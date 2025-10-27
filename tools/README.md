# LoRaTNCX Configuration Tools

This directory contains utilities for configuring and managing LoRaTNCX devices.

## LoRaTNCX Configuration Tool (`loratncx_config.py`)

A command-line utility for configuring LoRaTNCX devices using KISS protocol commands. This tool provides an easy way to set radio parameters, timing values, and query device configuration.

### Installation

1. Ensure Python 3.6 or later is installed
2. Install required dependencies:
   ```bash
   pip3 install -r requirements.txt
   ```

### Quick Start

1. **Test your connection first**:
   ```bash
   python3 test_connection.py /dev/ttyACM0
   ```
   This will verify connectivity and show device debug output.

2. **Get current configuration**:
   ```bash
   python3 loratncx_config.py --port /dev/ttyACM0 --get-config
   ```

3. **Try interactive mode**:
   ```bash
   python3 loratncx_config.py --port /dev/ttyACM0 --interactive
   ```

### Usage

#### Basic Configuration Commands

```bash
# Set frequency to 433.175 MHz
python3 loratncx_config.py --port /dev/ttyACM0 --frequency 433.175

# Configure for long-range operation (SF12, CR 4/8, high power)
python3 loratncx_config.py --port /dev/ttyACM0 --sf 12 --cr 8 --power 20

# Set CSMA parameters for busy network environment
python3 loratncx_config.py --port /dev/ttyACM0 --txdelay 50 --persist 31 --slottime 20

# Get current device configuration
python3 loratncx_config.py --port /dev/ttyACM0 --get-config
```

#### Interactive Mode

For easier experimentation and configuration, use interactive mode:

```bash
python3 loratncx_config.py --port /dev/ttyACM0 --interactive
```

In interactive mode, you can enter commands like:
- `freq 433.175` - Set frequency to 433.175 MHz
- `power 14` - Set TX power to 14 dBm
- `sf 9` - Set spreading factor to 9
- `config` - Get current configuration
- `help` - Show available commands
- `quit` - Exit

### Command Line Options

#### Connection Parameters
- `--port`, `-p`: Serial port (required, e.g., `/dev/ttyACM0`, `COM3`)
- `--baudrate`, `-b`: Baud rate (default: 115200)
- `--timeout`, `-t`: Serial timeout in seconds (default: 2.0)

#### Radio Parameters
- `--frequency`, `--freq`, `-f`: Set frequency in MHz (150.0 - 960.0)
- `--power`: Set TX power in dBm (-9 to +22)
- `--bandwidth`, `--bw`: Set bandwidth in kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0)
- `--spreading-factor`, `--sf`: Set spreading factor (7-12)
- `--coding-rate`, `--cr`: Set coding rate (5-8, representing 4/5 to 4/8)

#### CSMA Parameters
- `--txdelay`: Set TX delay in 10ms units (0-255)
- `--persistence`, `--persist`: Set persistence value (0-255)
- `--slottime`: Set slot time in 10ms units (0-255)

#### Actions
- `--get-config`: Get current configuration
- `--interactive`, `-i`: Run in interactive mode
- `--debug`, `-d`: Show debug output from device
- `--monitor`, `-m`: Monitor debug output for specified seconds

### Parameter Guidelines

#### Frequency Bands
- **ISM 433 MHz**: 433.050 - 434.790 MHz (EMEA, Russia, APAC)
- **ISM 868 MHz**: 863.000 - 870.000 MHz (Europe)
- **ISM 915 MHz**: 902.000 - 928.000 MHz (Americas)
- **Amateur bands**: Check your license privileges and local band plans

#### LoRa Parameters

**Spreading Factor (SF)**:
- SF7: Fastest data rate (~5.5 kbps @ 125kHz BW), shortest range
- SF12: Slowest data rate (~0.25 kbps @ 125kHz BW), longest range
- Each increment doubles airtime and improves sensitivity by ~2.5dB

**Bandwidth**:
- Narrow bandwidth: Better sensitivity, longer range, lower data rate
- Wide bandwidth: Higher data rate, shorter range, less sensitive

**Coding Rate**:
- 5 (4/5): Minimal overhead, best data rate
- 8 (4/8): Maximum error correction, lowest data rate

**TX Power**:
- Low Power (-9 to 2 dBm): Very local, battery conservation
- Medium Power (2-14 dBm): Local to regional networks
- High Power (17-22 dBm): Long distance, high power consumption

#### CSMA Parameters

**TX Delay**: Pre-transmission delay allowing receiver synchronization
- Default: 30 (300ms)
- Range: 0-255 (0-2550ms in 10ms increments)

**Persistence**: p-persistent CSMA transmission probability
- Formula: Probability = (value + 1) / 256
- Default: 63 (~25% probability)
- 0: Never transmit (0.4% probability)
- 255: Always transmit (100% probability)

**Slot Time**: CSMA back-off interval
- Default: 10 (100ms)
- Range: 0-255 (0-2550ms in 10ms increments)

### Examples

#### Short Range, High Data Rate
```bash
python3 loratncx_config.py --port /dev/ttyACM0 \
    --frequency 915.0 --sf 7 --bw 250.0 --cr 5 --power 8
```

#### Long Range, Low Data Rate
```bash
python3 loratncx_config.py --port /dev/ttyACM0 \
    --frequency 433.175 --sf 12 --bw 62.5 --cr 8 --power 20
```

#### APRS Configuration (Medium Range)
```bash
python3 loratncx_config.py --port /dev/ttyACM0 \
    --frequency 433.175 --sf 9 --bw 125.0 --cr 7 --power 14 \
    --txdelay 30 --persist 63 --slottime 10
```

#### Busy Network Environment
```bash
python3 loratncx_config.py --port /dev/ttyACM0 \
    --txdelay 50 --persist 31 --slottime 20
```

#### Debug and Monitoring
```bash
# Monitor device debug output for 30 seconds
python3 loratncx_config.py --port /dev/ttyACM0 --monitor 30

# Configure with debug output visible
python3 loratncx_config.py --port /dev/ttyACM0 --debug \
    --frequency 433.175 --power 14

# Interactive mode with debug output
python3 loratncx_config.py --port /dev/ttyACM0 --debug --interactive
```

### Troubleshooting

**Connection Issues**:
- Verify the correct serial port path
- Check that the device is powered and connected
- Ensure no other applications are using the serial port
- Try different baud rates if communication fails

**Debug Output Interference**:
If you see debug messages like `[LOOP] System running - uptime: XXX seconds`, this is normal device debug output. The tool now handles this automatically:

```bash
# The tool filters debug output automatically
python3 loratncx_config.py --port /dev/ttyACM0 --get-config

# To see debug output alongside commands, use --debug
python3 loratncx_config.py --port /dev/ttyACM0 --debug --get-config

# To monitor debug output only
python3 loratncx_config.py --port /dev/ttyACM0 --monitor 30
```

**Configuration Not Applied**:
- Check for error messages in the device's serial console output
- Verify parameters are within valid ranges
- Use `--get-config` to verify current settings
- The device may print configuration to debug console instead of KISS interface

**Configuration Query Shows No Response**:
This can happen if the device outputs configuration to the debug console rather than the KISS interface. Try:
1. Use `--debug` flag to see all device output
2. Use `--monitor 10` to watch the debug stream
3. Check if configuration appears in the debug messages

**Permission Errors (Linux/macOS)**:
```bash
# Add user to dialout group (logout/login required)
sudo usermod -a -G dialout $USER

# Or run with sudo (not recommended)
sudo python3 loratncx_config.py --port /dev/ttyACM0 --get-config
```

### KISS Protocol Reference

This tool implements the KISS protocol commands as documented in `../docs/LORATNCX_KISS_PROTOCOL.md`. The script handles proper KISS frame construction, byte stuffing, and command formatting automatically.

### Contributing

When adding new features or commands:
1. Follow the existing code style and structure
2. Add parameter validation and helpful error messages  
3. Update the help text and documentation
4. Test with actual hardware when possible

### License

This tool is part of the LoRaTNCX project and follows the same license terms.