# LoRaTNCX Web Interface

## Overview

The LoRaTNCX Web Interface provides a modern, responsive web-based interface for monitoring and configuring your LoRaTNCX device. Built with Bootstrap 5 and featuring real-time WebSocket communication, it offers comprehensive control over all device functions.

## Features

### Dashboard
- Real-time status monitoring for Radio, GNSS, Battery, and Network
- Live RSSI and packet activity charts
- Recent APRS messages display
- Connection status indicator

### Radio Configuration
- Frequency, power, and modulation settings
- Bandwidth and spreading factor controls
- Real-time radio statistics
- Packet transmission/reception counters

### APRS Functionality
- Callsign and beacon configuration
- Message composition and sending
- Real-time message monitoring
- Auto-beacon controls

### System Management
- WiFi configuration
- System information display
- Firmware update capability
- Configuration backup/restore
- Real-time system logs

## Build Process

### Prerequisites
- Python 3.6+
- Internet connection (for downloading dependencies)
- PlatformIO (for firmware upload)

### Building the Web Interface

1. **Navigate to project directory:**
   ```bash
   cd /path/to/LoRaTNCX
   ```

2. **Run the build script:**
   ```bash
   python3 tools/build_web.py
   ```

3. **Build options:**
   ```bash
   # Skip downloading external dependencies (if already downloaded)
   python3 tools/build_web.py --no-download
   
   # Custom directories
   python3 tools/build_web.py --src custom/src --dist custom/dist --data custom/data
   ```

### Build Output

The build process:
1. Downloads Bootstrap, Bootstrap Icons, and Chart.js from CDN
2. Processes HTML to use local files instead of CDN links
3. Copies custom CSS and JavaScript files
4. Compresses all files with gzip (60-85% size reduction)
5. Generates a manifest file with build information
6. Copies everything to the `data/` directory for SPIFFS upload

**Space Usage:** ~750 KB total (7.8% of 9.375 MB SPIFFS)

## Deployment

### SPIFFS Upload with PlatformIO

1. **Ensure partition scheme is set correctly in `platformio.ini`:**
   ```ini
   board_build.partitions = app3M_spiffs9M_fact512k_16MB.csv
   ```

2. **Upload SPIFFS data:**
   ```bash
   ~/.platformio/penv/bin/platformio run -t uploadfs -e heltec_wifi_lora_32_V4
   ```

3. **Build and upload firmware:**
   ```bash
   ~/.platformio/penv/bin/platformio run -t upload -e heltec_wifi_lora_32_V4
   ```

## WebSocket API

The web interface communicates with the ESP32 via WebSocket on `/ws`. Messages are JSON-formatted.

### Message Types

#### Status Requests
```json
{
  "type": "request",
  "data": "all_status"  // or "status", "radio", "aprs", etc.
}
```

#### Configuration Updates
```json
{
  "type": "config",
  "category": "radio",
  "data": {
    "frequency": 433.775,
    "power": 20,
    "bandwidth": 125000,
    "spreading_factor": 7,
    "coding_rate": 5
  }
}
```

#### Commands
```json
{
  "type": "command",
  "action": "restart"  // or "send_aprs_message", "backup_config", etc.
}
```

### Response Format
```json
{
  "type": "radio",
  "payload": {
    "frequency": 433.775,
    "power": 20,
    "rssi": -95,
    "snr": 8.5,
    "packets_tx": 142,
    "packets_rx": 87
  }
}
```

## File Structure

```
web/
├── src/                    # Source files
│   ├── index.html         # Main HTML template
│   ├── style.css          # Custom CSS styles
│   └── app.js             # JavaScript application
├── dist/                  # Built files (intermediate)
└── README.md              # This file

data/                      # SPIFFS deployment files
├── index.html.gz          # Compressed HTML
├── bootstrap.min.css.gz   # Compressed Bootstrap CSS
├── app.js.gz             # Compressed application JS
├── manifest.json         # Build manifest
└── ...                   # Other compressed assets

tools/
└── build_web.py          # Build automation script
```

## Customization

### Styling
- Edit `web/src/style.css` for custom styles
- Bootstrap 5 classes available throughout
- CSS custom properties defined for consistent theming

### Functionality
- Modify `web/src/app.js` for new features
- Add new tabs by updating both HTML and JavaScript
- WebSocket message handlers can be extended for new data types

### Dependencies
- Bootstrap 5.3.2 (CSS framework)
- Bootstrap Icons 1.11.1 (icon font)
- Chart.js 4.4.0 (real-time charts)
- No jQuery dependency (vanilla JavaScript)

## Browser Compatibility

- Modern browsers with WebSocket support
- Responsive design works on mobile devices
- Progressive Web App features can be added
- Offline functionality possible with service workers

## Development

### Local Development
1. Use any HTTP server to serve `web/src/` during development
2. Mock WebSocket server for testing UI without hardware
3. Browser developer tools for debugging

### Hot Reload Setup
```bash
# Simple Python HTTP server for development
cd web/src
python3 -m http.server 8000
```

## Security Considerations

- WebSocket connections should be secured in production
- Consider authentication for configuration changes
- HTTPS recommended for remote access
- Input validation on both client and server sides

## Performance

- Gzip compression reduces file sizes by 60-85%
- Lazy loading of chart data minimizes initial load
- WebSocket provides efficient real-time updates
- Responsive design optimized for mobile performance

## Troubleshooting

### Build Issues
- Ensure Python 3.6+ is installed
- Check internet connection for dependency downloads
- Verify write permissions in project directory

### Upload Issues
- Confirm correct partition scheme in platformio.ini
- Check SPIFFS size limits (current usage: ~750 KB)
- Verify PlatformIO installation and ESP32 tools

### Runtime Issues
- Check ESP32 serial output for WebSocket errors
- Verify WiFi connectivity on device
- Use browser developer tools to debug JavaScript issues