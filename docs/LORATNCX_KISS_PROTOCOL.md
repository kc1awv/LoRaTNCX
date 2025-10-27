# KISS Protocol Reference

This document provides reference information for the KISS protocol implementation in the LoRaTNCX, including command mappings, frame formats, and usage examples.

## Protocol Overview

The KISS (Keep It Simple Stupid) protocol provides a standardized interface between packet radio applications and Terminal Node Controllers (TNCs). This implementation extends standard KISS with LoRa-specific configuration commands.

## Frame Structure

KISS frames use the following format:
```
FEND CMD [DATA...] FEND
```

**Frame Elements:**
- `FEND`: Frame End marker (0xC0)
- `CMD`: Command byte specifying the operation
- `DATA`: Optional command-specific payload data
- Byte stuffing applied to prevent data conflicts with frame markers

### Byte Stuffing Rules

To prevent confusion between data bytes and frame markers, the following substitutions are applied:

| Original    | Escaped Sequence       | Description              |
| ----------- | ---------------------- | ------------------------ |
| 0xC0 (FEND) | 0xDB 0xDC (FESC+TFEND) | Frame End marker in data |
| 0xDB (FESC) | 0xDB 0xDD (FESC+TFESC) | Escape character in data |

**Constants Used:**
- FEND = 0xC0 (Frame End)
- FESC = 0xDB (Frame Escape) 
- TFEND = 0xDC (Transposed Frame End)
- TFESC = 0xDD (Transposed Frame Escape)

## Standard KISS Commands

### Data Frame (0x00)
Transmits a packet over the radio interface.
- **Command Code**: 0x00 (includes port in upper nibble)
- **Format**: Port is encoded in upper 4 bits: `(port << 4) | 0x00`
- **Data**: Raw packet data to transmit
- **Example**: `C0 00 [packet_data] C0` (port 0)
- **Multi-port Example**: `C0 10 [packet_data] C0` (port 1)

### TX_DELAY (0x01)
Configures pre-transmission delay.
- **Command Code**: 0x01
- **Parameter**: 1 byte (0-255)
- **Units**: 10 milliseconds
- **Purpose**: Allows receiver time to synchronize before data transmission
- **Default**: 30 (300ms)
- **Range**: 0-2550ms

### PERSISTENCE (0x02)
Controls p-persistent CSMA probability.
- **Command Code**: 0x02
- **Parameter**: 1 byte (0-255)
- **Formula**: Probability = (parameter + 1) / 256
- **Purpose**: Reduces collision probability in multi-station networks
- **Default**: 63 (~25% transmission probability)
- **Special Values**:
  - 0: Never transmit (0.4% probability)
  - 255: Always transmit (100% probability)

### SLOT_TIME (0x03)
Configures CSMA back-off interval.
- **Command Code**: 0x03
- **Parameter**: 1 byte (0-255)
- **Units**: 10 milliseconds
- **Purpose**: Time to wait before retrying transmission after persistence test failure
- **Default**: 10 (100ms)
- **Range**: 0-2550ms

### TX_TAIL (0x04)
Configures post-transmission delay.
- **Command Code**: 0x04
- **Status**: Not implemented (no post-transmission delay needed for LoRa)

### FULL_DUPLEX (0x05)
Configures duplex mode.
- **Command Code**: 0x05
- **Status**: Not implemented (LoRa is inherently half-duplex)
- **Note**: Command is ignored if received

## Extended Hardware Commands

The TNC extends KISS with hardware-specific commands using the SET_HARDWARE (0x06) command with sub-command bytes.

### SET_HARDWARE (0x06)
Base command for radio configuration operations.
- **Format**: `C0 06 [subcmd] [data] C0`
- **Sub-commands**: Specific radio parameter to modify

#### Set Frequency (0x06 0x01)
Configures LoRa operating frequency.
- **Sub-command**: 0x01
- **Data Format**: 4 bytes, IEEE 754 float, little-endian
- **Range**: 150.0 - 960.0 MHz (hardware dependent)
- **Resolution**: 61 Hz (theoretical)
- **Example**: 433.175 MHz = `C0 06 01 AE 47 D8 43 C0`

**Regional Frequency Guidelines:**
- ISM 433 MHz: 433.050 - 434.790 MHz (EMEA & Russia, APAC, ITU Regions 1 & 3)
- ISM 868 MHz: 863.000 - 870.000 MHz (Europe / CEPT / ETSI) 
- ISM 915 MHz: 902.000 - 928.000 MHz (The Americas, ITU Region 2)
- Amateur bands: Check your license privileges and local band plans

#### Set TX Power (0x06 0x02)
Configures transmitter output power.
- **Sub-command**: 0x02
- **Data Format**: 1 byte, signed integer
- **Range**: -9 to +22 dBm (hardware dependent)
- **Resolution**: 1 dBm
- **Default**: 8 dBm
- **Example**: 14 dBm = `C0 06 02 0E C0`
- **Example**: -3 dBm = `C0 06 02 FD C0` (0xFD = -3 in signed 8-bit)

**Power Level Guidelines:**
- Low Power: -9 to 2 dBm (very local, battery conservation)
- Medium Power: 2-14 dBm (local to regional networks)
- High Power: 17-22 dBm (long distance, high power consumption)
- **Note**: Respect local regulations and amateur radio power limits

#### Set Bandwidth (0x06 0x03)
Configures LoRa signal bandwidth.
- **Sub-command**: 0x03
- **Data Format**: 4 bytes, IEEE 754 float, little-endian
- **Valid Options**: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0 kHz
- **Note**: Actual supported bandwidths depend on RadioLib and hardware capabilities
- **Default**: 125.0 kHz
- **Trade-offs**:
  - Narrow bandwidth: Better sensitivity, longer range, lower data rate
  - Wide bandwidth: Higher data rate, shorter range, less sensitive

#### Set Spreading Factor (0x06 0x04)
Configures LoRa spreading factor.
- **Sub-command**: 0x04
- **Data Format**: 1 byte, unsigned integer
- **Range**: 7-12
- **Default**: 9
- **Characteristics**:
  - SF7: Fastest data rate (~5.5 kbps @ 125kHz BW)
  - SF12: Slowest data rate (~0.25 kbps @ 125kHz BW)
  - Each increment doubles airtime and improves sensitivity by ~2.5dB

#### Set Coding Rate (0x06 0x05)
Configures LoRa forward error correction.
- **Sub-command**: 0x05
- **Data Format**: 1 byte (5-8, representing 4/5 to 4/8)
- **Options**:
  - 5 (4/5): Minimal overhead, best data rate
  - 6 (4/6): Light error correction
  - 7 (4/7): Moderate error correction (default)
  - 8 (4/8): Maximum error correction, lowest data rate
- **Trade-off**: Higher coding rates provide better error correction at the cost of increased airtime

#### Get Configuration (0x06 0x10)
Requests current radio configuration.
- **Sub-command**: 0x10
- **Data Format**: None (query command)
- **Response**: Configuration printed to serial console
- **Example**: `C0 06 10 C0`

**Response Format:**
```
Current Radio Configuration:
  Frequency: 915.00 MHz
  TX Power: 8 dBm
  Bandwidth: 125.0 kHz
  Spreading Factor: 9
  Coding Rate: 7 (4/7)
  TX Delay: 30 x 10ms
  Persistence: 63
  Slot Time: 10 x 10ms
```

## Channel Access Implementation

The TNC implements p-persistent CSMA (Carrier Sense Multiple Access) algorithm:

### Algorithm Steps

1. **Channel Assessment**: Check for recent packet reception activity
2. **Clear Channel Procedure**:
   - Generate random number R (0-255)
   - If R ≤ PERSISTENCE: proceed to transmission
   - If R > PERSISTENCE: wait SLOT_TIME and repeat from step 1
3. **Busy Channel Procedure**:
   - Wait SLOT_TIME
   - Return to step 1
4. **Transmission Sequence**:
   - Wait TX_DELAY period
   - Transmit packet
   - Return to monitoring mode

### Channel Busy Detection

Channel is considered busy if:
- Packet reception occurred within last 1000ms (1 second)
- Current RSSI above threshold (future enhancement)

## Data Format Specifications

### IEEE 754 Float Encoding

Frequency and bandwidth parameters use 32-bit IEEE 754 floating-point format in little-endian byte order.

**Example: 433.175 MHz**
```
Decimal: 433.175
Hex: 0x43D847AE
Little-endian bytes: AE 47 D8 43
KISS frame: C0 06 01 AE 47 D8 43 C0
```

### Signed Integer Encoding

TX power uses signed 8-bit integer format (two's complement).

**Examples:**
- +14 dBm: `0x0E`
- -3 dBm: `0xFD` (253 decimal)

## Programming Examples

### Python Implementation

```python
import serial
import struct
import time

class KISSCommands:
    # Standard KISS commands
    DATA_FRAME = 0x00
    TX_DELAY = 0x01
    PERSISTENCE = 0x02
    SLOT_TIME = 0x03
    
    # Hardware commands
    SET_HARDWARE = 0x06
    SET_FREQUENCY = 0x01
    SET_TX_POWER = 0x02
    SET_BANDWIDTH = 0x03
    SET_SPREADING_FACTOR = 0x04
    SET_CODING_RATE = 0x05
    GET_CONFIG = 0x10

def escape_kiss_data(data):
    """Apply KISS byte stuffing to data."""
    data = data.replace(b'\xDB', b'\xDB\xDD')
    data = data.replace(b'\xC0', b'\xDB\xDC')
    return data

def send_kiss_command(ser, cmd, data=b''):
    """Send a KISS command with proper framing."""
    # Build frame
    frame_data = bytes([cmd]) + data
    frame_data = escape_kiss_data(frame_data)
    frame = b'\xC0' + frame_data + b'\xC0'
    
    # Send frame
    ser.write(frame)
    time.sleep(0.1)  # Allow processing time

# Example usage - adjust COM port as needed
try:
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)  # Linux/Mac
    # ser = serial.Serial('COM3', 115200, timeout=1)        # Windows
except serial.SerialException as e:
    print(f"Failed to open serial port: {e}")
    exit(1)

# Configure for 433 MHz band
freq_data = struct.pack('<f', 433.175)
send_kiss_command(ser, KISSCommands.SET_HARDWARE, 
                 bytes([KISSCommands.SET_FREQUENCY]) + freq_data)

# Set conservative parameters for long range
send_kiss_command(ser, KISSCommands.SET_HARDWARE,
                 bytes([KISSCommands.SET_SPREADING_FACTOR, 12]))
send_kiss_command(ser, KISSCommands.SET_HARDWARE,
                 bytes([KISSCommands.SET_CODING_RATE, 8]))

# Configure CSMA parameters
send_kiss_command(ser, KISSCommands.TX_DELAY, bytes([50]))  # 500ms
send_kiss_command(ser, KISSCommands.PERSISTENCE, bytes([31]))  # ~12%
send_kiss_command(ser, KISSCommands.SLOT_TIME, bytes([20]))  # 200ms

# Query configuration
send_kiss_command(ser, KISSCommands.SET_HARDWARE, 
                 bytes([KISSCommands.GET_CONFIG]))

ser.close()
```

### C++ Application Example

```cpp
#include <iostream>
#include <vector>
#include <cstring>

class KISSInterface {
private:
    std::vector<uint8_t> escapeData(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> escaped;
        for (uint8_t byte : data) {
            if (byte == 0xC0) {
                escaped.push_back(0xDB);
                escaped.push_back(0xDC);
            } else if (byte == 0xDB) {
                escaped.push_back(0xDB);
                escaped.push_back(0xDD);
            } else {
                escaped.push_back(byte);
            }
        }
        return escaped;
    }
    
public:
    std::vector<uint8_t> buildFrame(uint8_t cmd, const std::vector<uint8_t>& data = {}) {
        std::vector<uint8_t> frame_data = {cmd};
        frame_data.insert(frame_data.end(), data.begin(), data.end());
        
        std::vector<uint8_t> escaped = escapeData(frame_data);
        
        std::vector<uint8_t> frame = {0xC0};
        frame.insert(frame.end(), escaped.begin(), escaped.end());
        frame.push_back(0xC0);
        
        return frame;
    }
};

// Example: Set frequency to 868.1 MHz
KISSInterface kiss;
float freq = 868.1f;
std::vector<uint8_t> freq_data(sizeof(float));
memcpy(freq_data.data(), &freq, sizeof(float));

std::vector<uint8_t> cmd_data = {0x01};  // SET_FREQUENCY subcommand
cmd_data.insert(cmd_data.end(), freq_data.begin(), freq_data.end());

auto frame = kiss.buildFrame(0x06, cmd_data);  // SET_HARDWARE command
```

## Application Integration

### Direwolf APRS Software

Configuration for `direwolf.conf`:
```
# Serial interface (adjust port as needed)
CHANNEL 0
MYCALL N0CALL-1
MODEM 1200
PTT RIG 2 /dev/ttyUSB0

# KISS TCP interface
KISSPORT 8001

# KISS timing parameters  
TXDELAY 30
PERSIST 63
SLOTTIME 10

# Note: AGW interface not implemented
```

### APRS Client Applications

Most APRS applications support KISS over TCP. Configure with:
- **Host**: TNC IP address
- **Port**: 8001
- **Protocol**: TCP KISS

### Custom Applications

For custom packet radio applications:
1. Establish TCP connection to port 8001
2. Send configuration commands during initialization
3. Send data frames using KISS DATA_FRAME command (0x00)
4. Parse received frames by monitoring incoming KISS data frames

## Troubleshooting Commands

### Configuration Verification
Use GET_CONFIG command to verify current settings match expectations.

### Parameter Validation
The TNC validates all parameters and reports errors to the serial console. Invalid commands are ignored with diagnostic messages.

### Network Diagnostics
Use the web interface at `http://[tnc-ip]/status` for real-time monitoring of KISS activity and radio statistics.

## Performance Considerations

### Command Processing Speed
- Commands are processed immediately upon reception
- Configuration changes take effect for subsequent transmissions
- No queuing or buffering of configuration commands

### Memory Usage
- KISS buffers sized for maximum LoRa packet length
- Configuration stored in non-volatile memory
- Minimal RAM overhead for command processing

### Timing Accuracy
- 10ms timing resolution for KISS parameters
- Hardware timer-based implementation
- Jitter typically <1ms under normal conditions