# LoRaTNCX Configuration Tool

Command-line utility to configure LoRa radio parameters on a LoRaTNCX KISS TNC.

## Purpose

Most KISS applications (like Dire Wolf, APRS clients, packet terminal programs) don't know about LoRa-specific parameters. They expect a pre-configured TNC that's ready to accept KISS data frames.

This tool allows you to configure your LoRaTNCX before launching your KISS application, so the radio is set up with the correct frequency, spreading factor, bandwidth, etc.

## Installation

No installation needed - just run the Python script directly:

```bash
chmod +x tools/loratncx_config.py
python3 tools/loratncx_config.py <port> [options]
```

## Usage

### Get Current Configuration

```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config
```

Output:
```
============================================================
LoRaTNCX Current Configuration
============================================================
  Frequency:        915.000 MHz
  Bandwidth:        125.0 kHz
  Spreading Factor: SF12
  Coding Rate:      4/7
  Output Power:     20 dBm
  Sync Word:        0x1424
============================================================
```

### Set Individual Parameters

```bash
# Set frequency
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 915.0

# Set spreading factor (6-12)
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 12

# Set bandwidth (kHz)
python3 tools/loratncx_config.py /dev/ttyUSB0 --bandwidth 125

# Set coding rate (5-8 for 4/5 to 4/8)
python3 tools/loratncx_config.py /dev/ttyUSB0 --coding-rate 7

# Set output power (dBm)
python3 tools/loratncx_config.py /dev/ttyUSB0 --power 20

# Set sync word (hex)
python3 tools/loratncx_config.py /dev/ttyUSB0 --syncword 0x1424
```

### Set Multiple Parameters at Once

```bash
# Configure for long range
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --frequency 915.0 \
    --spreading-factor 12 \
    --bandwidth 62.5 \
    --power 20

# Configure for fast data rate
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --spreading-factor 7 \
    --bandwidth 500 \
    --coding-rate 5
```

### Save Configuration to NVS

Save the current configuration so it persists across reboots:

```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --save
```

### Reset to Factory Defaults

```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --reset
```

## Parameters Reference

### Frequency
- **Range**: 433-928 MHz (depends on your board variant)
- **Common Values**:
  - 433.775 MHz (EU 70cm)
  - 868.0 MHz (EU ISM)
  - 915.0 MHz (US ISM)
  - 902-928 MHz (US ISM band)
- **Units**: MHz
- **Flag**: `--frequency` or `-f`

### Bandwidth
- **Range**: 7.8 - 500 kHz
- **Common Values**: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500
- **Units**: kHz
- **Flag**: `--bandwidth` or `-b`
- **Note**: Lower bandwidth = longer range, slower data rate

### Spreading Factor
- **Range**: 6-12
- **Common Values**: 7 (fast), 9 (balanced), 12 (long range)
- **Flag**: `--spreading-factor` or `-s`
- **Note**: Higher SF = longer range, slower data rate, more air time

### Coding Rate
- **Range**: 5-8 (represents 4/5, 4/6, 4/7, 4/8)
- **Common Value**: 7 (4/7)
- **Flag**: `--coding-rate` or `-c`
- **Note**: Higher CR = more error correction, slower data rate

### Output Power
- **Range**: Typically 2-20 dBm (check your board's max)
- **Common Value**: 20 dBm (100 mW)
- **Units**: dBm
- **Flag**: `--power` or `-p`

### Sync Word
- **Range**: 0x0000-0xFFFF (2-byte value)
- **Common Values**:
  - 0x1424 (LoRaWAN private)
  - 0x3444 (LoRaWAN public)
  - 0x1234 (custom)
- **Flag**: `--syncword` or `-w`
- **Note**: Both TNCs must use the same sync word to communicate

## Performance Guide

### Long Range Configuration
Maximum range, slow data rate (~250 bps):
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --spreading-factor 12 \
    --bandwidth 62.5 \
    --coding-rate 8 \
    --power 20 \
    --save
```

### Balanced Configuration
Good range, reasonable speed (~1-2 kbps):
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --spreading-factor 9 \
    --bandwidth 125 \
    --coding-rate 7 \
    --power 17 \
    --save
```

### Fast Configuration
Shorter range, fast data rate (~5-10 kbps):
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
    --spreading-factor 7 \
    --bandwidth 250 \
    --coding-rate 5 \
    --power 14 \
    --save
```

## Using with KISS Applications

1. **Configure the TNC first**:
   ```bash
   python3 tools/loratncx_config.py /dev/ttyUSB0 \
       --frequency 915.0 \
       --spreading-factor 12 \
       --save
   ```

2. **Launch your KISS application** (examples):
   ```bash
   # Dire Wolf
   direwolf -c direwolf.conf -p /dev/ttyUSB0 -b 115200

   # APRS client
   xastir
   
   # Packet terminal
   linpac
   ```

3. **The TNC is now ready** - your application will send/receive KISS frames, and the TNC handles all the LoRa modulation/demodulation automatically.

## Troubleshooting

### "Could not retrieve configuration"
- Check that the TNC is powered on
- Verify the correct serial port (try `ls /dev/tty*`)
- Make sure no other program is using the port
- Try unplugging and replugging the USB cable

### "Permission denied" on Linux
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Configuration doesn't persist after reboot
Make sure to use `--save` to write to NVS:
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --save
```

### Both TNCs configured but can't communicate
- Verify both TNCs have the **same** frequency, bandwidth, spreading factor, coding rate, and sync word
- Check antennas are connected
- Try increasing power or decreasing distance
