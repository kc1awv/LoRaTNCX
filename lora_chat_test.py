#!/usr/bin/env python3
"""
LoRa Chat Test Script

This script demonstrates the chat functionality by sending test messages
and testing various commands programmatically.
"""

import subprocess
import time
import sys
import threading
from pathlib import Path

def test_chat_functionality():
    """Test chat application functionality"""
    
    print("🧪 LoRa Chat Application Test")
    print("=" * 40)
    
    # Create test configuration
    test_config = {
        "station": {
            "callsign": "KX1TEST",
            "node_name": "Test Station",
            "location": "TestLab"
        },
        "radio": {
            "frequency": 915000000,  # US ISM band
            "tx_power": 17,
            "bandwidth_index": 8,    # 250 kHz
            "spreading_factor": 7,   # Fast data rate
            "coding_rate": 5
        },
        "hello": {
            "enabled": True,
            "interval": 15,  # Faster for testing
            "message_template": "CQ CQ from {node_name} ({callsign})"
        },
        "serial": {
            "port": "/dev/ttyACM0",
            "baudrate": 115200,
            "timeout": 1
        }
    }
    
    # Save test config
    import json
    with open("test_config.json", "w") as f:
        json.dump(test_config, f, indent=2)
    
    print("✅ Test configuration created")
    print(f"   Callsign: {test_config['station']['callsign']}")
    print(f"   Frequency: {test_config['radio']['frequency']/1e6:.1f} MHz")
    print(f"   Power: {test_config['radio']['tx_power']} dBm")
    print(f"   Bandwidth: 250 kHz, SF7, CR 4/5")
    
    # Test commands to send
    test_commands = [
        "/hello",
        "This is a test message from the automated test script",
        "/nodes", 
        "/set freq 433000000",
        "/set power 20",
        "Testing parameter changes",
        "/config",
        "/quit"
    ]
    
    print(f"\n🚀 Starting chat application with test config...")
    print("📝 Will send these test commands:")
    for i, cmd in enumerate(test_commands, 1):
        print(f"  {i}. {cmd}")
    
    print(f"\n⏰ Test will run for 20 seconds...")
    
    try:
        # Start the chat application
        process = subprocess.Popen(
            ["python3", "lora_chat.py", "/dev/ttyACM0", "test_config.json"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        # Give it time to start up
        time.sleep(3)
        
        # Send test commands
        for cmd in test_commands:
            print(f"📤 Sending: {cmd}")
            process.stdin.write(cmd + "\n")
            process.stdin.flush()
            time.sleep(2)  # Wait between commands
        
        # Wait for completion or timeout
        try:
            stdout, stderr = process.communicate(timeout=10)
            print("📋 Application output:")
            print(stdout)
            if stderr:
                print("⚠️  Errors:")
                print(stderr)
        except subprocess.TimeoutExpired:
            process.kill()
            print("⏰ Test timed out (normal for interactive app)")
        
    except Exception as e:
        print(f"❌ Test error: {e}")
    
    print("✅ Chat functionality test complete!")

def create_demo_readme():
    """Create a README for the chat application"""
    
    readme_content = """# LoRa KISS Chat Application

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
"""
    
    with open("LORA_CHAT_README.md", "w") as f:
        f.write(readme_content)
    
    print("📖 Created LORA_CHAT_README.md with comprehensive documentation")

if __name__ == "__main__":
    print("🧪 LoRa Chat Test Suite")
    print("=" * 50)
    
    # Create documentation
    create_demo_readme()
    
    # Run functionality test if requested
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        test_chat_functionality()
    else:
        print("\n✅ Demo files created!")
        print("\n🚀 To test the chat application:")
        print("   python3 lora_chat.py")
        print("\n🧪 To run automated test:")
        print("   python3 lora_chat_test.py test")
        print("\n📖 See LORA_CHAT_README.md for full documentation")