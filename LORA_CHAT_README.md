# LoRa KISS Chat Application

A comprehensive keyboard-to-keyboard chat application for testing LoRaTNCX devices.

## Features

🔧 **Dynamic Radio Configuration**
- Set frequency, power, bandwidth, spreading factor, coding rate via KISS SetHardware
- Real-time parameter changes without firmware modification
- Support for all ISM bands (433, 868, 915 MHz)

📡 **Node Discovery** 
- Automatic discovery of other nodes via hello packets
- Periodic beacon transmission with station information
- RSSI and timing information for discovered nodes

💬 **Real-time Chat**
- Keyboard-to-keyboard messaging
- Message timestamping and station identification
- Command interface for configuration

## Quick Start

1. **Basic Usage:**
   ```bash
   python3 lora_chat.py [serial_port] [config_file]
   ```

2. **Examples:**
   ```bash
   # Use default config
   python3 lora_chat.py
   
   # Specify port and config
   python3 lora_chat.py /dev/ttyACM0 my_config.json
   
   # Different node configurations
   python3 lora_chat.py /dev/ttyACM0 node1_config.json
   python3 lora_chat.py /dev/ttyACM1 node2_config.json
   ```

## Commands

### Chat Commands
- `/help` - Show help information
- `/config` - Display current configuration
- `/nodes` - Show discovered nodes
- `/hello` - Send hello packet immediately
- `/quit` - Exit application

### Radio Parameter Commands
- `/set freq <Hz>` - Set frequency (e.g., `/set freq 433000000`)
- `/set power <dBm>` - Set TX power (e.g., `/set power 17`)
- `/set bw <index>` - Set bandwidth (e.g., `/set bw 8` for 250kHz)
- `/set sf <value>` - Set spreading factor (e.g., `/set sf 7`)
- `/set cr <value>` - Set coding rate (e.g., `/set cr 5` for 4/5)

### Bandwidth Index Values
| Index | Bandwidth |
|-------|-----------|
| 0 | 7.8 kHz |
| 1 | 10.4 kHz |
| 2 | 15.6 kHz |
| 3 | 20.8 kHz |
| 4 | 31.25 kHz |
| 5 | 41.7 kHz |
| 6 | 62.5 kHz |
| 7 | 125 kHz |
| 8 | 250 kHz |
| 9 | 500 kHz |

## Configuration

Configuration is stored in JSON format with these sections:

### Station Information
```json
{
  "station": {
    "callsign": "MYCALL",
    "node_name": "My LoRa Node", 
    "location": "QTH"
  }
}
```

### Radio Parameters
```json
{
  "radio": {
    "frequency": 433000000,
    "tx_power": 14,
    "bandwidth_index": 7,
    "spreading_factor": 8,
    "coding_rate": 5
  }
}
```

### Hello/Beacon Configuration  
```json
{
  "hello": {
    "enabled": true,
    "interval": 60,
    "message_template": "Hello from {node_name} ({callsign})"
  }
}
```

## Testing Two Nodes

To test communication between two devices:

1. **Node 1:**
   ```bash
   python3 lora_chat.py /dev/ttyACM0 node1_config.json
   ```

2. **Node 2:**
   ```bash
   python3 lora_chat.py /dev/ttyACM1 node2_config.json
   ```

3. **Watch for node discovery via hello packets**
4. **Send test messages between nodes**
5. **Experiment with different radio parameters**

## Configuration Profiles

### Long Range Profile (Max Range)
```json
{
  "radio": {
    "frequency": 433000000,
    "tx_power": 20,
    "bandwidth_index": 6,
    "spreading_factor": 12,
    "coding_rate": 8
  }
}
```

### High Speed Profile (Max Data Rate)
```json
{
  "radio": {
    "frequency": 915000000,
    "tx_power": 17,
    "bandwidth_index": 9,
    "spreading_factor": 7,
    "coding_rate": 5
  }
}
```

### Balanced Profile (Good Range/Speed)
```json
{
  "radio": {
    "frequency": 868000000,
    "tx_power": 14,
    "bandwidth_index": 7,
    "spreading_factor": 9,
    "coding_rate": 5
  }
}
```

## Requirements

- Python 3.6+
- pyserial
- LoRaTNCX device in KISS mode
- Compatible LoRa radio (SX1262/SX1278/etc.)

## Troubleshooting

### Device Not Found
- Check USB/serial port permissions
- Verify device is connected and powered
- Try different port (ttyACM0, ttyACM1, ttyUSB0, etc.)

### No Communication
- Verify both devices use same frequency/parameters
- Check antenna connections
- Ensure devices are in range
- Check for interference on the frequency

### KISS Mode Issues
- Device should auto-enter KISS mode on connection
- Look for "KISS mode" confirmation message
- Try manual KISSM command if needed

## Protocol Details

The application uses standard KISS protocol with SetHardware extensions:

- **Data Frames:** Standard KISS data frames (0x00) for messages
- **SetHardware:** KISS command 0x06 for radio parameter control
- **Frame Format:** Standard KISS with FEND (0xC0) framing

This ensures compatibility with other KISS TNCs and applications.
