# LoRaTNCX Usage Examples

## Basic Setup and Testing

### 1. Initial Setup
Flash your device and connect to the serial console at 115200 baud. You should see:

```
===============================================
        LoRaTNCX Serial Console v1.1
===============================================

Board: Heltec WiFi LoRa 32 V4
CPU Frequency: 240 MHz
Free Heap: 294000 bytes
Frequency Band: 868 MHz (863-928 MHz)

Console ready. Type 'help' for available commands.
-----------------------------------------------
Initializing LoRa radio...
[LoRa] Radio initialized successfully
[LoRa] Configuration:
  Hardware: Heltec WiFi LoRa 32 V4
  Frequency: 868000000 Hz (868.0 MHz)
  TX Power: 20 dBm (max: 28 dBm)
  Bandwidth: 125 kHz
  Spreading Factor: SF7
  Coding Rate: 4/5
  Preamble Length: 8
  Symbol Timeout: 0
  Fixed Length Payload: No
  IQ Inversion: No
> 
```

### 2. Basic Communication Test

Set up two devices and test basic communication:

**Device 1:**
```
> lora send Hello from Device 1
Sending: "Hello from Device 1"
Message queued for transmission
[LoRa] TX completed successfully
> 
```

**Device 2 (should receive):**
```
[LoRa] RX: "Hello from Device 1" (RSSI: -45 dBm, SNR: 8 dB, Size: 20)
```

### 3. Configuration Commands

#### Check current status:
```
> lora status
LoRa Status: RX

> lora config
[LoRa] Configuration:
  Hardware: Heltec WiFi LoRa 32 V4
  Frequency: 868000000 Hz (868.0 MHz)
  TX Power: 20 dBm (max: 28 dBm)
  Bandwidth: 125 kHz
  Spreading Factor: SF7
  Coding Rate: 4/5
  Preamble Length: 8
  Symbol Timeout: 0
  Fixed Length Payload: No
  IQ Inversion: No

> lora stats
[LoRa] Statistics:
  TX Count: 1
  RX Count: 0
  Last RSSI: 0 dBm
  Last SNR: 0 dB
  Current State: RX
```

#### Change frequency (within valid band):
```
> lora freq 869.0
[LoRa] Frequency set to 869.0 MHz

> lora freq
Current frequency: 869.0 MHz
```

#### Adjust power level:
```
> lora power 14
[LoRa] TX power set to 14 dBm

> lora power
Current TX power: 14 dBm
```

#### Change spreading factor for better range/speed trade-off:
```
> lora sf 10
[LoRa] Spreading factor set to SF10

> lora sf
Current spreading factor: SF10
```

## Advanced Usage Scenarios

### Long Range Communication
For maximum range, use these settings:
```
> lora sf 12          # Slowest but longest range
> lora power 21       # Maximum power (adjust based on hardware)
> lora bw 0           # 125 kHz bandwidth
> lora cr 4           # 4/8 coding rate for best error correction
```

### High Speed Communication
For faster data rates:
```
> lora sf 7           # Fastest spreading factor
> lora bw 2           # 500 kHz bandwidth
> lora cr 1           # 4/5 coding rate
```

### Frequency Band Examples

#### 433 MHz ISM Band Usage:
```
> lora band ISM_433       # Select 433 MHz ISM band
> lora freq 434.0         # Set to 434 MHz (within band limits)
> lora send Test 433 MHz
```

#### 915 MHz ISM Band Usage (North America):
```
> lora band ISM_902_928   # Select North American ISM band
> lora freq 915.0         # Set to 915 MHz
> lora send Test 915 MHz
```

#### Amateur Radio 70cm Band:
```
> lora band AMATEUR_70CM  # Select 70cm amateur band
> lora freq 432.1         # Set to amateur frequency
> lora send CQ CQ DE CALL # Amateur radio transmission
```

#### View Available Bands:
```
> lora bands              # Show all available bands
> lora bands ism          # Show ISM bands only
> lora bands amateur      # Show amateur radio bands
```

## Testing Between Different Boards

### V3 to V4 Communication
Both devices must use identical LoRa parameters:

**Device V3 (22 dBm max):**
```
> lora power 15
> lora freq 868.0
> lora sf 8
> lora send Hello from V3
```

**Device V4 (22 dBm max):**
```
> lora power 15          # Match V3 power for fair test
> lora freq 868.0
> lora sf 8
> lora send Hello from V4
```

### Range Testing
1. Start with both devices close together
2. Gradually increase distance while testing communication
3. Monitor RSSI values to gauge signal strength:
```
[LoRa] RX: "Test message" (RSSI: -85 dBm, SNR: 5 dB, Size: 12)
```

RSSI Guidelines:
- **-30 to -60 dBm**: Excellent signal
- **-60 to -80 dBm**: Good signal  
- **-80 to -100 dBm**: Weak but usable
- **Below -100 dBm**: Very weak, may have errors

## Troubleshooting

### No Communication
1. Verify both devices use the same frequency band
2. Check that LoRa parameters match (SF, BW, CR)
3. Ensure adequate power levels
4. Verify antennas are connected

### Poor Range
1. Increase spreading factor: `lora sf 12`
2. Increase power: `lora power 21` (or `28` for V4)
3. Check antenna connections
4. Ensure clear line of sight

### Transmission Failures
```
[LoRa] TX timeout occurred
```
This usually indicates hardware issues. Try:
1. Reset the device: `reset`
2. Check power supply
3. Verify antenna connection

### Invalid Parameter Errors
```
[LoRa] Invalid frequency: 2400000000 Hz
[LoRa] Invalid TX power: 30 dBm (max: 28 dBm)
```
Refer to the valid ranges in the main README file.

## System Monitoring

### Check System Health:
```
> status
System Status:
  Uptime: 1205 seconds
  Free Heap: 285432 bytes
  CPU Frequency: 240 MHz
  Flash Size: 8388608 bytes
  Chip Model: ESP32-S3
  Chip Revision: 0
```

### Monitor Communication:
```
> lora stats
[LoRa] Statistics:
  TX Count: 15
  RX Count: 8
  Last RSSI: -67 dBm
  Last SNR: 9 dB
  Current State: RX
```

This gives you packet counts and signal quality metrics for performance evaluation.

## KISS TNC Mode Examples

### Entering KISS Mode
From the serial console:
```
> kiss
(Device enters KISS mode silently - no prompt appears)
```

The device is now in KISS mode and ready for packet radio applications.

### Using with APRS Software

1. **Setup your APRS application** (UI-View, Xastir, etc.)
2. **Configure port settings**:
   - Port: COM port of your device
   - Speed: 115200 baud
   - Protocol: KISS TNC
3. **Enter KISS mode**: Type `kiss` in console
4. **Start APRS application** - it will communicate via KISS frames

### Manual KISS Commands

You can send raw KISS frames to configure the LoRa radio. Here are some examples using hex bytes:

#### Set TX Power to 15 dBm:
```
C0 06 12 0F C0
```
- `C0` = FEND (frame start)
- `06` = SETHARDWARE command
- `12` = SET_TXPOWER sub-command  
- `0F` = 15 dBm (hex)
- `C0` = FEND (frame end)

#### Set Frequency to 915.0 MHz (915000000 Hz):
```
C0 06 10 36 89 69 00 C0
```
- `C0` = FEND
- `06` = SETHARDWARE command
- `10` = SET_FREQ_LOW sub-command
- `36 89 69 00` = 915000000 in big-endian format
- `C0` = FEND

#### Set Spreading Factor to SF8:
```
C0 06 14 08 C0
```
- `C0` = FEND
- `06` = SETHARDWARE command  
- `14` = SET_SF sub-command
- `08` = SF8
- `C0` = FEND

#### Set Bandwidth to 250 kHz:
```
C0 06 13 08 C0
```
- `C0` = FEND
- `06` = SETHARDWARE command
- `13` = SET_BANDWIDTH sub-command
- `08` = 250 kHz (see bandwidth encoding table)
- `C0` = FEND

#### Get Current Configuration:
```
C0 06 1A C0
```
This returns multiple KISS frames with current settings.

#### Select ISM 915 MHz Band (index 3):
```
C0 06 19 03 C0
```
- `19` = SELECT_BAND sub-command
- `03` = Band index (varies based on available bands)

### Exiting KISS Mode
Send RETURN command:
```
C0 FF C0
```
- `C0` = FEND
- `FF` = RETURN command
- `C0` = FEND

The device will return to command mode and display:
```
cmd:
```

### Integration Examples

#### Python KISS Client
```python
import serial
import time

# Open serial connection
ser = serial.Serial('COM3', 115200, timeout=1)

# Enter KISS mode
ser.write(b'kiss\r\n')
time.sleep(1)

# Set frequency to 915.0 MHz
kiss_freq_cmd = bytes([0xC0, 0x06, 0x10, 0x36, 0x89, 0x69, 0x00, 0xC0])
ser.write(kiss_freq_cmd)

# Send data frame
kiss_data = bytes([0xC0, 0x00]) + b"Hello LoRa" + bytes([0xC0])
ser.write(kiss_data)

# Listen for responses
while True:
    if ser.in_waiting:
        data = ser.read(ser.in_waiting)
        print(f"Received: {data.hex()}")
```

This demonstrates basic KISS communication for custom applications.