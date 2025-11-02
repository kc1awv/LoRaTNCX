# SPIFFS Regional Bands File Upload Guide

This guide explains how to upload custom regional frequency bands to your LoRaTNCX device using the SPIFFS filesystem.

## Template File

The `regional_bands_template.json` file contains examples for various regions and use cases:

- **ISM Bands**: US, EU, Japan, Australia, Canada, China, Brazil, India, South Korea
- **Amateur Radio**: Regional subbands for different modes
- **Templates**: Custom band templates you can modify

## Upload Methods

### Method 1: PlatformIO Upload Filesystem (Recommended)

1. **Customize the template**:
   ```bash
   # Copy template to active filename
   cp data/regional_bands_template.json data/regional_bands.json
   
   # Edit the file to your needs
   nano data/regional_bands.json  # or use your preferred editor
   ```

2. **Upload to device**:
   ```bash
   # Upload filesystem data to device
   pio run --target uploadfs
   ```

3. **Verify upload**:
   - Connect to device serial console
   - Use `lora bands` command to see loaded bands
   - Check for "[FreqBand] Loaded X regional bands" message

### Method 2: Arduino IDE SPIFFS Plugin

1. **Install ESP32 SPIFFS plugin** for Arduino IDE
2. **Copy and customize**:
   - Copy `regional_bands_template.json` to your Arduino sketch `data/` folder
   - Rename to `regional_bands.json`
   - Edit for your region's requirements
3. **Upload**: Use "ESP32 Sketch Data Upload" from Tools menu
4. **Verify**: Check serial output for successful loading

### Method 3: Manual SPIFFS Upload

For devices already deployed, you can use web upload or direct SPIFFS tools:

```bash
# Using esptool.py (replace with your device settings)
esptool.py --port COM3 --baud 921600 write_flash 0x3D0000 regional_bands.bin
```

## Customization Guide

### 1. Select Your Region

Enable only the bands relevant to your location:

```json
{
  "name": "US ISM 902-928 (FCC Compliant)",
  "identifier": "US_FCC_ISM_902_928", 
  "enabled": true  // Set to false to disable
}
```

### 2. Frequency Ranges

Adjust frequency ranges for local regulations:

```json
{
  "min_frequency": 902.0,     // Minimum allowed frequency (MHz)
  "max_frequency": 928.0,     // Maximum allowed frequency (MHz) 
  "default_frequency": 915.0  // Default when band is selected
}
```

### 3. License Types

Set appropriate license requirements:

```json
{
  "license": 0,  // 0=ISM (no license), 1=Amateur (license required), 2=Custom
}
```

### 4. Regional Identification

Use clear regional identifiers:

```json
{
  "region": "US",                    // Country/region code
  "description": "Detailed rules..."  // Regulatory information
}
```

## Example Customizations

### US Ham Radio Operator

Keep only US amateur bands and ISM:

```json
"enabled": true   // US_FCC_ISM_902_928
"enabled": true   // US_AMATEUR_70CM_WEAK  
"enabled": true   // US_AMATEUR_70CM_DIGITAL
"enabled": true   // US_AMATEUR_33CM_WEAK
"enabled": false  // All EU/JP/etc bands
```

### EU ISM User

Enable EU-specific ISM bands:

```json
"enabled": true   // EU_ETSI_SRD_433
"enabled": true   // EU_ETSI_SRD_863_870_1PCT  
"enabled": false  // All US-specific bands
```

### Multi-Region Device

Enable global ISM bands for travel:

```json
"enabled": true   // ISM_433 (global)
"enabled": true   // ISM_902_928 (US/CA/AU)
"enabled": true   // EU_ETSI_SRD_863_870 (EU)
"enabled": true   // JP_ARIB_920_928 (Japan)
```

## Verification Commands

After uploading, verify the bands loaded correctly:

```bash
# Show all loaded bands
lora bands

# Filter by type
lora bands ism
lora bands amateur

# Check current configuration  
lora band

# Test band selection
lora band US_FCC_ISM_902_928
```

## Troubleshooting

### File Not Found
- **Error**: `[FreqBand] Regional bands file /regional_bands.json not found`
- **Solution**: Ensure file is named exactly `regional_bands.json` in SPIFFS root

### JSON Parse Error
- **Error**: `[FreqBand] JSON parse error in /regional_bands.json`
- **Solution**: Validate JSON syntax, remove comments if copying from template

### Band Not Available
- **Error**: `[FreqBand] Band not found or disabled: BAND_ID`
- **Solution**: Check that band has `"enabled": true` in the JSON file

### SPIFFS Mount Failed
- **Error**: `[FreqBand] SPIFFS initialization failed`
- **Solution**: Ensure device has SPIFFS partition, re-upload filesystem

## Legal Compliance

⚠️ **Important**: You are responsible for ensuring compliance with local regulations:

- **Verify frequency legality** in your jurisdiction
- **Check power limits** and duty cycle restrictions  
- **Obtain required licenses** for amateur radio bands
- **Understand ISM band rules** which vary by region
- **Consult local authorities** when in doubt

The device provides warnings but does not enforce legal compliance.

## File Size Considerations

- **Maximum file size**: ~32KB (typical SPIFFS partition limit)
- **Recommended**: Keep under 100 bands for best performance
- **Optimization**: Remove unused bands and comments in production

## Updates and Maintenance

To update regional bands:

1. Edit `data/regional_bands.json`
2. Re-upload with `pio run --target uploadfs`
3. Restart device or use `lora bands` to reload
4. Verify with `lora bands` command

The device will automatically load regional bands on startup and merge them with built-in ISM and amateur radio bands.