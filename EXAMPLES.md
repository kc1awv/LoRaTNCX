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

```

This demonstrates basic KISS communication for custom applications.

## TNC-2 Compatible Command Extensions

### Overview of TNC-2 Command Integration

The LoRaTNCX can be extended to support classic TNC-2 commands while maintaining its modern LoRa-specific features. This section outlines how traditional packet radio commands could be adapted for LoRa operation.

**Important Note**: These are proposed extensions to make LoRaTNCX more familiar to traditional packet radio operators. Many commands use the same capitalization conventions as the original TNC-2, where capital letters indicate the minimum abbreviation.

### Core TNC-2 Commands (Proposed Implementation)

#### Station Identification
```bash
# Set station callsign
> MYcall KC1AWV-1
Station callsign set to: KC1AWV-1

# Set digipeater alias  
> MYAlias LORA1
Digipeater alias set to: LORA1

# Show current callsign
> MYcall
Current callsign: KC1AWV-1
```

#### Beacon Configuration
```bash
# Enable beacon every 600 seconds (10 minutes)
> Beacon E 600  
Beacon enabled: Every 600 seconds

# Set beacon text (120 char max, like original TNC-2)
> BText LoRaTNCX - Experimental LoRa Packet Node
Beacon text set

# Show beacon status
> Beacon
Beacon: Every 600 seconds
```

#### Connection Management  
```bash
# Connect to another station
> Connect KC1AWV-2 via LORA2
Connecting to KC1AWV-2 via LORA2...

# Allow/disallow connections
> CONOk ON
Incoming connections allowed

> CONOk OFF  
Incoming connections disabled

# Connection status
> CStatus
Stream 0: Connected to KC1AWV-2 (120 sec)
Stream 1: Disconnected
```

#### Monitor Mode
```bash
# Enable monitoring
> Monitor ON
Monitor mode enabled

# Monitor with timestamps
> MStamp ON
Monitor timestamps enabled

# Filter monitoring by callsign patterns
> MFilter KC1 N1 W1
Monitor filter: KC1*, N1*, W1*

# Show stations heard
> MHeard
Stations heard:
KC1AWV-2  14:30:25  -67dBm  SF7
N1ABC-5   14:28:10  -78dBm  SF8
W1XYZ     14:25:33  -45dBm  SF7
```

#### Flow Control and Terminal Settings
```bash
# Terminal character length (7/8 bit)  
> AWlen 8
Terminal set to 8-bit mode

# XON/XOFF flow control
> Xflow ON
XON/XOFF flow control enabled

# Screen width for formatting
> Screenln 132
Screen width set to 132 characters

# Echo typed characters
> Echo ON
Local echo enabled
```

#### LoRa-Specific Adaptations

Traditional TNC-2 commands adapted for LoRa operation:

```bash
# Transmit delay (adapted for LoRa preamble timing)
> TXdelay 8
Preamble length set to 8 symbols

# Frame retry count
> RETry 5
Maximum retries set to 5

# Maximum frame size (LoRa packet length)
> Paclen 255
Maximum packet length: 255 bytes

# Response time (adapted for LoRa timing)
> RESptime 20  
ACK response time: 2000ms
```

### Advanced TNC-2 Features

#### Digipeating Support
```bash
# Enable LoRa digipeating
> DIGipeat ON
LoRa digipeating enabled

# Set maximum digipeat hops
> DIGihops 3
Maximum digipeater hops: 3

# Show digipeater statistics
> DIGistat
Digipeater Statistics:
  Packets repeated: 47
  Packets dropped: 3
  Current load: Low
```

#### Link Management
```bash
# Maximum outstanding frames (window size)
> MAXframe 2
Window size set to 2 frames

# Frame acknowledgment timing
> FRack 5
Frame acknowledgment timer: 5 units

# Link quality monitoring  
> CHech 120
Link check timeout: 120 seconds

# Disconnect after idle time
> DIScon 600
Auto-disconnect after 600 seconds idle
```

#### Converse and Transparent Modes
```bash
# Enter converse mode
> CONVers
*** Connected to KC1AWV-2
Hello, this is a test message in converse mode
[Message transmitted via LoRa]

# Set converse mode as default after connect
> CONMode CONVERS
Default mode after connect: CONVERSE

# Enter transparent mode (binary data)
> TRANS
*** Entering transparent mode
[All data passes through without processing]

# Escape back to command mode (Ctrl+C default)
^C
cmd: Back in command mode
```

### Modern Enhancements to TNC-2 Commands

#### LoRa Parameter Integration
```bash
# Traditional frequency command with LoRa bands
> FREQuency 915.0
Frequency set to 915.0 MHz (ISM Band)

# Power output (traditional HAM style)
> POWer 15
TX Power: 15 dBm (≈ 32 mW)

# LoRa spreading factor as "speed"
> SPeed 7
LoRa Spreading Factor: SF7 (fastest)

# Bandwidth selection
> BANdwidth 250
LoRa Bandwidth: 250 kHz
```

#### Error Correction and Statistics
```bash
# Show detailed LoRa statistics
> STats
LoRa Statistics:
  Packets TX: 1,247
  Packets RX: 2,108  
  TX Errors: 12
  RX CRC Errors: 43
  Average RSSI: -72 dBm
  Average SNR: 8.2 dB
  Band Usage: 0.3%

# Show current LoRa status
> STatus  
Status: Connected to KC1AWV-2
LoRa Mode: SF8 BW125 CR4/5
Frequency: 915.0 MHz
Power: 15 dBm
Link Quality: Excellent (-45 dBm, SNR 12 dB)
```

### Configuration Management
```bash
# Save current configuration to NVS
> SAVe
Configuration saved to non-volatile storage

# Reset to factory defaults
> RESET
Resetting to factory defaults...

# Display current parameters
> DISplay
[Shows all current TNC-2 compatible parameters]

# Calibrate LoRa radio (frequency adjustment)
> CALibra
Performing LoRa frequency calibration...
Calibration complete: +0.2 kHz offset
```

### Command Shortcuts and Abbreviations

Following TNC-2 tradition, commands support minimum abbreviations (shown in capitals):

```bash
# Full command vs minimum abbreviation
MYcall          → MY
CONVers         → CON  
MONitor         → M
BEacon          → B
STatus          → ST
DISplay         → D
DIGipeat        → DIG
FREQuency       → F
CONnect         → C
DISconnect      → DISC
KISS            → K
```

### Integration with Packet Radio Software

The enhanced TNC-2 command set allows LoRaTNCX to work with existing packet radio applications:

```bash
# Terminal Node Controller software compatibility
> TNCtype TNC2
TNC-2 compatibility mode enabled

# APRS-specific enhancements
> APrs ON
APRS mode enabled - UI frames enhanced

# Automatic position beaconing
> POSition 42.3601,-71.0589
Position set for APRS beacons

# Weather station integration  
> WEAther ON
Weather data integration enabled
```

### Network and Routing Features

```bash
# Network routing table
> ROUtes
LoRa Routing Table:
KC1AWV-2 → Direct (RF)
N1ABC    → via KC1AWV-2 (2 hops)
W1XYZ-1  → via LORA1 (1 hop)

# Network identifier
> NETwork LORA-MESH-1
Network ID set to: LORA-MESH-1

# Automatic routing updates
> ROUtupd 300
Route updates every 300 seconds
```

This comprehensive command set would make LoRaTNCX familiar to traditional packet radio operators while leveraging LoRa's unique capabilities for improved range and reliability.

### Implementation Priority

If implementing these features, suggested priority order:
1. **Core Station ID**: MYcall, BText, Beacon
2. **Monitor Functions**: Monitor, MStamp, MHeard  
3. **Connection Management**: Connect, CONOk, CStatus
4. **LoRa Parameters**: Power, Frequency integration
5. **Advanced Features**: Digipeating, Routing, APRS integration

Each command should maintain backward compatibility with existing LoRaTNCX functionality while adding familiar TNC-2 behavior for traditional packet radio integration.

### TNC-2 Command Reference Table

Here's a comprehensive mapping of classic TNC-2 commands to proposed LoRaTNCX implementations:

| Original TNC-2 | Shorthand | LoRaTNCX Adaptation | Description |
|----------------|-----------|---------------------|-------------|
| `8bitconv`     | `8B`      | `TErminal 8bit`     | 8-bit terminal mode |
| `AUtolf`       | `AU`      | `AUtolf ON/OFF`     | Auto linefeed after CR |
| `AXDelay`      | `AXD`     | `PTTdelay <ms>`     | PTT keyup delay (LoRa: preamble) |
| `Beacon`       | `B`       | `Beacon E/A <sec>`  | Beacon timing control |
| `BText`        | `BT`      | `BText <text>`      | Beacon message text |
| `CANline`      | `CAN`     | `CANcel $18`        | Line delete character |
| `CHech`        | `CH`      | `CHeck <sec>`       | Link timeout monitoring |
| `CONMode`      | `CONM`    | `CONMode CONV/TRANS`| Default connection mode |
| `Connect`      | `C`       | `Connect <call> [via]`| Establish LoRa link |
| `CONOk`        | `CONO`    | `CONOk ON/OFF`      | Allow incoming connects |
| `CONVers`      | `CONV`    | `CONVers`           | Enter conversation mode |
| `CText`        | `CT`      | `CText <message>`   | Connect acknowledgment text |
| `DIGipeat`     | `DIG`     | `DIGipeat ON/OFF`   | LoRa digipeater function |
| `Disconnect`   | `D`       | `Disconnect`        | Terminate LoRa link |
| `Echo`         | `E`       | `Echo ON/OFF`       | Terminal echo control |
| `FRack`        | `FR`      | `FRack <units>`     | Frame ACK timeout |
| `HID`          | `H`       | `ID <interval>`     | Automatic ID transmission |
| `KISS`         | `K`       | `KISS`              | Enter KISS TNC mode |
| `MAXframe`     | `MAX`     | `MAXframe <count>`  | Window size (outstanding frames) |
| `Monitor`      | `M`       | `Monitor ON/OFF`    | RF monitoring mode |
| `MStamp`       | `MS`      | `MStamp ON/OFF`     | Timestamp monitored frames |
| `MYcall`       | `MY`      | `MYcall <callsign>` | Station identification |
| `MYAlias`      | `MYA`     | `MYAlias <alias>`   | Digipeater alias |
| `Paclen`       | `P`       | `Paclen <bytes>`    | Maximum packet length |
| `RETry`        | `R`       | `RETry <count>`     | Maximum retransmissions |
| `TXdelay`      | `TX`      | `TXdelay <symbols>` | Transmit preamble length |
| `Unproto`      | `UN`      | `Unproto <path>`    | UI frame destination |

### Extended LoRa-Specific Commands

| Command | Shorthand | Description | Example |
|---------|-----------|-------------|---------|
| `LORAfreq` | `LF` | Set LoRa frequency | `LORAfreq 915.0` |
| `LORApower` | `LP` | Set LoRa TX power | `LORApower 20` |
| `LORAsf` | `LSF` | Set spreading factor | `LORAsf 8` |
| `LORAband` | `LB` | Select frequency band | `LORAband ISM_915` |
| `LORAcr` | `LCR` | Set coding rate | `LORAcr 5` |
| `LORAbw` | `LBW` | Set bandwidth | `LORAbw 125` |
| `RSsi` | `RS` | Show RSSI statistics | `RSsi` |
| `SNr` | `SN` | Show SNR statistics | `SNr` |
| `BAnd` | `BA` | Show band information | `BAnd` |

This comprehensive command set bridges traditional packet radio operation with modern LoRa technology, making LoRaTNCX accessible to both new users and experienced packet radio operators.
````