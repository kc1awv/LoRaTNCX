# LoRa TNC Terminal Interface

A user-friendly terminal interface for interacting with the LoRaTNCX device over serial connection.

## Features

- **Three-pane interface:**
  - TNC Output window (top) - Shows responses from the TNC
  - Packet Log window (middle) - Shows incoming/outgoing packet information
  - Input window (bottom) - For entering commands and chat messages

- **Smart mode detection:**
  - Automatically detects command mode vs converse mode
  - Color-coded status indicators
  - Mode-specific input handling

- **Command history:**
  - Use Up/Down arrows to navigate through previous commands
  - Persistent history during session

- **Real-time updates:**
  - Live display of TNC responses
  - Packet logging with timestamps
  - Connection status monitoring

## Installation

1. Navigate to the tools directory:
   ```bash
   cd tools/
   ```

2. Run the installation script:
   ```bash
   chmod +x install.sh
   ./install.sh
   ```

   This will install the required Python packages and make the terminal executable.

## Usage

### Basic Usage
```bash
./tnc_terminal.py /dev/ttyACM0
```

### With Custom Baud Rate
```bash
./tnc_terminal.py /dev/ttyUSB0 -b 9600
```

### Command Line Options
- `port` - Serial port device (required)
- `-b, --baudrate` - Baud rate (default: 115200)

### Finding Your Serial Port
Common ports for ESP32 devices:
- `/dev/ttyACM0` - USB CDC/ACM device
- `/dev/ttyUSB0` - USB-to-serial adapter

Use `ls /dev/tty*` to see available ports.

## Interface Layout

```
┌─────────────────────────────────────────────┐
│                TNC Output                   │
│ [10:30:15] Connected to TNC                 │
│ [10:30:16] CMD> MYCALL KC1AWV               │
│ [10:30:16] Callsign set to KC1AWV           │
│                                             │
├─────────────────────────────────────────────┤
│                Packet Log                   │
│ [10:30:20] TX: Beacon frame sent            │
│ [10:30:25] RX: Packet from N0CALL           │
│                                             │
├─────────────────────────────────────────────┤
│              Input [COMMAND]                │
│ > FREQ 915.0_                               │
└─────────────────────────────────────────────┘
 Port: /dev/ttyACM0 | CONNECTED | COMMAND | F1=Help F10=Quit
```

## Key Commands

### Navigation
- **F1** - Show help screen
- **F10** - Quit program
- **Ctrl+C** - Quit program
- **Up/Down** - Navigate command history
- **Enter** - Send command or message

### TNC Commands (Command Mode)
- `MYCALL <callsign>` - Set your callsign
- `FREQ <frequency>` - Set operating frequency
- `POWER <dbm>` - Set transmit power
- `PRESET <name>` - Apply configuration preset
- `CONNECT <callsign>` - Connect to another station
- `BEACON` - Send beacon frame
- `HELP` - Show TNC command help

### Available Presets
- `HIGH_SPEED` - Fast data, short range
- `BALANCED` - Good balance of speed/range  
- `LONG_RANGE` - Maximum range, slower data
- `LOW_POWER` - Power-optimized settings
- `AMATEUR_70CM` - 70cm band optimized
- `AMATEUR_33CM` - 33cm band optimized
- `AMATEUR_23CM` - 23cm band optimized

### Converse Mode
When connected to another station:
- Type messages normally to send chat
- `/DISC` or `QUIT` - Disconnect from station
- `/CMD` - Return to command mode

## Color Coding

- **Green** - Connected status, transmitted packets
- **Red** - Disconnected status, errors
- **Yellow** - Warnings
- **Cyan** - Received packets
- **Magenta** - Commands

## Troubleshooting

### Permission Denied
If you get permission errors accessing the serial port:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

### Port Not Found
- Check that the device is connected
- Try different USB ports
- Use `dmesg | tail` to see recent USB device messages
- List available ports: `ls /dev/tty*`

### Connection Issues
- Verify baud rate matches TNC configuration (default: 115200)
- Check that no other programs are using the serial port
- Try disconnecting and reconnecting the USB cable

### Display Issues
- Ensure terminal is large enough (minimum 80x24)
- Some terminals may not support all color features
- Try resizing the terminal window if display is corrupted

## Dependencies

- **Python 3.6+** - Required for f-string support
- **pyserial** - Serial communication library
- **curses** - Terminal interface (included with Python on Linux)

## Development

The terminal interface is designed to be modular and extensible:

- `TNCTerminal` class handles all functionality
- Separate methods for each window type
- Threading for non-blocking serial I/O
- Configurable buffer sizes and display options

## License

This tool is part of the LoRaTNCX project and follows the same license terms.