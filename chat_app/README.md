# LoRaTNCX Chat Application

A simple keyboard-to-keyboard chat application that interfaces with the LoRaTNCX KISS TNC for amateur radio packet communication over LoRa.

## Features

- Simple text-based user interface
- KISS protocol support for LoRaTNCX device
- Configurable radio parameters (frequency, power, bandwidth, spreading factor, etc.)
- Automatic periodic "hello" beacons for station discovery
- Message logging with timestamps
- Persistent configuration storage
- Color-coded message display

## Requirements

- Python 3.6 or higher
- LoRaTNCX device connected via USB
- Amateur radio license (required for legal operation)

## Installation

1. Navigate to the chat_app directory:
   ```bash
   cd chat_app
   ```

2. Install required Python packages:
   ```bash
   pip install -r requirements.txt
   ```

## Usage

1. Connect your LoRaTNCX device via USB
2. Run the chat application:
   ```bash
   python loratncx_chat.py
   ```

3. Follow the setup prompts:
   - The application will scan for your TNC device
   - Enter your node name (human-readable identifier)
   - Enter your amateur radio callsign
   - Review and optionally modify radio settings

4. Once in chat mode, you can:
   - Type messages and press Enter to send
   - Use `/hello` to send a beacon manually
   - Use `/config` to display current settings
   - Use `/quit` to exit

## Radio Configuration

The application supports the following configurable parameters:

- **Frequency**: Operating frequency in MHz (default: 915.0)
- **TX Power**: Transmit power in dBm (default: 8)
- **Bandwidth**: Signal bandwidth in kHz (default: 125.0)
- **Spreading Factor**: LoRa spreading factor 7-12 (default: 9)
- **Coding Rate**: Forward error correction 5-8 (default: 7)

### Frequency Guidelines

- **433 MHz ISM**: 433.050 - 434.790 MHz (EMEA, APAC)
- **868 MHz ISM**: 863.000 - 870.000 MHz (Europe)
- **915 MHz ISM**: 902.000 - 928.000 MHz (Americas)
- **Amateur bands**: Check your license privileges and local band plans

⚠️ **Important**: Ensure you are operating within your amateur radio license privileges and local regulations.

## Message Format

Messages are transmitted in a simple format:
```
CALLSIGN>TO:MESSAGE
```

For example:
```
KC1ABC>ALL:Hello from my LoRa station!
```

## Configuration Storage

Settings are automatically saved to `chat_config.json` and will be loaded on subsequent runs.

## Troubleshooting

### Device Not Found
- Ensure the LoRaTNCX is connected via USB
- Check that the device is powered on
- Try different USB ports
- On Linux, you may need to add your user to the `dialout` group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  (Log out and back in after this change)

### Connection Issues
- Verify the correct serial port is being used
- Ensure no other applications are using the serial port
- Check USB cable integrity

### No Messages Received
- Verify radio settings match other stations
- Check antenna connections
- Ensure you're within range of other stations
- Try different frequencies if permitted by your license

## Technical Details

- Uses KISS protocol for TNC communication
- Serial communication at 115200 baud
- Implements KISS byte stuffing for data integrity
- Periodic hello beacons every 5 minutes
- Non-blocking message reception
- Color-coded terminal output

## License

This software is provided for amateur radio use. Users are responsible for ensuring compliance with all applicable regulations and license requirements.

## Contributing

This is a development tool for the LoRaTNCX project. Improvements and bug fixes are welcome.