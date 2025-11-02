# LoRaTNCX - LoRa Terminal Node Controller

A terminal node controller implementation for Heltec WiFi LoRa 32 V3 and V4 development boards using the RadioLib library.

## Dependencies

This project uses the [RadioLib](https://github.com/jgromes/RadioLib) library version 7.4.0 for LoRa communication, providing a modern and well-maintained interface to the SX1262 LoRa transceiver.

## Hardware Support

### Heltec WiFi LoRa 32 V3
- **LoRa Chip**: SX1262
- **Frequency Bands**: 433-510 MHz, 863-928 MHz
- **Maximum TX Power**: 22 dBm
- **Default TX Power**: 10 dBm
- **PA Control**: Not available (direct radio control)

### Heltec WiFi LoRa 32 V4
- **LoRa Chip**: SX1262
- **Frequency Bands**: 433-510 MHz, 863-928 MHz  
- **Maximum TX Power**: 22 dBm
- **Default TX Power**: 14 dBm
- **PA Control**: Available (automatic TX/RX switching)

## Build Environments

The project provides 4 build environments for different hardware versions and frequency bands:

### V3 Environments
- `heltec_wifi_lora_32_V3` - 868 MHz band (863-928 MHz, includes 915 MHz)
- `heltec_wifi_lora_32_V3_433` - 433 MHz band (433-510 MHz)

### V4 Environments
- `heltec_wifi_lora_32_V4` - 868 MHz band (863-928 MHz, includes 915 MHz)
- `heltec_wifi_lora_32_V4_433` - 433 MHz band (433-510 MHz)

**Note**: The 868 MHz band covers 863-928 MHz, which includes the North American 915 MHz ISM band. You can set any frequency within the supported range using the `lora freq` command.

## Building

To build for a specific environment:
```bash
# Build V3 for 868 MHz band (includes 915 MHz)
platformio run -e heltec_wifi_lora_32_V3

# Build V4 for 433 MHz band
platformio run -e heltec_wifi_lora_32_V4_433

# Build all environments
platformio run
```

## Serial Console Commands

The device provides a serial console interface with the following commands:

### System Commands
- `help` - Show available commands
- `status` - Display system information (uptime, memory, etc.)
- `clear` - Clear the terminal screen
- `reset` - Restart the device

### LoRa Commands
- `lora status` - Show current LoRa radio state
- `lora config` - Display LoRa configuration
- `lora stats` - Show transmission/reception statistics
- `lora send <message>` - Transmit a LoRa message
- `lora rx` - Start continuous receive mode
- `lora freq <mhz>` - Set frequency in MHz (e.g., 868.0)
- `lora power <dbm>` - Set TX power in dBm
- `lora sf <sf>` - Set spreading factor (7-12)
- `lora bw <khz>` - Set bandwidth in kHz (125/250/500) or shorthand (0/1/2)
- `lora cr <cr>` - Set coding rate (5-8 for 4/5-4/8) or legacy (1-4)

## LoRa Configuration

### Default Settings
- **Bandwidth**: 125 kHz
- **Spreading Factor**: SF7
- **Coding Rate**: 4/5
- **Preamble Length**: 8 symbols
- **CRC**: Enabled
- **IQ Inversion**: Disabled

### Frequency Bands
The frequency band is set at compile time using build flags:
- `FREQ_BAND_433` - 433 MHz (433-510 MHz range)
- `FREQ_BAND_868` - 868 MHz (863-928 MHz range, includes 915 MHz)

The 868 MHz band covers the full 863-928 MHz range, which includes both European 868 MHz and North American 915 MHz ISM bands. You can set any frequency within the supported range at runtime using `lora freq <mhz>`.

### Power Settings
Power levels are automatically limited based on hardware:
- **V3**: -9 to 22 dBm
- **V4**: -9 to 22 dBm

The V4 hardware automatically enables the PA (Power Amplifier) for transmit operations and disables it for receive operations through GPIO control.

## Usage Examples

### Basic Communication Test
1. Flash two devices with the same frequency band configuration
2. Connect to serial console (115200 baud)
3. On device 1: `lora send Hello World`
4. On device 2: You should see the received message
5. Both devices automatically return to receive mode after transmission

### Changing Configuration
```
> lora freq 434.0
[LoRa] Frequency set to 434.0 MHz

> lora power 15
[LoRa] TX power set to 15 dBm

> lora sf 10
[LoRa] Spreading factor set to SF10

> lora bw 250
[LoRa] Bandwidth set to 250.0 kHz
```

### Monitoring
```
> lora stats
[LoRa] Statistics:
  TX Count: 5
  RX Count: 12
  Last RSSI: -45 dBm
  Last SNR: 8 dB
  Current State: RX
```

## Hardware Differences

The main differences between V3 and V4 that affect software:

1. **Power Amplifier Control**: V4 has dedicated PA control pins that are automatically managed during TX/RX operations
2. **Default Power**: V4 uses higher default power (14 dBm vs 10 dBm) due to PA availability
3. **OLED I2C Pins**: Different I2C pin assignments for the display (not yet implemented)

The LoRaRadio class automatically handles these differences based on compile-time board detection using RadioLib's hardware abstraction.

## License

This project is open source. Please refer to the license file for details.