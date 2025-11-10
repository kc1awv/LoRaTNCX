# 12. Applications and Integration

## Overview

LoRaTNCX integrates seamlessly with existing packet radio software through standard protocols. This section provides practical examples for connecting LoRaTNCX with popular applications including Dire Wolf, APRS clients, BPQ32, and custom software. All integrations use either serial KISS or TCP KISS connections.

## Dire Wolf Integration

Dire Wolf is a popular sound card TNC application that supports KISS protocol. LoRaTNCX can be used as a high-performance LoRa TNC with Dire Wolf.

### Basic Dire Wolf Configuration

**Create direwolf.conf:**
```ini
# Dire Wolf configuration for LoRaTNCX
# Use TCP KISS connection (recommended)
AGWPORT 8001
AGWPEER 192.168.4.1:8001

# Or use serial port
# PTT /dev/ttyACM0 RTS
# KISSPORT /dev/ttyACM0

# APRS configuration
MYCALL N0CALL-1
IGATE 1
TXDELAY 30
TXTAIL 10

# LoRa-specific settings (adjust for your region)
# 433.775 MHz APRS frequency (adjust for your region)
FREQ 433.775
MODEM 1200  # Dire Wolf will use this, but LoRaTNCX handles modulation

# Beacon configuration
PBEACON sendto=IG delay=0:30 every=10:00 symbol="igate" overlay="R" comment="LoRaTNCX iGate" via=WIDE1-1
```

**Start Dire Wolf:**
```bash
# Using TCP KISS (recommended)
direwolf -c direwolf.conf

# Or using serial port
direwolf -c direwolf.conf -p
```

### Dire Wolf with GNSS Integration

**Enhanced Configuration with GPS:**
```ini
# Dire Wolf with GPS support
AGWPORT 8001
AGWPEER 192.168.4.1:8001

# GPS configuration (connect to NMEA server)
GPSD localhost:2947

# Or direct NMEA connection
# GPSNMEA 192.168.4.1:10110

MYCALL N0CALL-1
IGATE 1

# Smart beaconing based on speed
SMARTBEACONING 30 3:00 5 00:15 30 00:45 255 02:00

# Position beaconing
PBEACON sendto=IG delay=0:30 every=10:00 symbol="igate" overlay="R" comment="LoRaTNCX iGate" via=WIDE1-1
```

### Dire Wolf APRS iGate Setup

**Complete iGate Configuration:**
```ini
AGWPORT 8001
AGWPEER 192.168.4.1:8001

MYCALL YOURCALL-10  # Your iGate callsign
IGATE 1
IGTXVIA 0           # Don't add via path to transmitted packets
IGRXVIA 0           # Don't add via path to received packets

# APRS-IS server configuration
IGLOGIN YOURCALL-10 YOURPASSCODE

# Filters for iGate operation
FILTER IG 0         # iGate filter - only gate packets that need gating
FILTER IG ONLY      # Only iGate, don't send to RF

# Logging
LOGDIR /var/log/direwolf
LOGFILE igate.log
```

## APRS Clients

### YAAC (Yet Another APRS Client)

**YAAC Configuration:**
1. **Add TNC Port:**
   - Port Type: AGWPE
   - Host: 192.168.4.1
   - Port: 8001
   - Callsign: YOURCALL-1

2. **Station Configuration:**
   - Callsign: YOURCALL-1
   - Symbol: /l (laptop)
   - Comment: LoRaTNCX

**YAAC Properties (yaac.properties):**
```properties
# LoRaTNCX TCP KISS configuration
agwpe.enabled=true
agwpe.host=192.168.4.1
agwpe.port=8001
agwpe.callsign=YOURCALL-1

# APRS-IS connectivity
aprsis.enabled=true
aprsis.host=rotate.aprs2.net
aprsis.port=14580
aprsis.callsign=YOURCALL-1
aprsis.passcode=YOURPASSCODE
```

### APRSDroid (Android)

**APRSdroid Configuration:**
1. **Connection Method:** TCP/IP (KISS)
2. **Host:** 192.168.4.1
3. **Port:** 8001
4. **Callsign:** YOURCALL-1

**Advanced Settings:**
- **Beaconing:** Smart beaconing enabled
- **Speed thresholds:** Adjust for LoRa range
- **Symbol:** /l (laptop)
- **Comment:** LoRaTNCX Mobile

### Xastir (Linux)

**Xastir Configuration:**
1. **Interface Configuration:**
   - Interface Type: KISS
   - Device: 192.168.4.1:8001 (TCP)
   - Callsign: YOURCALL-1

2. **Station Setup:**
   - Callsign: YOURCALL-1
   - Symbol: /l
   - Comment: LoRaTNCX

**xastir.cnf:**
```ini
[TNC-0]
device=192.168.4.1:8001
style=0  # 0=TCP KISS, 1=Serial KISS
```

## BPQ32 Configuration

BPQ32 is a Windows packet radio BBS system that supports multiple TNC connections.

### BPQ32 Basic Setup

**bpq32.cfg Configuration:**
```ini
; BPQ32 Configuration for LoRaTNCX
LOCATOR=EM00aa    ; Your grid square
NODECALL=YOURCALL-1
NODEALIAS=LORA
IDMSG:
LoRaTNCX BBS - LoRa Packet Radio
***

; Port configuration for LoRaTNCX
PORT
 PORTNUM=1
 ID=LoRaTNCX Port
 TYPE=EXTERNAL
 DLLNAME=KISSDLL.DLL
 QUALITY=192
 MINQUAL=120
 FRACK=7000
 RESPTIME=1000
 RETRIES=10
 PACLEN=236
 TXDELAY=300
 TXTAIL=100
ENDPORT

; KISS configuration
[KISS]
; TCP KISS connection
HOST=192.168.4.1
PORT=8001
END[KISS]
```

### BPQ32 with Multiple Ports

**Advanced Configuration:**
```ini
; Multiple port configuration
LOCATOR=EM00aa
NODECALL=YOURCALL-1

; LoRa Port
PORT
 PORTNUM=1
 ID=LoRa VHF
 TYPE=EXTERNAL
 DLLNAME=KISSDLL.DLL
 QUALITY=192
 MINQUAL=120
 FRACK=7000
 RESPTIME=1000
 RETRIES=10
 PACLEN=236
 TXDELAY=300
 TXTAIL=100
ENDPORT

; HF Port (if available)
PORT
 PORTNUM=2
 ID=LoRa HF
 TYPE=EXTERNAL
 DLLNAME=KISSDLL.DLL
 QUALITY=100
 MINQUAL=80
 FRACK=15000
 RESPTIME=2000
 RETRIES=15
 PACLEN=100
ENDPORT

; Application definitions
APPLICATION 1,BBS,,YOURCALL-1
APPLICATION 2,CHAT,,YOURCALL-1
APPLICATION 3,DXC,,YOURCALL-1

[KISS]
HOST=192.168.4.1
PORT=8001
END[KISS]
```

## Packet Radio Software

### LinBPQ (Linux BPQ32)

**LinBPQ Configuration:**
```ini
LOCATOR=EM00aa
NODECALL=YOURCALL-1

PORT
 PORTNUM=1
 ID=LoRaTNCX
 TYPE=EXTERNAL
 DLLNAME=/usr/local/lib/linbpq/kiss.so
 QUALITY=192
 MINQUAL=120
 FRACK=7000
 RESPTIME=1000
 RETRIES=10
 PACLEN=236
 TXDELAY=300
 TXTAIL=100
ENDPORT

[KISS]
HOST=192.168.4.1
PORT=8001
END[KISS]
```

### Linux AX.25 Stack

**AX.25 Configuration:**
```bash
# Configure AX.25 port
sudo kissattach /dev/ttyACM0 ax0 44.131.1.1

# Or for TCP KISS (requires socat)
socat TCP:192.168.4.1:8001 PTY,link=/dev/ttyKISS &
sudo kissattach /dev/ttyKISS ax0 44.131.1.1

# Configure interface
sudo ifconfig ax0 44.131.1.1 netmask 255.255.255.0 up

# Add route
sudo route add -net 44.131.1.0 netmask 255.255.255.0 dev ax0
```

**ax25d.conf for BBS:**
```ini
# AX.25 daemon configuration
[YOURCALL-1 VIA ax0]
NOCALL   * * * * * *  L
default  * * * * * *  - root /usr/sbin/node node
```

### Winlink Express

**Winlink Configuration:**
1. **Add Packet TNC:**
   - TNC Type: KISS
   - Serial Port: TCP connection to 192.168.4.1:8001
   - Callsign: YOURCALL-1

2. **RMS Express Setup:**
   - Configure for packet radio
   - Set appropriate frequencies for your region
   - Enable compression for LoRa bandwidth efficiency

## Custom Applications

### Python KISS Client

**Basic KISS Client:**
```python
import socket
import time
import struct

class LoRaTNCXClient:
    def __init__(self, host='192.168.4.1', port=8001):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False
        
    def connect(self):
        """Connect to LoRaTNCX TCP KISS server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            self.connected = True
            print(f"Connected to LoRaTNCX at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from server"""
        if self.sock:
            self.sock.close()
        self.connected = False
    
    def send_kiss_frame(self, data):
        """Send data as KISS frame"""
        if not self.connected:
            return False
            
        # KISS frame: FEND + data + FEND
        FEND = 0xC0
        frame = bytes([FEND]) + data + bytes([FEND])
        
        try:
            self.sock.send(frame)
            return True
        except Exception as e:
            print(f"Send failed: {e}")
            return False
    
    def receive_kiss_frame(self, timeout=1.0):
        """Receive KISS frame"""
        if not self.connected:
            return None
            
        self.sock.settimeout(timeout)
        try:
            data = self.sock.recv(1024)
            if data:
                # Strip FEND bytes and return payload
                if len(data) >= 2 and data[0] == 0xC0 and data[-1] == 0xC0:
                    return data[1:-1]
            return data
        except socket.timeout:
            return None
        except Exception as e:
            print(f"Receive failed: {e}")
            return None

# Usage example
if __name__ == "__main__":
    client = LoRaTNCXClient()
    
    if client.connect():
        # Send APRS packet
        aprs_packet = b"N0CALL>APRS:>Hello from LoRaTNCX!"
        client.send_kiss_frame(aprs_packet)
        
        # Listen for packets
        while True:
            packet = client.receive_kiss_frame()
            if packet:
                print(f"Received: {packet}")
            time.sleep(0.1)
    
    client.disconnect()
```

### Node.js KISS Client

**Node.js Implementation:**
```javascript
const net = require('net');

class LoRaTNCXClient {
    constructor(host = '192.168.4.1', port = 8001) {
        this.host = host;
        this.port = port;
        this.client = null;
        this.connected = false;
    }
    
    connect() {
        return new Promise((resolve, reject) => {
            this.client = net.createConnection({
                host: this.host,
                port: this.port
            }, () => {
                this.connected = true;
                console.log(`Connected to LoRaTNCX at ${this.host}:${this.port}`);
                resolve(true);
            });
            
            this.client.on('error', (err) => {
                console.error('Connection error:', err);
                reject(err);
            });
            
            this.client.on('data', (data) => {
                this.handleData(data);
            });
            
            this.client.on('close', () => {
                this.connected = false;
                console.log('Connection closed');
            });
        });
    }
    
    sendKISSFrame(data) {
        if (!this.connected) return false;
        
        const FEND = 0xC0;
        const frame = Buffer.concat([
            Buffer.from([FEND]),
            Buffer.from(data),
            Buffer.from([FEND])
        ]);
        
        this.client.write(frame);
        return true;
    }
    
    handleData(data) {
        // Process incoming KISS frames
        console.log('Received data:', data);
        
        // Strip FEND bytes if present
        if (data[0] === 0xC0 && data[data.length - 1] === 0xC0) {
            const payload = data.slice(1, -1);
            console.log('KISS payload:', payload.toString());
        }
    }
    
    disconnect() {
        if (this.client) {
            this.client.end();
        }
        this.connected = false;
    }
}

// Usage
const client = new LoRaTNCXClient();

client.connect()
    .then(() => {
        // Send APRS beacon
        const aprsBeacon = "N0CALL>APRS:>LoRaTNCX Test Beacon";
        client.sendKISSFrame(Buffer.from(aprsBeacon));
        
        // Keep connection alive
        setInterval(() => {
            console.log('Connection active...');
        }, 30000);
    })
    .catch(console.error);
```

### APRS Position Reporter

**Python APRS Gateway with GNSS:**
```python
import socket
import threading
import time
import aprslib

class APRSGateway:
    def __init__(self, tnc_host='192.168.4.1', tnc_port=8001, 
                 nmea_host='192.168.4.1', nmea_port=10110):
        self.tnc_host = tnc_host
        self.tnc_port = tnc_port
        self.nmea_host = nmea_host
        self.nmea_port = nmea_port
        
        # APRS-IS connection
        self.aprs_is = aprslib.IS('N0CALL', passwd='12345', 
                                host='rotate.aprs2.net', port=14580)
        
        # TNC connection
        self.tnc_sock = None
        
        # Position tracking
        self.last_position = None
        
    def connect_tnc(self):
        """Connect to LoRaTNCX KISS server"""
        try:
            self.tnc_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.tnc_sock.connect((self.tnc_host, self.tnc_port))
            print("Connected to LoRaTNCX TNC")
            return True
        except Exception as e:
            print(f"TNC connection failed: {e}")
            return False
    
    def connect_nmea(self):
        """Connect to NMEA server for position data"""
        try:
            self.nmea_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.nmea_sock.connect((self.nmea_host, self.nmea_port))
            print("Connected to NMEA server")
            
            # Start NMEA processing thread
            threading.Thread(target=self.process_nmea, daemon=True).start()
            return True
        except Exception as e:
            print(f"NMEA connection failed: {e}")
            return False
    
    def process_nmea(self):
        """Process NMEA sentences for position updates"""
        buffer = ""
        while True:
            try:
                data = self.nmea_sock.recv(1024).decode('ascii', errors='ignore')
                buffer += data
                
                # Process complete sentences
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    if line.startswith('$GNRMC') or line.startswith('$GPRMC'):
                        # Parse position from RMC sentence
                        self.parse_position(line)
                        
            except Exception as e:
                print(f"NMEA error: {e}")
                time.sleep(1)
    
    def parse_position(self, rmc_sentence):
        """Parse position from RMC sentence"""
        parts = rmc_sentence.split(',')
        if len(parts) >= 7:
            try:
                lat = parts[3]
                lat_dir = parts[4]
                lon = parts[5] 
                lon_dir = parts[6]
                
                if lat and lon:
                    # Convert to decimal degrees
                    lat_dd = self.nmea_to_decimal(lat, lat_dir)
                    lon_dd = self.nmea_to_decimal(lon, lon_dir)
                    
                    self.last_position = (lat_dd, lon_dd)
                    print(f"Position updated: {lat_dd}, {lon_dd}")
                    
            except (ValueError, IndexError):
                pass
    
    def nmea_to_decimal(self, nmea_coord, direction):
        """Convert NMEA coordinate to decimal degrees"""
        # DDMM.MMMM format to decimal degrees
        degrees = float(nmea_coord[:2])
        minutes = float(nmea_coord[2:])
        decimal = degrees + minutes / 60.0
        
        if direction in ['S', 'W']:
            decimal = -decimal
            
        return decimal
    
    def send_aprs_position(self):
        """Send APRS position report"""
        if self.last_position and self.tnc_sock:
            lat, lon = self.last_position
            
            # Create APRS position report
            position = f"{lat:.4f},{lon:.4f}"
            aprs_packet = f"N0CALL>APRS:@123456z{position} LoRaTNCX Mobile"
            
            # Send via KISS
            kiss_frame = b'\xC0' + aprs_packet.encode() + b'\xC0'
            self.tnc_sock.send(kiss_frame)
            
            print(f"Sent APRS position: {aprs_packet}")
    
    def run(self):
        """Main gateway loop"""
        if not self.connect_tnc() or not self.connect_nmea():
            return
        
        # APRS-IS connection
        try:
            self.aprs_is.connect()
        except Exception as e:
            print(f"APRS-IS connection failed: {e}")
            return
        
        print("APRS Gateway running...")
        
        # Send position updates every 5 minutes
        while True:
            self.send_aprs_position()
            time.sleep(300)

if __name__ == "__main__":
    gateway = APRSGateway()
    gateway.run()
```

## Integration Best Practices

### Connection Management

**TCP vs Serial:**
- **TCP KISS:** Preferred for network-based applications, multiple clients supported
- **Serial KISS:** Direct hardware connection, lower latency, single client

**Connection Stability:**
- Implement reconnection logic for network interruptions
- Monitor connection health with periodic pings
- Handle connection drops gracefully

### Performance Optimization

**Frame Size:**
- Maximum KISS frame size: 236 bytes (BPQ32 default)
- Adjust PACLEN based on LoRa bandwidth and SF
  - Higher SF = lower data rate = smaller PACLEN
  - Lower SF = higher data rate = larger PACLEN
- Consider compression for text-heavy applications

### Error Handling

**Common Issues:**
- **Connection timeouts:** Increase FRACK for long-distance links
- **Frame corruption:** Implement CRC checking in applications
- **Buffer overflows:** Monitor packet sizes and implement flow control

**Debugging:**
- Enable verbose logging in applications
- Monitor LoRaTNCX web interface for radio statistics
- Use packet sniffers to analyze KISS traffic

### Security Considerations

**Network Security:**
- Use firewall rules to restrict KISS port access
- Consider VPN for remote TNC access
- Implement authentication for critical applications

**RF Security:**
- Use unique sync words for private networks
- Implement frequency hopping for sensitive communications
- Monitor for unauthorized transmissions

---

**Next:** [Section 13: Testing and Validation](13-testing-validation.md) | **Previous:** [Section 11: GNSS Support](11-gnss-support.md) | **Main Index:** [User Manual](../user_manual.md)