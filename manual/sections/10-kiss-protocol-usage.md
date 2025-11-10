# 10. KISS Protocol Usage

## Overview

LoRaTNCX implements the KISS (Keep It Simple, Stupid) protocol for packet radio communication. KISS provides a simple framing mechanism for exchanging data packets between the TNC and host applications. The protocol is widely used in amateur packet radio and supports both serial and network connections.

## Connecting via Serial

### Serial Port Configuration

**Default Settings:**
- **Baud Rate:** 115200 bps
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None

**Platform-Specific Connection:**

**Linux/macOS:**
```bash
# List available serial ports
ls /dev/tty*

# Connect using screen
screen /dev/ttyACM0 115200

# Or using minicom
minicom -b 115200 -o -D /dev/ttyACM0
```

**Windows:**
```cmd
# Using PuTTY
# Serial line: COM3
# Speed: 115200
# Data bits: 8
# Stop bits: 1
# Parity: None
# Flow control: None

# Using PowerShell
# (Requires serial terminal program)
```

**Python (Cross-Platform):**
```python
import serial
import time

# Open serial connection
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
time.sleep(2)  # Allow device to initialize

# Connection is now ready for KISS communication
```

### Serial Connection States

**Initialization:**
- Device sends no data until first valid KISS frame
- Serial buffer is flushed on connection
- Ready for KISS commands immediately

**Active Communication:**
- Bidirectional data exchange
- No keepalive packets required
- Connection remains active until disconnected

## TCP KISS Server

### Server Configuration

**Default Settings:**
- **Port:** 8001
- **Protocol:** Raw TCP
- **Max Clients:** 4 simultaneous connections
- **Enabled by Default:** Yes

**Server Control:**
- Configure via web interface (Network tab)
- Enable/disable TCP KISS server
- Change port number (requires reboot)
- Monitor connected clients

### Connecting to TCP Server

**Using Netcat:**
```bash
# Connect to TCP KISS server
nc 192.168.4.1 8001

# Or with timeout
timeout 30 nc 192.168.4.1 8001
```

**Using Telnet:**
```bash
telnet 192.168.4.1 8001
```

**Using Python:**
```python
import socket

# Connect to TCP KISS server
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.4.1', 8001))

print("Connected to LoRaTNCX TCP KISS server")
# Now ready to send/receive KISS frames
```

**Using JavaScript/Node.js:**
```javascript
const net = require('net');

const client = net.createConnection({ port: 8001, host: '192.168.4.1' }, () => {
    console.log('Connected to LoRaTNCX TCP KISS server');
    // Ready for KISS communication
});

client.on('data', (data) => {
    console.log('Received:', data);
});

client.on('close', () => {
    console.log('Connection closed');
});
```

### Multiple Client Support

- **Concurrent Connections:** Up to 4 clients simultaneously
- **Broadcast Reception:** All clients receive incoming LoRa packets
- **Individual Transmission:** Any client can transmit packets
- **No Client Isolation:** All clients share the same radio interface

## Data Frame Format

### KISS Frame Structure

KISS frames use byte stuffing to embed control characters in data streams:

**Frame Format:**
```
FEND + Command/Data + FEND
```

**Control Characters:**
- **FEND (0xC0):** Frame boundary marker
- **FESC (0xDB):** Escape character
- **TFEND (0xDC):** Escaped FEND
- **TFESC (0xDD):** Escaped FESC

### Byte Stuffing Rules

**Data Transmission:**
1. If data contains FEND (0xC0) → Replace with FESC + TFEND
2. If data contains FESC (0xDB) → Replace with FESC + TFESC
3. Other bytes pass through unchanged

**Data Reception:**
1. If FESC + TFEND → Replace with FEND
2. If FESC + TFESC → Replace with FESC
3. Other bytes pass through unchanged

### Frame Types

**Data Frames (CMD_DATA = 0x00):**
```
FEND + 0x00 + [escaped packet data] + FEND
```
Used for transmitting and receiving packet radio data.

**Command Frames:**
```
FEND + Command + [escaped command data] + FEND
```
Used for configuration and status queries.

## Hardware Commands

### SETHARDWARE Commands (Configuration)

All SETHARDWARE commands use the format:
```
FEND + 0x06 + Subcommand + [Data] + FEND
```

**Set Frequency (0x01):**
- **Data:** 4 bytes (float, little-endian)
- **Range:** 150.0 - 960.0 MHz
- **Example:** Set to 915.0 MHz
  ```
  FEND + 0x06 + 0x01 + [0x00, 0x00, 0x3C, 0x44] + FEND
  ```

**Set Bandwidth (0x02):**
- **Data:** 4 bytes (float, little-endian)
- **Values:** 125.0, 250.0, 500.0 kHz
- **Example:** Set to 125 kHz
  ```
  FEND + 0x06 + 0x02 + [0x00, 0x00, 0x00, 0x42] + FEND
  ```

**Set Spreading Factor (0x03):**
- **Data:** 1 byte
- **Range:** 7-12
- **Example:** Set to SF12
  ```
  FEND + 0x06 + 0x03 + 0x0C + FEND
  ```

**Set Coding Rate (0x04):**
- **Data:** 1 byte
- **Range:** 5-8 (representing 4/5 to 4/8)
- **Example:** Set to 4/7
  ```
  FEND + 0x06 + 0x04 + 0x07 + FEND
  ```

**Set Power (0x05):**
- **Data:** 1 byte (signed int)
- **Range:** -9 to +22 dBm
- **Example:** Set to 20 dBm
  ```
  FEND + 0x06 + 0x05 + 0x14 + FEND
  ```

**Set Sync Word (0x08):**
- **Data:** 2 bytes (big-endian)
- **Range:** 0x0000-0xFFFF
- **Example:** Set to 0x1424
  ```
  FEND + 0x06 + 0x08 + [0x14, 0x24] + FEND
  ```

**Set GNSS Enable (0x09):**
- **Data:** 1 byte (0=disable, 1=enable)
- **Example:** Enable GNSS
  ```
  FEND + 0x06 + 0x09 + 0x01 + FEND
  ```

**Save Configuration (0x07):**
- **Data:** None
- **Example:** Save current settings
  ```
  FEND + 0x06 + 0x07 + FEND
  ```

**Reset Configuration (0xFF):**
- **Data:** None
- **Example:** Reset to factory defaults
  ```
  FEND + 0x06 + 0xFF + FEND
  ```

### GETHARDWARE Commands (Status Queries)

All GETHARDWARE commands use the format:
```
FEND + 0x07 + Subcommand + [Data] + FEND
```

**Query Configuration (0x01):**
- **Response:** Current LoRa radio settings
- **Format:** Binary data with frequency, bandwidth, etc.
- **Example Query:**
  ```
  FEND + 0x07 + 0x01 + FEND
  ```

**Query Battery (0x02):**
- **Response:** Battery voltage (float)
- **Format:** 4 bytes (little-endian float)
- **Example Query:**
  ```
  FEND + 0x07 + 0x02 + FEND
  ```

**Query Board (0x03):**
- **Response:** Board type and name
- **Format:** Board type byte + ASCII name string
- **Example Query:**
  ```
  FEND + 0x07 + 0x03 + FEND
  ```

**Query GNSS (0x04):**
- **Response:** GNSS status and position
- **Format:** Binary data with fix status, satellites, coordinates
- **Example Query:**
  ```
  FEND + 0x07 + 0x04 + FEND
  ```

**Query All (0xFF):**
- **Response:** Complete system status
- **Format:** Multiple frames with all information
- **Example Query:**
  ```
  FEND + 0x07 + 0xFF + FEND
  ```

## Integration with Applications

### Dire Wolf Integration

Dire Wolf is a popular packet radio software modem that supports KISS TNC connections.

**Configuration (direwolf.conf):**
```ini
# KISS TNC Configuration
AGWPORT 8001
AGWPEER 192.168.4.1:8001

# Or for serial connection
# PTT /dev/ttyACM0 RTS
# KISSPORT /dev/ttyACM0

# Radio parameters (let LoRaTNCX handle these)
# Let the TNC handle modulation parameters
```

**Running Dire Wolf:**
```bash
# Connect via TCP
direwolf -c direwolf.conf

# Or via serial
direwolf -c direwolf-serial.conf
```

### APRS Applications

**APRS-IS Gateway:**
```python
import socket
import kiss

# Connect to LoRaTNCX
tnc = kiss.KISS(host='192.168.4.1', port=8001)
tnc.start()

# Connect to APRS-IS
aprs_is = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
aprs_is.connect(('rotate.aprs2.net', 14580))
aprs_is.send(b'user YOURCALL pass YOURPASS vers LoRaTNCX 1.0\r\n')

# Bridge packets between LoRa and APRS-IS
while True:
    # Receive from LoRa
    packet = tnc.read()
    if packet:
        # Forward to APRS-IS
        aprs_is.send(packet + b'\r\n')
    
    # Receive from APRS-IS
    data = aprs_is.recv(1024)
    if data:
        # Forward to LoRa
        tnc.write(data.strip())
```

### Custom Packet Radio Applications

**Python KISS Library:**
```python
import kiss
import time

class LoRaTNC:
    def __init__(self, host='192.168.4.1', port=8001):
        self.kiss = kiss.KISS(host=host, port=port)
        self.kiss.start()
    
    def send_packet(self, data):
        """Send a packet via LoRa"""
        self.kiss.write(data)
        print(f"Sent packet: {data.hex()}")
    
    def receive_packets(self):
        """Receive packets from LoRa"""
        while True:
            packet = self.kiss.read()
            if packet:
                print(f"Received packet: {packet.hex()}")
                # Process packet here
            time.sleep(0.1)

# Usage
tnc = LoRaTNC()
tnc.send_packet(b'Hello LoRa World!')
tnc.receive_packets()
```

**JavaScript/Node.js Implementation:**
```javascript
const net = require('net');
const EventEmitter = require('events');

class LoRaTNC extends EventEmitter {
    constructor(host = '192.168.4.1', port = 8001) {
        super();
        this.host = host;
        this.port = port;
        this.socket = null;
        this.buffer = Buffer.alloc(0);
    }
    
    connect() {
        this.socket = net.createConnection({
            host: this.host,
            port: this.port
        });
        
        this.socket.on('connect', () => {
            console.log('Connected to LoRaTNCX');
            this.emit('connected');
        });
        
        this.socket.on('data', (data) => {
            this.buffer = Buffer.concat([this.buffer, data]);
            this.processFrames();
        });
        
        this.socket.on('close', () => {
            console.log('Disconnected from LoRaTNCX');
            this.emit('disconnected');
        });
        
        this.socket.on('error', (err) => {
            console.error('Connection error:', err);
            this.emit('error', err);
        });
    }
    
    sendFrame(frame) {
        if (this.socket && this.socket.writable) {
            this.socket.write(frame);
        }
    }
    
    processFrames() {
        // KISS frame processing logic here
        // Parse FEND-delimited frames
        // Handle escape sequences
        // Emit 'packet' events for data frames
    }
}

// Usage
const tnc = new LoRaTNC();
tnc.connect();

tnc.on('connected', () => {
    console.log('Ready to send/receive packets');
});

tnc.on('packet', (packet) => {
    console.log('Received packet:', packet.toString('hex'));
});
```

### AX.25 Packet Construction

**Basic AX.25 Packet:**
```python
def create_ax25_packet(source_call, dest_call, data):
    """
    Create a basic AX.25 packet for LoRa transmission
    """
    # AX.25 header (simplified)
    header = b''
    
    # Destination callsign (6 bytes + SSID)
    dest_padded = dest_call.ljust(6, b' ').upper()
    header += dest_padded + b'\x40'  # SSID byte
    
    # Source callsign (6 bytes + SSID)
    src_padded = source_call.ljust(6, b' ').upper()
    header += src_padded + b'\x40'  # SSID byte
    
    # Control and PID bytes
    header += b'\x03\xf0'  # UI frame
    
    # Packet data
    packet = header + data
    
    return packet

# Example usage
packet = create_ax25_packet('N0CALL', 'CQ', b'Hello from LoRa!')
# Send via KISS interface
```

## Troubleshooting

### Serial Connection Issues

**No Response from Device:**
- Check serial port and baud rate
- Verify device is powered on
- Try different USB cable/port
- Check device manager (Windows) or dmesg (Linux)

**Garbled Data:**
- Verify baud rate (115200)
- Check serial port settings
- Ensure no flow control
- Try different terminal program

**Permission Errors:**
```bash
# Linux: Add user to dialout group
sudo usermod -a -G dialout $USER

# Then logout and login again, or:
newgrp dialout
```

### TCP Connection Issues

**Connection Refused:**
- Verify TCP KISS server is enabled
- Check port number (default 8001)
- Ensure device is on same network
- Check firewall settings

**Connection Drops:**
- Check WiFi signal strength
- Verify device hasn't rebooted
- Monitor device logs
- Try different client application

### KISS Protocol Issues

**Invalid Frame Errors:**
- Check byte stuffing implementation
- Verify FEND/FESC handling
- Ensure proper frame boundaries
- Debug with packet sniffer

**No Data Reception:**
- Verify antenna connections
- Check LoRa parameters (frequency, bandwidth, etc.)
- Ensure devices use same sync word
- Test with known working setup

**Transmission Failures:**
- Check transmit power settings
- Verify antenna SWR
- Monitor duty cycle limits
- Check for interference

### Performance Optimization

**Latency Considerations:**
- Serial: Low latency, direct connection
- TCP: Slight latency due to network stack
- Buffer sizes affect responsiveness

**Throughput Limits:**
- LoRa data rates: 293 bps (SF12) to 5469 bps (SF7)
- Serial baud rate: 115200 bps (not limiting)
- TCP overhead: Minimal impact

**Error Handling:**
- Implement retry logic for failed transmissions
- Monitor RSSI for link quality
- Use appropriate spreading factors for range/speed balance

## Advanced Usage

### Custom KISS Implementations

**Low-Level Frame Processing:**
```c
// Example C implementation of KISS frame processing
#define FEND  0xC0
#define FESC  0xDB
#define TFEND 0xDC
#define TFESC 0xDD

void process_kiss_byte(uint8_t byte, kiss_state_t* state) {
    switch (state->phase) {
        case KISS_OUTSIDE_FRAME:
            if (byte == FEND) {
                state->phase = KISS_IN_FRAME;
                state->buffer_index = 0;
            }
            break;
            
        case KISS_IN_FRAME:
            if (byte == FEND) {
                if (state->buffer_index > 0) {
                    // Complete frame received
                    process_frame(state->buffer, state->buffer_index);
                    state->phase = KISS_OUTSIDE_FRAME;
                }
            } else if (byte == FESC) {
                state->phase = KISS_ESCAPE_PENDING;
            } else {
                if (state->buffer_index < BUFFER_SIZE) {
                    state->buffer[state->buffer_index++] = byte;
                }
            }
            break;
            
        case KISS_ESCAPE_PENDING:
            state->phase = KISS_IN_FRAME;
            if (byte == TFEND) {
                state->buffer[state->buffer_index++] = FEND;
            } else if (byte == TFESC) {
                state->buffer[state->buffer_index++] = FESC;
            } else {
                // Invalid escape sequence
                state->buffer[state->buffer_index++] = byte;
            }
            break;
    }
}
```

### Protocol Extensions

**LoRa-Specific Commands:**
- Hardware configuration commands extend standard KISS
- GNSS integration commands for position data
- Battery monitoring for remote installations

**Future Extensions:**
- Quality of Service (QoS) parameters
- Channel access coordination
- Mesh networking commands

---

**Next:** [Section 11: GNSS Support](11-gnss-support.md) | **Previous:** [Section 9: Web Interface](09-web-interface.md) | **Main Index:** [User Manual](../user_manual.md)