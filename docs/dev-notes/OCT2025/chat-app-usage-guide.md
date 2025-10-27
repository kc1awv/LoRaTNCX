# LoRaTNCX Chat Application Usage Guide

*Created: October 27, 2025*  
*Author: Claude Sonnet 4 (AI Assistant)*

## Overview

The LoRaTNCX Chat Application is a Python-based keyboard-to-keyboard chat system that interfaces with the LoRaTNCX KISS TNC for amateur radio packet communication over LoRa. This document provides comprehensive usage instructions and troubleshooting guidance.

## Quick Start

### Prerequisites

1. **Hardware**: LoRaTNCX device (Heltec WiFi LoRa 32 V4) with firmware loaded
2. **Software**: Python 3.6+ and pip
3. **License**: Valid amateur radio license for legal operation
4. **Connection**: USB cable to connect TNC to computer

### Installation

```bash
cd LoRaTNCX/chat_app
pip install -r requirements.txt        # Full version with colors
# OR
pip install -r requirements_simple.txt # Minimal version
```

### First Run

```bash
python loratncx_chat.py    # Full-featured version
# OR
python simple_chat.py      # Minimal version
```

## Detailed Usage Guide

### Initial Setup Process

1. **Device Detection**
   - Application scans common serial ports (`/dev/ttyUSB0`, `/dev/ttyACM0`, etc.)
   - If not found automatically, prompts for manual port entry
   - Connects at 115200 baud with 2-second initialization delay

2. **User Information Setup**
   - **Node Name**: Human-readable identifier (e.g., "My LoRa Station")
   - **Callsign**: Amateur radio callsign (automatically uppercased)
   - Settings are saved and pre-filled on subsequent runs

3. **Radio Configuration**
   - Reviews current settings (frequency, power, bandwidth, etc.)
   - Option to modify parameters interactively
   - Sends KISS commands to configure TNC hardware

### Chat Interface

Once configured, the application displays:
```
=== LoRaTNCX Chat Active ===
Node: My Station (KC1ABC)
Frequency: 915.0 MHz
Type messages to send, or:
  /hello - Send hello beacon
  /config - Show current configuration
  /quit - Exit chat
==================================================
```

### Message Display Format

- **Sent messages**: `[HH:MM:SS] YOUR_CALL> message text` (blue in full version)
- **Received messages**: `[HH:MM:SS] OTHER_CALL> message text` (green in full version)
- **Hello beacons**: `[HELLO] Beacon sent` (magenta in full version)
- **Raw packets**: `[HH:MM:SS] RAW> unparsed data` (yellow in full version)

### Commands

| Command | Function |
|---------|----------|
| `/hello` | Send hello beacon manually |
| `/config` | Display current configuration |
| `/quit` | Exit application |
| Any other text | Send as chat message |

### Automatic Features

- **Hello Beacons**: Sent every 5 minutes for station discovery
- **Message Processing**: Continuous background reception
- **Configuration Persistence**: Settings saved to `chat_config.json`

## Radio Configuration Options

### Frequency Settings
- **433 MHz ISM**: 433.050 - 434.790 MHz (EMEA, APAC, ITU Regions 1 & 3)
- **868 MHz ISM**: 863.000 - 870.000 MHz (Europe/CEPT/ETSI)
- **915 MHz ISM**: 902.000 - 928.000 MHz (Americas, ITU Region 2)
- **Amateur Bands**: Per license privileges and local band plans

### Power Levels
- **Low Power**: -9 to 2 dBm (local, battery conservation)
- **Medium Power**: 2-14 dBm (local to regional networks)
- **High Power**: 17-22 dBm (long distance, high consumption)

### LoRa Parameters
- **Bandwidth**: 7.8 to 500.0 kHz (default: 125.0 kHz)
- **Spreading Factor**: 7-12 (default: 9)
- **Coding Rate**: 5-8 representing 4/5 to 4/8 (default: 7)

## Message Protocol

### Packet Format
Messages are transmitted using a simple format:
```
FROM_CALL>TO_CALL:MESSAGE_TEXT
```

Examples:
```
KC1ABC>ALL:Hello from my LoRa station!
VE1DEF>KC1ABC:Thanks for the beacon, heard you 5/9
N0XYZ>ALL:Testing new antenna setup
```

### Special Considerations
- **Character Encoding**: UTF-8 with error handling for malformed packets
- **Message Length**: Limited by LoRa packet size and airtime regulations
- **KISS Framing**: Automatic byte stuffing for data integrity

## Troubleshooting

### Device Connection Issues

**Problem**: "Failed to connect to TNC"
- **Check**: USB cable connection and device power
- **Try**: Different USB ports or cables
- **Linux**: Add user to dialout group: `sudo usermod -a -G dialout $USER`
- **Verify**: Device appears as `/dev/ttyUSB*` or `/dev/ttyACM*`

**Problem**: Device found but no response
- **Check**: Firmware is properly loaded on TNC
- **Verify**: Device initializes (watch for LED activity)
- **Try**: Restart device and wait for full initialization

### Communication Issues

**Problem**: Messages sent but not received by others
- **Verify**: Radio settings match other stations
- **Check**: Antenna connections and SWR
- **Test**: Frequency is within license privileges
- **Try**: Different power levels or locations

**Problem**: Can't receive messages from others
- **Verify**: Same frequency, bandwidth, and spreading factor
- **Check**: RF environment and interference
- **Test**: Known good station for comparison

**Problem**: Garbled or incomplete messages
- **Check**: Signal strength and quality
- **Adjust**: Spreading factor (higher for better reception)
- **Verify**: Coding rate settings
- **Test**: Shorter messages to rule out airtime limits

### Application Issues

**Problem**: Python dependency errors
- **Solution**: Install requirements: `pip install -r requirements.txt`
- **Alternative**: Use simple version: `python simple_chat.py`

**Problem**: Permission denied on serial port
- **Linux**: `sudo usermod -a -G dialout $USER` then logout/login
- **Check**: No other applications using the port

**Problem**: Configuration not saving
- **Verify**: Write permissions in application directory
- **Check**: Disk space availability
- **Manual**: Copy `chat_config.json.example` to `chat_config.json`

## Testing and Validation

### Protocol Testing
Use the included test script to verify KISS implementation:
```bash
python kiss_test.py
```

This tests:
- KISS byte stuffing algorithms
- TNC connection and response
- Basic packet transmission

### Range Testing
1. Start with both stations close together
2. Verify bidirectional communication
3. Gradually increase distance
4. Note signal quality degradation points
5. Test different antenna configurations

### Performance Monitoring
- **Message Success Rate**: Count sent vs. acknowledged
- **Signal Reports**: Exchange signal strength information  
- **Throughput**: Time large messages and calculate effective data rate
- **Battery Life**: Monitor power consumption during extended use

## Best Practices

### Operating Procedure
1. **Identify**: Always include your callsign in messages
2. **Listen**: Monitor frequency before transmitting
3. **Coordinate**: Agree on frequency and parameters with other stations
4. **Courtesy**: Keep messages concise to minimize airtime
5. **Log**: Maintain records of contacts and configurations

### Technical Considerations
- **Frequency Coordination**: Use established calling frequencies
- **Parameter Standardization**: Document working configurations
- **Antenna Optimization**: Test different antenna types and orientations
- **Location Selection**: Consider RF environment and terrain

### Legal Compliance
- **License Verification**: Ensure all operators hold valid licenses
- **Frequency Privileges**: Verify frequency is within license class
- **Power Limits**: Respect maximum power restrictions
- **Identification**: Follow local identification requirements
- **Third Party**: Comply with third-party traffic restrictions

## Advanced Usage

### Multiple Station Networks
- Coordinate common frequencies and parameters
- Establish check-in schedules
- Use consistent node naming conventions
- Document network configuration

### Integration with Other Systems
- **APRS Gateway**: Bridge to APRS-IS network
- **Digital Modes**: Interface with other packet protocols
- **Logging Software**: Export contacts to amateur radio logging programs
- **Automation**: Script repetitive operations

### Custom Modifications
The application is designed for easy modification:
- **Message Formats**: Customize packet structure
- **UI Improvements**: Add features like chat history
- **Protocol Extensions**: Implement additional KISS commands
- **Network Features**: Add mesh or relay capabilities

## Configuration File Reference

The `chat_config.json` file stores persistent settings:

```json
{
  "callsign": "KC1ABC",
  "node_name": "My LoRa Node", 
  "radio_config": {
    "frequency": 915.0,
    "tx_power": 8,
    "bandwidth": 125.0,
    "spreading_factor": 9,
    "coding_rate": 7,
    "tx_delay": 30,
    "persistence": 63,
    "slot_time": 10
  }
}
```

All parameters can be modified directly in the file or through the application interface.

## Conclusion

The LoRaTNCX Chat Application provides a practical interface for testing and using the LoRaTNCX KISS TNC. It demonstrates proper KISS protocol implementation while offering a user-friendly chat experience. The application serves as both a functional tool and educational example for amateur radio packet communication development.

For technical details about the code implementation, see the companion document `chat-app-code-architecture.md`.