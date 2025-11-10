# 9. Web Interface

## Overview

LoRaTNCX includes a comprehensive web-based control center that provides full configuration and monitoring capabilities through any modern web browser. The interface is served directly from the device and requires no additional software installation.

## Accessing the Web Interface

### Default Access Points

**Access Point Mode (Default):**
- **URL:** `http://192.168.4.1`
- **Network:** `LoRaTNCX-XXXX` (where XXXX is last 4 hex digits of MAC address)
- **Password:** `loratncx`

**Station Mode:**
- **URL:** Device IP address (shown on OLED display or via serial)
- **Network:** Your configured WiFi network
- **Example:** `http://192.168.1.100`

**mDNS Discovery:**
- **URL:** `http://loratncx-xxxx.local` (where xxxx is MAC address suffix)
- **Works on:** Networks that support mDNS (Bonjour)

### Browser Compatibility

The web interface works with all modern browsers:
- **Chrome/Chromium:** Recommended (best performance)
- **Firefox:** Fully supported
- **Safari:** Supported (including iOS)
- **Edge:** Fully supported
- **Mobile browsers:** Responsive design for phones/tablets

### First-Time Setup

1. **Power on** the LoRaTNCX device
2. **Connect** to the `LoRaTNCX-XXXX` WiFi network
3. **Open browser** and navigate to `http://192.168.4.1`
4. **Configure** WiFi settings for your network (optional)
5. **Begin using** the device

## Status Dashboard

The status dashboard provides real-time information about device status and is visible at the top of every page.

### Status Cards

**Board Information:**
- **Icon:** Microchip symbol
- **Shows:** Board type (V3/V4) and hardware revision
- **Updates:** On device startup

**WiFi Status:**
- **Icon:** WiFi signal symbol (changes color based on status)
- **Shows:** Connection state (AP/STA/AP+STA) and mode
- **Colors:**
  - **Green:** Connected to network
  - **Red:** Disconnected or error
  - **Gray:** WiFi disabled

**IP Address:**
- **Icon:** Network cable symbol
- **Shows:** Current IP address(es)
- **Displays:**
  - AP mode: `192.168.4.1`
  - STA mode: DHCP-assigned IP
  - AP+STA mode: Both addresses

**Battery Status:**
- **Icon:** Battery symbol with level indicator
- **Shows:** Voltage and charge status
- **Levels:**
  - **Green:** > 3.9V (Good)
  - **Yellow:** 3.7-3.9V (Medium)
  - **Red:** < 3.4V (Low/Critical)

**Uptime:**
- **Icon:** Clock symbol
- **Shows:** Time since last restart
- **Format:** Days, hours, minutes, seconds

### Auto-Refresh Feature

- **Default:** Enabled (refreshes every 5 seconds)
- **Toggle:** Checkbox in navigation area
- **Purpose:** Keeps status information current
- **Disable:** For manual control or to reduce network traffic

## LoRa Configuration Page

The LoRa Radio tab provides complete control over LoRa radio parameters.

### Configuration Fields

**Frequency (MHz):**
- **Range:** 150.0 - 960.0 MHz
- **Default:** 915.0 MHz (US ISM band)
- **Input:** Numeric field with validation

**Bandwidth (kHz):**
- **Options:** 125, 250, 500 kHz
- **Default:** 125 kHz
- **Dropdown:** Select from available options

**Spreading Factor:**
- **Range:** 7-12
- **Default:** 12
- **Dropdown:** SF7 through SF12

**Coding Rate:**
- **Options:** 4/5, 4/6, 4/7, 4/8
- **Default:** 4/7
- **Dropdown:** Displayed as 5-8 (numerator-1)

**Output Power (dBm):**
- **Range:** 2-20 dBm (V3), 2-22 dBm (V4)
- **Default:** 20 dBm
- **Slider:** Visual power level indicator

**Sync Word:**
- **Format:** Hexadecimal (0x0000-0xFFFF)
- **Default:** 0x1424
- **Input:** Hex field with validation

### Configuration Actions

**Apply Changes:**
- **Button:** "Apply Configuration"
- **Action:** Applies settings to radio immediately
- **Feedback:** Success/error messages
- **Persistence:** Changes are temporary until saved

**Save to Flash:**
- **Button:** "Save Configuration"
- **Action:** Saves current settings to non-volatile memory
- **Result:** Settings persist across reboots

**Reset to Defaults:**
- **Button:** "Reset to Defaults"
- **Action:** Restores factory LoRa settings
- **Confirmation:** Requires user confirmation
- **Scope:** Only affects LoRa parameters

### Real-Time Feedback

- **Success Messages:** Green alerts for successful operations
- **Error Messages:** Red alerts for configuration errors
- **Validation:** Input validation prevents invalid parameters
- **Status Updates:** Radio status updates after configuration changes

## WiFi Configuration Page

The Network tab manages all WiFi and networking settings.

### WiFi Mode Selection

**Mode Options:**
- **Off:** WiFi completely disabled
- **Access Point:** Device creates WiFi network
- **Station:** Device connects to existing network
- **Access Point + Station:** Both modes simultaneously

**Mode Switching:**
- **Immediate Effect:** Changes apply when "Apply" is clicked
- **Connection Impact:** May disconnect current session
- **Reconnection:** User may need to reconnect to new network

### Access Point Configuration

**SSID Settings:**
- **Field:** Access Point SSID
- **Default:** `LoRaTNCX-XXXX`
- **Length:** 1-32 characters

**Password Settings:**
- **Field:** Access Point Password
- **Default:** `loratncx`
- **Requirements:** 8-63 characters
- **Security:** WPA2-PSK encryption

### Station Mode Configuration

**Network Selection:**
- **Scan Button:** "Scan Networks"
- **Results:** List of available networks with signal strength
- **Selection:** Click network to auto-fill SSID

**Connection Settings:**
- **SSID:** Network name (manual entry or from scan)
- **Password:** Network password
- **Security:** Supports WPA/WPA2/WPA3 networks

### Network Settings

**DHCP Configuration:**
- **Toggle:** Enable/disable DHCP
- **Default:** Enabled (automatic IP assignment)

**Static IP (when DHCP disabled):**
- **IP Address:** Device IP
- **Gateway:** Network gateway
- **Subnet Mask:** Network subnet
- **DNS Server:** DNS server IP

### TCP KISS Server

**Server Control:**
- **Enable/Disable:** TCP KISS server toggle
- **Port:** Server port number
- **Default:** Enabled, port 8001

**Connection Info:**
- **Status:** Shows if server is running
- **Clients:** Number of connected clients (future feature)

### Configuration Actions

**Apply Changes:**
- **Button:** "Apply Configuration"
- **Warning:** May cause disconnection
- **Delay:** 500ms delay before applying changes

**Save to Flash:**
- **Button:** "Save Configuration"
- **Persistence:** Settings survive reboots

## GNSS Configuration Page

The GNSS tab configures GPS/GLONASS functionality (V4 boards only).

### GNSS Settings

**Enable/Disable:**
- **Toggle:** GNSS functionality on/off
- **Default:** Enabled
- **Hardware:** Only available on V4 boards

**Status Display:**
- **Fix Status:** GPS lock indicator
- **Satellites:** Number of satellites visible
- **Position:** Latitude, longitude, altitude (when available)

### NMEA Configuration

**TCP Server:**
- **Port:** NMEA data TCP port
- **Default:** 10110
- **Protocol:** Standard NMEA over TCP

**Serial Output:**
- **Baud Rate:** Serial GNSS output
- **Default:** 9600 baud
- **Passthrough:** Raw NMEA data on serial port

## System Page

The System tab provides system information and maintenance functions.

### System Information

**Device Info:**
- **Board Type:** V3 or V4
- **Firmware Version:** Current firmware version
- **MAC Address:** Device MAC address
- **Chip ID:** ESP32 unique identifier

**Performance Metrics:**
- **Uptime:** Time since last restart
- **Free Heap:** Available RAM
- **CPU Frequency:** Current clock speed

**Storage Info:**
- **Flash Size:** Total flash memory
- **Free Space:** Available storage
- **SPIFFS Usage:** Filesystem utilization

### Maintenance Functions

**Reboot Device:**
- **Button:** "Reboot Device"
- **Action:** Restarts the ESP32
- **Warning:** Causes temporary disconnection

**Factory Reset:**
- **Button:** "Factory Reset" (future feature)
- **Action:** Reset all settings to defaults
- **Warning:** Removes all saved configurations

## REST API Usage

The web interface uses a RESTful API that can be accessed programmatically.

### API Endpoints

**Status Information:**
```http
GET /api/status
```
Returns real-time device status information.

**System Information:**
```http
GET /api/system
```
Returns system performance metrics.

**LoRa Configuration:**
```http
GET  /api/lora/config     # Get current LoRa settings
POST /api/lora/config     # Apply LoRa configuration
POST /api/lora/save       # Save LoRa config to flash
POST /api/lora/reset      # Reset LoRa to defaults
```

**WiFi Configuration:**
```http
GET  /api/wifi/config     # Get current WiFi settings
POST /api/wifi/config     # Apply WiFi configuration
POST /api/wifi/save       # Save WiFi config to flash
GET  /api/wifi/scan       # Scan available networks
```

**GNSS Configuration:**
```http
GET  /api/gnss/config     # Get GNSS settings
POST /api/gnss/config     # Apply GNSS configuration
GET  /api/gnss/status     # Get GNSS status/fix info
```

**System Control:**
```http
POST /api/reboot          # Reboot the device
```

### API Response Format

All API responses use JSON format:

**Success Response:**
```json
{
  "success": true,
  "data": { ... },
  "timestamp": 1234567890
}
```

**Error Response:**
```json
{
  "success": false,
  "error": "Error description",
  "timestamp": 1234567890
}
```

### Authentication

- **Current:** No authentication required
- **Security:** Access control via network isolation
- **Future:** May include API key authentication

### CORS Support

- **Enabled:** Cross-Origin Resource Sharing
- **Headers:** Standard CORS headers included
- **Purpose:** Allows web applications to access API

## Mobile Compatibility

The web interface is fully responsive and works on mobile devices.

### Mobile Features

**Touch-Optimized:**
- Large touch targets for buttons
- Swipe gestures for navigation
- Responsive layout for small screens

**Mobile Browsers:**
- **iOS Safari:** Full support
- **Android Chrome:** Full support
- **Mobile Firefox:** Supported

**Network Considerations:**
- **WiFi Only:** Requires connection to device network
- **No Cellular:** Cannot access over cellular data
- **Local Access:** Must be on same network segment

## Troubleshooting

### Cannot Access Web Interface

**Symptoms:** Browser cannot reach `http://192.168.4.1`

**Solutions:**
- Verify connection to correct WiFi network
- Check device power and status LEDs
- Try different browser or clear cache
- Check firewall/antivirus blocking access
- Verify device IP address via serial console

### Interface Not Loading

**Symptoms:** Page loads but interface doesn't work

**Solutions:**
- Check browser console for JavaScript errors
- Ensure JavaScript is enabled
- Try incognito/private browsing mode
- Clear browser cache and cookies
- Update browser to latest version

### Configuration Not Applying

**Symptoms:** Changes don't take effect

**Solutions:**
- Click "Apply" button after making changes
- Wait for success confirmation message
- Check for validation errors (red alerts)
- Try refreshing the page and reapplying
- Check device logs via serial console

### Auto-Refresh Not Working

**Symptoms:** Status doesn't update automatically

**Solutions:**
- Verify "Auto-refresh" checkbox is checked
- Check browser console for network errors
- Ensure stable WiFi connection
- Try manual refresh with browser reload
- Disable and re-enable auto-refresh

### Mobile Access Issues

**Symptoms:** Cannot access from mobile device

**Solutions:**
- Ensure mobile device is connected to device WiFi
- Check mobile browser compatibility
- Try different mobile browser
- Verify mobile data is disabled (must use WiFi)
- Check for mobile firewall or security apps

## Security Considerations

### Network Security

1. **Default Passwords:**
   - Change default AP password immediately
   - Use strong, unique passwords

2. **Network Access:**
   - Limit physical access to device
   - Use in trusted network environments
   - Consider VPN for remote access

3. **API Access:**
   - Currently open (no authentication)
   - Restrict network access to trusted devices
   - Monitor for unauthorized access

### Data Privacy

1. **Configuration Data:**
   - WiFi passwords stored locally
   - No data transmitted to external servers
   - All configuration remains on device

2. **Location Data:**
   - GNSS data stays local
   - NMEA data only accessible on local network
   - No automatic data transmission

## Advanced Features

### Keyboard Shortcuts

- **Ctrl+R:** Manual refresh
- **Tab:** Navigate between form fields
- **Enter:** Submit forms (where applicable)

### Browser Developer Tools

**Console Logging:**
- Open browser developer tools (F12)
- Monitor network requests and responses
- View JavaScript console for debugging

**Network Tab:**
- Monitor API calls and responses
- Check response times and status codes
- Debug connection issues

### Custom Styling

The interface uses CSS custom properties (variables) that can be modified for custom themes:

```css
:root {
  --primary-bg: #0a0a0a;
  --accent: #00d4aa;
  /* ... other variables ... */
}
```

---

**Next:** [Section 10: KISS Protocol Usage](10-kiss-protocol-usage.md) | **Previous:** [Section 8: WiFi and Networking](08-wifi-networking.md) | **Main Index:** [User Manual](../user_manual.md)