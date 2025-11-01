# LoRaTNCX Web Interface

This directory contains the web interface files for LoRaTNCX that will be uploaded to the ESP32's LittleFS filesystem.

## File Structure

```
data/www/
├── index.html          # Main web interface page
├── css/
│   ├── pico.min.css   # Pico CSS framework (minified)
│   └── custom.css     # Custom styles for LoRaTNCX
└── js/
    ├── theme-switcher.js # Theme switching functionality
    └── app.js           # Main application JavaScript
```

## Features

### 🎨 Modern Design
- Uses Pico.css framework for clean, modern appearance
- Responsive design that works on desktop and mobile
- Dark mode with automatic browser preference detection
- Theme switcher (Auto/Light/Dark)

### 📊 Device Status Monitoring
- Real-time WiFi connection status
- System information (uptime, memory, CPU)
- LoRa radio status and configuration
- Battery level monitoring
- Signal strength indicators

### 📶 WiFi Management
- View current WiFi connection details
- Add new WiFi networks
- Remove saved networks
- Access Point mode information with password display

### 🔌 API Endpoints
- `/api/status` - Device and system status
- `/api/networks` - WiFi network management
- POST `/api/networks` - Add new network
- DELETE `/api/networks` - Remove network

## Usage

1. **Upload Files**: Use PlatformIO's "Upload Filesystem Image" task to upload these files to the ESP32
2. **Connect to Device**: 
   - If connected to WiFi: Use the device's IP address
   - If in AP mode: Connect to the LoRaTNCX-XXXX network and go to 192.168.4.1
3. **Access Interface**: Open a web browser and navigate to the device's IP address

## Development Notes

- Files are served from LittleFS filesystem
- JavaScript uses modern ES6+ features
- Theme preferences are stored in localStorage
- API calls use fetch() for modern async communication
- Responsive design uses CSS Grid and Flexbox

## Browser Compatibility

- Modern browsers with ES6+ support
- Chrome 60+, Firefox 55+, Safari 12+
- Mobile browsers (iOS Safari, Chrome Mobile)

## File Upload Instructions

To upload the web interface files to your LoRaTNCX device:

1. Build your project first: `pio run`
2. Upload the filesystem: `pio run --target uploadfs`
3. Upload the firmware: `pio run --target upload`

The web interface will be available immediately after the device boots and connects to WiFi or starts its access point.