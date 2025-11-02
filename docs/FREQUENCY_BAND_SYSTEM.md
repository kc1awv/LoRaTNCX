# Frequency Band Configuration System

The LoRaTNCX now features a comprehensive frequency band management system that replaces the old build-time frequency flags. This system provides:

## Key Features

- **Runtime Band Selection**: No need to recompile for different frequency bands
- **Predefined ISM Bands**: Common ISM bands with proper frequency limits
- **Amateur Radio Support**: Ham radio bands with proper allocations
- **Regional Configurations**: Extensible regional band definitions
- **Legal Compliance**: Built-in frequency validation and licensing awareness

## Available Bands

### ISM Bands (No License Required)
- **ISM_433**: 433.05-433.92 MHz (Global)
- **ISM_470_510**: 470-510 MHz (China/Asia)
- **ISM_863_870**: 863-870 MHz (Europe)
- **ISM_902_928**: 902-928 MHz (North America)

### Amateur Radio Bands (License Required)
- **AMATEUR_70CM**: 420-450 MHz (70cm band, Global)
- **AMATEUR_33CM**: 902-928 MHz (33cm band, US)
- **AMATEUR_23CM**: 1240-1300 MHz (23cm band, Global)
- **AMATEUR_FREE**: 144-1300 MHz (Free selection, operator responsibility)

## Command Usage

### View Available Bands
```
lora bands              # Show all available bands
lora bands ism          # Show only ISM bands
lora bands amateur      # Show only amateur radio bands
```

### Select a Band
```
lora band               # Show current band
lora band ISM_915       # Select North American ISM band
lora band AMATEUR_70CM  # Select 70cm amateur band
```

### Set Frequency Within Band
```
lora freq               # Show current frequency and band
lora freq 915.0         # Set frequency (must be within current band)
```

## Regional Band Configuration

Users can extend the system by creating custom regional bands in `/regional_bands.json`:

```json
{
  "regional_bands": [
    {
      "name": "Custom Regional ISM",
      "identifier": "CUSTOM_ISM",
      "min_frequency": 863.0,
      "max_frequency": 870.0,
      "default_frequency": 868.0,
      "license": 0,
      "region": "Custom",
      "description": "Custom regional ISM band",
      "enabled": true
    }
  ]
}
```

### License Types
- `0`: ISM (No license required)
- `1`: Amateur Radio (License required)
- `2`: Custom (User responsibility)

## Migration from Build Flags

The old build flag system has been completely replaced with runtime configuration. The system automatically defaults to the North American ISM band (`ISM_902_928`) and users can select any available band using the command interface.

## Hardware Capabilities

The system respects hardware limitations:
- **SX1262 Range**: 150-960 MHz (typical)
- **Validation**: Automatic frequency validation against hardware and band limits
- **Error Handling**: Clear error messages for invalid frequency selections

## Examples

### Basic ISM Usage (No License)
```
lora band ISM_915       # Select North American ISM band
lora freq 915.0         # Set to center frequency
lora send "Hello World" # Transmit message
```

### Amateur Radio Usage (License Required)
```
lora band AMATEUR_70CM  # Select 70cm amateur band
lora freq 432.1         # Set to amateur frequency
lora send "CQ CQ DE CALL" # Amateur radio transmission
```

### Check Current Configuration
```
lora band               # Show current band and limits
lora freq               # Show current frequency
lora bands              # List all available bands
```

## Safety and Legal Compliance

**Important**: Users are responsible for ensuring their frequency usage complies with local regulations:

- **ISM Bands**: Generally license-free but may have power and duty cycle restrictions
- **Amateur Bands**: Require appropriate amateur radio license
- **Custom Bands**: User assumes full legal responsibility

The system provides warnings but does not enforce legal compliance - operators must ensure they have proper authorization for their chosen frequencies.