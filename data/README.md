# Data Folder - SPIFFS Web Interface Files

This folder contains files that will be uploaded to the ESP32's SPIFFS filesystem.

## Contents

- **index.html** - Web-based configuration interface for LoRaTNCX

## Uploading to Device

The files in this folder must be uploaded to SPIFFS before the web interface will work.

### Using PlatformIO Tasks (VS Code)
1. Open the PlatformIO menu
2. Select your environment (V3 or V4)
3. Run the "Upload Filesystem Image" task

Or use the predefined tasks:
- "Upload Filesystem (SPIFFS) V3"
- "Upload Filesystem (SPIFFS) V4"

### Using PlatformIO CLI

For Heltec V3:
```bash
pio run --target uploadfs --environment heltec_wifi_lora_32_V3
```

For Heltec V4:
```bash
pio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

### Upload Order
1. Upload firmware first (normal upload)
2. Upload filesystem (uploadfs target)
3. The device will serve the web interface from SPIFFS

## Web Interface Features

The web interface provides:
- Real-time status monitoring
- LoRa radio configuration
- WiFi network management
- System information
- Network scanning
- Configuration persistence
- Remote reboot

## Accessing the Interface

After uploading:
1. Connect to the TNC's WiFi AP: `LoRaTNCX-XXXX`
2. Open browser to: `http://192.168.4.1`

## File Size

The current web interface is a single HTML file (~30KB) with embedded CSS and JavaScript for simplicity and reliability.

## Customization

You can modify `index.html` to customize the interface. Changes require re-uploading the filesystem to take effect.

## Troubleshooting

**Web interface not loading?**
- Verify SPIFFS upload completed successfully
- Check serial output for "SPIFFS mounted successfully"
- Ensure WiFi is enabled and connected
- Try accessing directly: `http://192.168.4.1/index.html`

**Changes not appearing?**
- Re-upload the filesystem after making changes
- Clear browser cache or try incognito mode
- Reboot the device after uploading
