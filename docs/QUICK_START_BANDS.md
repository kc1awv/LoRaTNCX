# Quick Start Guide: Regional Frequency Bands

This guide helps you quickly set up custom regional frequency bands for your LoRaTNCX device.

## Step 1: Choose Your Approach

### Option A: Use Built-in Bands Only (Easiest)
The device comes with predefined ISM and Amateur Radio bands:
- ISM bands: 433MHz, 470-510MHz, 863-870MHz, 902-928MHz
- Amateur bands: 70cm, 33cm, 23cm

**No additional setup required** - skip to Step 4.

### Option B: Add Regional Custom Bands
Add region-specific bands with local regulations and restrictions.

## Step 2: Customize Regional Bands (Option B)

1. **Copy the template**:
   ```bash
   cp data/regional_bands_template.json data/regional_bands.json
   ```

2. **Edit for your region**:
   - Open `data/regional_bands.json` 
   - Find bands for your country/region
   - Set `"enabled": true` for bands you need
   - Set `"enabled": false` for bands you don't need

3. **Common regional edits**:
   
   **US Users**:
   ```json
   "US_FCC_ISM_902_928": { "enabled": true }
   "US_AMATEUR_70CM_WEAK": { "enabled": true }  // If you have ham license
   ```
   
   **EU Users**:
   ```json
   "EU_ETSI_SRD_433": { "enabled": true }
   "EU_ETSI_SRD_863_870": { "enabled": true }
   ```
   
   **Global Users**:
   ```json
   "ISM_433": { "enabled": true }        // Already built-in
   "ISM_902_928": { "enabled": true }    // Already built-in  
   ```

## Step 3: Upload Regional Bands (Option B)

**Method 1: PlatformIO (Recommended)**
```bash
pio run --target uploadfs
```

**Method 2: VS Code Task**
- Open Command Palette (`Ctrl+Shift+P`)
- Type "Tasks: Run Task"
- Select "Upload Filesystem (SPIFFS) V3" or "V4"

## Step 4: Build and Upload Firmware

1. **Build**:
   ```bash
   # For Heltec V3
   pio run -e heltec_wifi_lora_32_V3
   
   # For Heltec V4
   pio run -e heltec_wifi_lora_32_V4
   ```

2. **Upload**:
   ```bash
   # Upload to connected device
   pio run -e heltec_wifi_lora_32_V3 --target upload
   ```

## Step 5: Test Band Configuration

1. **Connect to serial console** (115200 baud)

2. **Check available bands**:
   ```
   lora bands              # Show all available bands
   lora bands ism          # Show ISM bands only
   ```

3. **Select a band**:
   ```
   lora band ISM_915       # Select North American ISM
   lora band AMATEUR_70CM  # Select 70cm amateur (if licensed)
   ```

4. **Set frequency**:
   ```
   lora freq 915.0         # Set frequency within selected band
   ```

## Step 6: Verify Operation

1. **Check current configuration**:
   ```
   lora band               # Show current band
   lora config             # Show LoRa configuration
   ```

2. **Test transmission**:
   ```
   lora send "Hello World" # Send test message
   ```

## Example Regional Configurations

### United States (ISM + Amateur)
```json
{
  "US_FCC_ISM_902_928": { "enabled": true },
  "US_AMATEUR_70CM_WEAK": { "enabled": true },
  "US_AMATEUR_33CM_WEAK": { "enabled": true }
}
```

### European Union (ISM Only)
```json
{
  "EU_ETSI_SRD_433": { "enabled": true },
  "EU_ETSI_SRD_863_870_1PCT": { "enabled": true }
}
```

### Global/Multi-Region
```json
{
  "ISM_433": { "enabled": true },
  "ISM_902_928": { "enabled": true },
  "EU_ETSI_SRD_863_870": { "enabled": true },
  "JP_ARIB_920_928": { "enabled": true }
}
```

## Troubleshooting

**No regional bands loaded?**
- Check that `regional_bands.json` exists in device SPIFFS
- Verify JSON syntax is valid
- Use `pio run --target uploadfs` to re-upload

**Band not available?**
- Ensure band has `"enabled": true` in JSON file
- Check that band identifier matches exactly
- Restart device after uploading new bands

**Frequency rejected?**
- Verify frequency is within selected band limits
- Use `lora band` to check current band restrictions
- Select appropriate band first, then set frequency

## Legal Compliance

⚠️ **Important**: You are responsible for legal operation:

- **ISM bands**: Usually no license required, but power/duty cycle limits apply
- **Amateur bands**: Require appropriate amateur radio license  
- **Custom bands**: User assumes full legal responsibility
- **Regional rules**: Vary by country - check local spectrum authority

## Next Steps

- Read [FREQUENCY_BAND_SYSTEM.md](FREQUENCY_BAND_SYSTEM.md) for complete documentation
- See [SPIFFS_UPLOAD_GUIDE.md](SPIFFS_UPLOAD_GUIDE.md) for detailed upload instructions
- Check [EXAMPLES.md](../EXAMPLES.md) for usage examples
- Review local spectrum regulations for your area