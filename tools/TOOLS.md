# TNC Terminal Tools

This directory contains user-friendly tools for interacting with the LoRaTNCX device.

## Files

- **`tnc_terminal.py`** - Main terminal interface program
- **`test_connection.py`** - Simple connection test utility  
- **`install.sh`** - Installation script
- **`requirements.txt`** - Python package dependencies
- **`README.md`** - Detailed documentation

## Quick Start

1. Install dependencies:
   ```bash
   ./install.sh
   ```

2. Test connection:
   ```bash
   ./test_connection.py /dev/ttyACM0
   ```

3. Launch terminal interface:
   ```bash
   ./tnc_terminal.py /dev/ttyACM0
   ```

## Features

The TNC Terminal provides a three-pane interface:

- **TNC Output** - Command responses and status messages
- **Packet Log** - Real-time packet transmission/reception logging  
- **Input Area** - Command entry with history and mode detection

The interface automatically detects when you're in command mode vs converse mode and adapts the display accordingly.

## Usage Examples

### Basic TNC Configuration
```bash
# Launch terminal
./tnc_terminal.py /dev/ttyACM0

# In the terminal, configure your station:
MYCALL KC1AWV
FREQ 915.0
PRESET BALANCED
SAVE
```

### Testing Communication
```bash
# Send a beacon
BEACON

# Connect to another station  
CONNECT N0CALL

# Once connected, type messages normally
# Use /DISC to disconnect
```

See `README.md` for complete documentation.