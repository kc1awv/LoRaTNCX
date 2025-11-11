# LoRaTNCX Web Interface

This directory contains the web interface for LoRaTNCX, built with Bootstrap 5.

## Features

- **Status Dashboard**: Real-time device status, WiFi connection, IP address, and uptime
- **LoRa Configuration**: Complete radio parameter configuration
- **WiFi Configuration**: Network settings for Access Point and Station modes
- **System Information**: Firmware version, hardware details, memory usage
- **Responsive Design**: Works on desktop and mobile devices

## Development

### Prerequisites

- Node.js (for build scripts)
- Modern web browser

### Building

```bash
# Install dependencies (none required for basic build)
npm install

# Build the web interface
npm run build

# The built files will be in the dist/ directory
```

### Development Server

```bash
# Start a local development server
npm run dev
```

Then open `http://localhost:3000` in your browser.

## Deployment

The built files in `dist/` should be uploaded to the device's SPIFFS filesystem using PlatformIO:

```bash
# From the project root
./build-web.sh
platformio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

## API Endpoints

The web interface communicates with the device via REST API endpoints:

- `GET /api/status` - Device status information
- `GET /api/system` - System information
- `GET /api/lora/config` - Current LoRa configuration
- `POST /api/lora/config` - Apply LoRa configuration
- `POST /api/lora/save` - Save LoRa config to NVS
- `POST /api/lora/reset` - Reset LoRa config to defaults
- `POST /api/wifi/config` - Apply WiFi configuration
- `POST /api/system/restart` - Restart the device

## Technologies Used

- **Bootstrap 5**: CSS framework for responsive design
- **Vanilla JavaScript**: No additional frameworks for minimal footprint
- **Fetch API**: For AJAX requests to device API

## Browser Support

- Chrome/Chromium (recommended)
- Firefox
- Safari
- Edge
- Mobile browsers (iOS Safari, Chrome Mobile)