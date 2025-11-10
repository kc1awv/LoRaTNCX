# 11. GNSS Support

## Overview

LoRaTNCX provides comprehensive GNSS (Global Navigation Satellite System) support for position-aware packet radio applications. The system supports multi-constellation receivers including GPS, GLONASS, BeiDou, and QZSS through dedicated GNSS modules. GNSS data is made available via NMEA protocol over TCP for integration with external applications.

**Important Note:** LoRaTNCX provides GNSS data streams but does not include built-in APRS functionality. APRS position reporting and messaging must be handled by external applications that connect to the NMEA TCP server.

## Hardware Requirements

### Supported Boards

**Heltec WiFi LoRa 32 V4:**
- Built-in GNSS module (u-blox or compatible)
- Dedicated GNSS pins with power control
- Integrated GNSS antenna
- Default configuration available

**Heltec WiFi LoRa 32 V3:**
- No built-in GNSS module
- External GNSS module required
- User-configurable pins
- Manual antenna connection needed

## GNSS Configuration

### Enabling GNSS

**Via Web Interface:**
1. Connect to LoRaTNCX web interface
2. Navigate to GNSS tab
3. Check "Enable GNSS" checkbox
4. Configure pin settings (V3 boards only)
5. Set TCP port (default 10110)
6. Save configuration

**Via REST API:**
```json
POST /api/gnss/config
{
  "enabled": true,
  "tcpPort": 10110
}
```

### Pin Configuration (V3 Boards)

For V3 boards without built-in GNSS, configure the following pins:

**Required Pins:**
- **RX Pin:** GNSS TX → MCU RX (data input)
- **TX Pin:** MCU TX → GNSS RX (data output)

**Optional Pins:**
- **Control Pin:** Power control (active LOW)
- **Wake Pin:** Wake-up signal
- **PPS Pin:** Pulse per second output
- **Reset Pin:** Module reset (active LOW)

**Default V3 Pin Suggestions:**
```json
{
  "pinRX": 39,
  "pinTX": 38,
  "pinCtrl": 34,
  "pinWake": -1,
  "pinPPS": -1,
  "pinRST": -1
}
```

### V4 Board Configuration

V4 boards have fixed GNSS pin assignments:

- **RX:** GPIO39 (GNSS TX → MCU RX)
- **TX:** GPIO38 (MCU TX → GNSS RX)
- **Control:** GPIO34 (power control, active LOW)
- **Wake:** GPIO40
- **PPS:** GPIO41
- **Reset:** GPIO42

No pin configuration needed - automatically detected.

## NMEA over TCP

### TCP Server Configuration

**Default Settings:**
- **Port:** 10110 (standard NMEA-over-TCP port)
- **Max Clients:** 4 simultaneous connections
- **Protocol:** Raw TCP with NMEA sentences
- **Buffering:** Real-time sentence forwarding

**Server Control:**
- Automatically starts when GNSS is enabled
- Runs on same IP as web interface
- No authentication required
- Survives WiFi mode changes

### Connecting to NMEA Server

**Using Netcat:**
```bash
# Connect and monitor NMEA data
nc 192.168.4.1 10110
```

**Using Telnet:**
```bash
telnet 192.168.4.1 10110
```

**Using Python:**
```python
import socket
import time

# Connect to NMEA server
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.4.1', 10110))

print("Connected to NMEA server")

# Read NMEA sentences
buffer = ""
while True:
    data = sock.recv(1024).decode('ascii', errors='ignore')
    buffer += data
    
    # Process complete sentences
    while '\n' in buffer:
        line, buffer = buffer.split('\n', 1)
        if line.startswith('$'):
            print(f"NMEA: {line.strip()}")
    
    time.sleep(0.1)
```

**Using Node.js:**
```javascript
const net = require('net');

const client = net.createConnection({
    port: 10110,
    host: '192.168.4.1'
}, () => {
    console.log('Connected to NMEA server');
});

client.on('data', (data) => {
    const sentences = data.toString().split('\n');
    sentences.forEach(sentence => {
        if (sentence.startsWith('$')) {
            console.log('NMEA:', sentence.trim());
        }
    });
});

client.on('close', () => {
    console.log('NMEA connection closed');
});
```

### NMEA Sentence Types

LoRaTNCX forwards all standard NMEA sentences from the GNSS module. Modern multi-constellation receivers may use different talker IDs:

**Position and Time:**
- **$GNGGA:** Multi-constellation fix data (position, time, satellites)
- **$GNRMC:** Multi-constellation recommended minimum navigation information
- **$GNGLL:** Multi-constellation geographic position, latitude/longitude
- **$GNVTG:** Multi-constellation track made good and ground speed
- **$GNZDA:** Multi-constellation time and date

**Satellite Information:**
- **$GPGSV:** GPS satellites in view
- **$GLGSV:** GLONASS satellites in view
- **$BDGSV:** BeiDou satellites in view
- **$GQGSV:** QZSS satellites in view

**Accuracy and Status:**
- **$GNGSA:** Multi-constellation DOP and active satellites
- **$GNGST:** Multi-constellation pseudorange noise statistics

**System Information:**
- **$GPTXT:** Text messages from GNSS receiver (antenna status, warnings)

### NMEA Data Format

**Example NMEA Sentences (No Fix Condition):**
```
$GNGGA,,,,,,0,00,25.5,,,,,,*64
$GNGLL,,,,,,V,N*7A
$GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,1*01
$GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,4*04
$GPGSV,1,1,00,0*65
$BDGSV,1,1,00,0*74
$GNRMC,,V,,,,,,,,,,N,V*37
$GNVTG,,,,,,,,,N*2E
$GNZDA,,,,,,*56
$GPTXT,01,01,01,ANTENNA OPEN*25
```

**Example NMEA Sentences (With Fix):**
```
$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
$GNGLL,4807.038,N,01131.000,E,123519,A,A*5C
$GNVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
```

**Sentence Structure:**
- **$** - Sentence start delimiter
- **GN/GP/GL/BD/GQ** - Talker ID (GN = multi-constellation, GP = GPS, GL = GLONASS, BD = BeiDou, GQ = QZSS)
- **GGA** - Sentence type
- **,** - Field separator
- **\*** - Checksum delimiter
- **47** - Checksum (hex)

## GNSS Status Monitoring

### Web Interface Status

The GNSS tab displays real-time status information:

**Fix Status:**
- **No Fix:** Not receiving satellite signals
- **2D Fix:** Position only (latitude/longitude)
- **3D Fix:** Position with altitude

**Satellite Information:**
- **Satellites Used:** Number of satellites in fix calculation
- **Satellites Visible:** Total satellites detected

**Position Data:**
- **Latitude/Longitude:** Current position (when available)
- **Altitude:** Height above sea level
- **HDOP:** Horizontal dilution of precision

**Time Information:**
- **UTC Time:** Current time from satellites
- **Date:** Current date

### REST API Status

**Get GNSS Status:**
```http
GET /api/gnss/status
```

**Response:**
```json
{
  "enabled": true,
  "hasFix": true,
  "latitude": 40.7128,
  "longitude": -74.0060,
  "altitude": 10.5,
  "satellites": 8,
  "hdop": 1.2,
  "speed": 0.0,
  "course": 0.0,
  "time": {
    "hour": 14,
    "minute": 30,
    "second": 45
  },
  "date": {
    "day": 15,
    "month": 10,
    "year": 2024
  }
}
```

### Serial Passthrough

GNSS data is also available via serial port for direct connection:

**Serial Configuration:**
- Same baud rate as GNSS module (default 9600)
- Raw NMEA data stream
- No KISS framing
- Available when GNSS enabled

**Accessing Serial GNSS:**
```bash
# Monitor GNSS data on serial port
# (Requires second serial connection or serial multiplexer)
screen /dev/ttyACM0 9600
```

## External APRS Integration

**Important:** LoRaTNCX does not include built-in APRS functionality. Position reporting and APRS messaging must be implemented using external applications.

### APRS Software Integration

**Using Dire Wolf with GPS:**
```bash
# Dire Wolf configuration for APRS with GPS
# direwolf.conf
AGWPORT 8001
AGWPEER 192.168.4.1:8001

# GPS configuration
GPSD localhost:2947  # If using gpsd
# Or direct NMEA
# GPSNMEA 192.168.4.1:10110

# APRS beacon configuration
PBEACON sendto=IG delay=0:30 every=10:00 symbol="igate" overlay="R" comment="LoRaTNCX iGate" via=WIDE1-1
```

**Python APRS Gateway:**
```python
import socket
import aprslib
import threading
import time

class APRSGateway:
    def __init__(self, tnc_host='192.168.4.1', tnc_port=8001, nmea_host='192.168.4.1', nmea_port=10110):
        self.tnc_host = tnc_host
        self.tnc_port = tnc_port
        self.nmea_host = nmea_host
        self.nmea_port = nmea_port
        
        # APRS-IS connection
        self.aprs_is = aprslib.IS('N0CALL', passwd='12345', host='rotate.aprs2.net', port=14580)
        
        # Position tracking
        self.last_position = None
        
    def connect_nmea(self):
        """Connect to NMEA server for position data"""
        self.nmea_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.nmea_sock.connect((self.nmea_host, self.nmea_port))
        
        # Start NMEA processing thread
        threading.Thread(target=self.process_nmea, daemon=True).start()
    
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
                    if line.startswith('$GPRMC') or line.startswith('$GPGGA'):
                        # Parse position from NMEA
                        position = self.parse_nmea_position(line)
                        if position:
                            self.last_position = position
                            
            except Exception as e:
                print(f"NMEA error: {e}")
                time.sleep(1)
    
    def parse_nmea_position(self, sentence):
        """Parse position from NMEA sentence"""
        # Simplified NMEA parsing - use pynmea2 library for production
        parts = sentence.split(',')
        if len(parts) >= 6:
            try:
                lat = float(parts[3]) if parts[3] else None
                lon = float(parts[5]) if parts[5] else None
                if lat and lon:
                    return (lat, lon)
            except ValueError:
                pass
        return None
    
    def send_aprs_position(self):
        """Send APRS position report"""
        if self.last_position:
            lat, lon = self.last_position
            
            # Create APRS position report
            position = f"{lat:.4f},{lon:.4f}"
            
            # Send to APRS-IS
            self.aprs_is.sendall(f"N0CALL>APRS,TCPIP*:@{position} LoRaTNCX Gateway")
    
    def run(self):
        """Main gateway loop"""
        self.connect_nmea()
        
        # APRS-IS connection
        self.aprs_is.connect()
        
        # Send position updates every 10 minutes
        while True:
            self.send_aprs_position()
            time.sleep(600)  # 10 minutes

# Usage
gateway = APRSGateway()
gateway.run()
```

**APRS Applications:**
- **Dire Wolf:** Sound card TNC with APRS capabilities
- **YAAC:** Yet Another APRS Client (Java-based)
- **APRSdroid:** Android APRS application
- **Xastir:** Linux APRS client
- **UI-View32:** Windows APRS software

### Position Beacon Configuration

**Automatic Beacons:**
Most APRS software can be configured to automatically send position beacons:

```ini
# Dire Wolf PBEACON configuration
PBEACON sendto=IG delay=0:30 every=10:00 symbol="igate" overlay="R" comment="LoRaTNCX iGate" via=WIDE1-1
```

**Manual Position Reports:**
Send position reports manually through packet radio applications:

```
N0CALL>APRS: =4455.55N/09322.22W- LoRaTNCX Mobile
```

## Performance and Accuracy

### GNSS Performance Factors

**Satellite Visibility:**
- **Open Sky:** Best performance, fast fixes
- **Urban Areas:** Reduced satellite visibility, slower fixes
- **Indoors:** Limited or no satellite reception
- **Weather:** Generally not affected by weather

**Accuracy Specifications:**
- **GPS Only:** ~5-10 meters horizontal accuracy
- **GPS + GLONASS:** Improved accuracy and faster fixes
- **SBAS Enabled:** ~1-2 meters accuracy (WAAS, EGNOS, etc.)

**Time to First Fix:**
- **Cold Start:** 30-60 seconds
- **Warm Start:** 10-30 seconds
- **Hot Start:** 1-5 seconds

### Power Consumption

**GNSS Power States:**
- **Active:** ~25-30mA (tracking satellites)
- **Standby:** ~5-10mA (power control enabled)
- **Off:** ~0mA (power control disabled)

**Battery Life Impact:**
- Continuous operation: Reduces battery life by ~20-30%
- Periodic operation: Minimal impact if duty-cycled
- Mobile applications: Position updates every 5-10 minutes typical

## Troubleshooting

### General GNSS Diagnostics

**Monitor NMEA Output:**
Connect to the NMEA TCP server (port 10110) to monitor raw GNSS data:

```bash
# Monitor NMEA sentences in real-time
nc 192.168.4.1 10110
```

**Key Diagnostic Information:**
- **$GPTXT sentences:** Check for antenna status messages ("ANTENNA OPEN", "ANTENNA SHORT")
- **Satellite counts:** Look at $GPGSV, $BDGSV, etc. for satellites in view
- **HDOP values:** High values (>10) indicate poor geometry
- **Fix status:** "V" = no fix, "A" = valid fix in $GNRMC/$GPRMC

### No GNSS Fix

**Symptoms:** No satellite lock, no position data

**Solutions:**
- Check antenna connection and placement
- Ensure clear sky view (outdoors)
- Wait 1-2 minutes for satellite acquisition
- Check GNSS module power (LED indicators)
- Verify correct baud rate configuration
- Check for "ANTENNA OPEN" messages in $GPTXT sentences

### Antenna Connection Issues

**Symptoms:** $GPTXT messages showing "ANTENNA OPEN" or "ANTENNA SHORT"

**Solutions:**
- **ANTENNA OPEN:** Antenna not connected or cable damaged
  - Check antenna cable connection to GNSS module
  - Verify antenna cable continuity
  - Try different antenna cable
  - For V4 boards: Check integrated antenna connection
- **ANTENNA SHORT:** Antenna cable shorted
  - Check for damaged antenna cable
  - Verify antenna impedance (should be 50Ω)
  - Replace faulty antenna
- **No Antenna Messages:** May indicate older GNSS module firmware
  - Check module documentation for antenna status reporting
  - Monitor satellite visibility ($GPGSV, $BDGSV, etc.) instead

### Incorrect Position Data

**Symptoms:** Position readings are wrong or jumpy

**Solutions:**
- Check antenna orientation (point skyward)
- Avoid metal obstructions near antenna
- Allow time for GPS to stabilize (HDOP < 2.0 preferred)
- Check for multipath interference
- Verify coordinate system (lat/lon format)

### NMEA Server Connection Issues

**Symptoms:** Cannot connect to TCP port 10110

**Solutions:**
- Verify GNSS is enabled in configuration
- Check TCP port number (default 10110)
- Ensure device is on same network
- Test with different client applications
- Check firewall blocking port

### Serial GNSS Issues

**Symptoms:** No data on serial port

**Solutions:**
- Verify baud rate matches GNSS module
- Check serial pin connections
- Test with different terminal programs
- Verify GNSS module is powered and active

### APRS Integration Problems

**Symptoms:** Position reports not appearing on APRS networks

**Solutions:**
- Verify external APRS application configuration
- Check NMEA data format and parsing
- Ensure proper APRS packet formatting
- Test with known working APRS setup
- Check APRS-IS server connectivity

## Advanced Configuration

### GNSS Module Settings

**Baud Rate Configuration:**
- Default: 9600 bps
- Supported: 4800, 9600, 19200, 38400, 57600, 115200
- Must match module capabilities

**Update Rate:**
- Default: 1Hz (1 position update per second)
- Configurable: 0.1Hz to 10Hz (depending on module)
- Lower rates save power

**Constellation Selection:**
- GPS only (default)
- GPS + GLONASS
- GPS + Galileo (newer modules)
- All available constellations

### NMEA Sentence Filtering

Some GNSS modules allow filtering of NMEA sentences:

**Common Sentences:**
- GPGGA (essential for position)
- GPRMC (recommended minimum)
- GPGSV (satellite information)
- GPGSA (accuracy information)

**Filtering Commands:**
```
$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28  # Enable GGA,RMC,GSV,GSA
$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29  # RMC only
```

### Antenna Considerations

**Active vs Passive Antennas:**
- **Active:** Powered antenna with built-in amplifier
- **Passive:** No power required, lower gain
- V4 boards include active antenna with LNA

**Antenna Placement:**
- Clear sky view (hemispherical)
- Away from metal objects
- Minimum 10cm ground plane recommended
- Avoid proximity to LoRa antenna

### Multipath Mitigation

**Causes of Multipath:**
- Reflections from buildings/ground
- Indoor operation
- Near large metal structures

**Mitigation Strategies:**
- Use choke ring antennas (advanced)
- Position antenna away from reflectors
- Use carrier-phase tracking (expensive receivers)
- Accept reduced accuracy in problematic environments

---

**Next:** [Section 12: Applications and Integration](12-applications-integration.md) | **Previous:** [Section 10: KISS Protocol Usage](10-kiss-protocol-usage.md) | **Main Index:** [User Manual](../user_manual.md)