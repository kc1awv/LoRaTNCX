# 8. WiFi and Networking

## Overview

LoRaTNCX provides comprehensive WiFi networking capabilities that enable remote configuration, monitoring, and network-based KISS communication. The device can operate in multiple WiFi modes and includes a built-in web interface for easy management.

## WiFi Operating Modes

LoRaTNCX supports four WiFi operating modes, allowing flexible network integration:

### 1. Off Mode (TNC_WIFI_OFF = 0)
- **Description:** WiFi radio is completely disabled
- **Use Case:** Battery-powered operation, minimal power consumption
- **Access:** Serial connection only
- **Default:** No

### 2. Access Point Mode (TNC_WIFI_AP = 1)
- **Description:** Device creates its own WiFi network
- **Use Case:** Initial setup, field operation, direct device connection
- **Access:** Web interface at `http://192.168.4.1`
- **Default:** Yes (factory default)

### 3. Station Mode (TNC_WIFI_STA = 2)
- **Description:** Device connects to existing WiFi network
- **Use Case:** Integration with home/office networks, remote monitoring
- **Access:** Device gets IP from network DHCP
- **Default:** No

### 4. Access Point + Station Mode (TNC_WIFI_AP_STA = 3)
- **Description:** Device creates AP while connected to external network
- **Use Case:** Bridging networks, multiple device access
- **Access:** Both AP (192.168.4.1) and station IP available
- **Default:** No

## Default Configuration

When LoRaTNCX starts for the first time or after a factory reset:

- **Mode:** Access Point (AP) only
- **AP SSID:** `LoRaTNCX-XXXX` (where XXXX is last 4 hex digits of MAC address)
- **AP Password:** `loratncx`
- **AP IP Address:** `192.168.4.1`
- **TCP KISS Server:** Enabled on port 8001
- **DHCP:** Enabled for station mode

## Access Point Configuration

### Setting Up Access Point Mode

**Via Web Interface:**
1. Connect to `LoRaTNCX-XXXX` network with password `loratncx`
2. Open browser to `http://192.168.4.1`
3. Navigate to WiFi Configuration page
4. Set Mode to "Access Point"
5. Configure AP SSID and password
6. Save configuration

**Via REST API:**
```json
POST /api/wifi/config
{
  "mode": 1,
  "ap_ssid": "MyLoRaTNC",
  "ap_password": "mypassword123"
}
```

### Access Point Security

- **Password:** 8-63 characters required
- **Encryption:** WPA2-PSK (WiFi Protected Access 2 with Pre-Shared Key)
- **Channel:** Auto-selected (typically channel 1, 6, or 11)
- **Max Clients:** Up to 4 concurrent connections

**Security Recommendations:**
- Use strong, unique passwords
- Change default SSID and password
- Avoid using in public areas without additional security measures

## Station Mode Setup

### Connecting to Existing Networks

**Via Web Interface:**
1. Connect to LoRaTNCX access point
2. Open browser to `http://192.168.4.1`
3. Go to WiFi Configuration page
4. Set Mode to "Station" or "Access Point + Station"
5. Click "Scan Networks" to find available WiFi networks
6. Select your network and enter password
7. Configure IP settings (DHCP recommended)
8. Save configuration

**Via REST API:**
```json
POST /api/wifi/config
{
  "mode": 2,
  "ssid": "MyHomeNetwork",
  "password": "networkpassword",
  "dhcp": true
}
```

### Network Scanning

The device can scan for available WiFi networks:

**Via Web Interface:**
- WiFi Configuration page has "Scan Networks" button
- Shows SSID, signal strength (RSSI), and encryption status

**Via REST API:**
```http
GET /api/wifi/scan
```

**Response:**
```json
{
  "networks": [
    {
      "ssid": "MyHomeNetwork",
      "rssi": -45,
      "encrypted": true
    },
    {
      "ssid": "OpenNetwork",
      "rssi": -67,
      "encrypted": false
    }
  ]
}
```

### Static IP Configuration

For networks without DHCP or when a fixed IP is required:

**Via Web Interface:**
- Set "DHCP" to disabled
- Enter IP address, gateway, subnet mask, and DNS server

**Via REST API:**
```json
POST /api/wifi/config
{
  "mode": 2,
  "ssid": "MyNetwork",
  "password": "password",
  "dhcp": false,
  "ip": "192.168.1.100",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0",
  "dns": "8.8.8.8"
}
```

## TCP KISS Server

LoRaTNCX includes a TCP KISS server for network-based packet radio applications.

### Configuration

- **Default Port:** 8001
- **Protocol:** Raw TCP with KISS framing
- **Enabled by Default:** Yes

**Via Web Interface:**
- WiFi Configuration page
- "TCP KISS Server" section
- Enable/disable and set port number

**Via REST API:**
```json
POST /api/wifi/config
{
  "tcp_kiss_enabled": true,
  "tcp_kiss_port": 8001
}
```

### Connecting to TCP KISS Server

**Using Netcat:**
```bash
nc 192.168.4.1 8001
```

**Using Telnet:**
```bash
telnet 192.168.4.1 8001
```

**Using Python:**
```python
import socket
import time

# Connect to TCP KISS server
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.4.1', 8001))

# Send KISS frame (example: SET HARDWARE command)
kiss_frame = b'\xc0\x06\x01\x00\x00\x80\x3f\xc0'  # SET FREQUENCY to 1.0 MHz
sock.send(kiss_frame)

# Receive response
response = sock.recv(1024)
print(f"Received: {response.hex()}")

sock.close()
```

### Integration Examples

**Dire Wolf Integration:**
```bash
# In direwolf.conf
AGWPORT 8001
AGWPEER 192.168.4.1:8001
```

**Custom Applications:**
The TCP KISS server accepts standard KISS frames for:
- Transmitting packets via LoRa
- Receiving packets from LoRa network
- Configuring radio parameters
- Monitoring device status

## Security Considerations

### Network Security

1. **Change Default Passwords:**
   - Access point password should be changed from default
   - Use strong passwords (12+ characters, mixed case, numbers, symbols)

2. **WiFi Encryption:**
   - Only connect to WPA2/WPA3 encrypted networks
   - Avoid open (unencrypted) WiFi networks

3. **Firewall and Access Control:**
   - Limit access to web interface when in station mode
   - Use VPN for remote access over public networks

### Device Security

1. **Firmware Updates:**
   - Keep firmware updated for security patches
   - Verify firmware integrity before flashing

2. **Physical Security:**
   - Secure device physically when deployed
   - Use tamper-evident enclosures if needed

3. **Configuration Backup:**
   - Save configuration backups securely
   - Document network settings for recovery

## Troubleshooting

### Cannot Connect to Access Point

**Symptoms:** Device not visible in WiFi network list

**Solutions:**
- Check if device is powered on and WiFi is enabled
- Verify AP mode is active (OLED display shows "AP: LoRaTNCX-XXXX")
- Try different WiFi channels (1, 6, 11)
- Check for interference from other devices

### Cannot Connect to External Network

**Symptoms:** Station mode connection fails

**Solutions:**
- Verify SSID and password are correct
- Check signal strength (RSSI should be better than -70 dBm)
- Try different WiFi channels
- Check if network has MAC address filtering
- Verify DHCP server is available

### Web Interface Not Accessible

**Symptoms:** Cannot reach `http://192.168.4.1` or station IP

**Solutions:**
- Ensure connected to correct WiFi network
- Check IP address via OLED display or serial console
- Try different browsers
- Clear browser cache
- Check firewall/antivirus blocking access

### TCP KISS Server Connection Issues

**Symptoms:** Cannot connect to TCP port 8001

**Solutions:**
- Verify TCP KISS server is enabled in configuration
- Check port number (default 8001)
- Ensure no firewall blocking the port
- Try connecting from same network segment
- Check device logs for connection attempts

### Slow or Unreliable Connection

**Symptoms:** Intermittent connectivity, slow response

**Solutions:**
- Check signal strength and move closer to access point
- Avoid channel congestion (use WiFi analyzer app)
- Reduce number of connected devices
- Update device firmware
- Check for electrical interference

## Advanced Configuration

### mDNS Service Discovery

LoRaTNCX supports mDNS (Multicast DNS) for easy network discovery:

- **Service Name:** `_http._tcp`
- **Hostname:** `loratncx-xxxx.local` (where xxxx is MAC address suffix)
- **Access:** `http://loratncx-xxxx.local`

### Captive Portal

When in AP mode, the device provides a captive portal for initial configuration:

- Automatically redirects to web interface
- Works with most mobile devices and operating systems
- Provides setup wizard for first-time configuration

### Connection Monitoring

The device includes automatic connection monitoring:

- **Reconnection:** Automatic retry with exponential backoff
- **Status Display:** OLED shows connection status and IP addresses
- **Logging:** Connection events logged to serial console

### Power Management

WiFi power consumption can be significant:

- **AP Mode:** ~80-120mA continuous
- **STA Mode:** ~60-100mA when connected
- **AP+STA Mode:** ~100-150mA continuous

For battery operation, consider:
- Using station mode when possible
- Disabling WiFi when not needed
- Using sleep modes between transmissions

---

**Next:** [Section 9: Web Interface](09-web-interface.md) | **Previous:** [Section 7: LoRa Radio Configuration](07-lora-radio-configuration.md) | **Main Index:** [User Manual](../user_manual.md)