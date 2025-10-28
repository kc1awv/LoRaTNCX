#!/usr/bin/env python3
"""
LoRa Chat Launcher - Easy setup for testing multiple nodes
"""

import subprocess
import sys
import os
import json
import time

def create_node_configs():
    """Create predefined node configurations for testing"""
    
    nodes = {
        "node1": {
            "station": {
                "callsign": "KX1TEST",
                "node_name": "Base Station",
                "location": "Lab Bench"
            },
            "radio": {
                "frequency": 433000000,
                "tx_power": 14,
                "bandwidth_index": 7,
                "spreading_factor": 8,
                "coding_rate": 5
            },
            "hello": {
                "enabled": True,
                "interval": 45,
                "message_template": "CQ CQ from {node_name} ({callsign}) at {location}"
            },
            "serial": {
                "port": "/dev/ttyACM0",
                "baudrate": 115200,
                "timeout": 1
            }
        },
        
        "node2": {
            "station": {
                "callsign": "KX2TEST", 
                "node_name": "Remote Station",
                "location": "Field Test"
            },
            "radio": {
                "frequency": 433000000,
                "tx_power": 14,
                "bandwidth_index": 7,
                "spreading_factor": 8,
                "coding_rate": 5
            },
            "hello": {
                "enabled": True,
                "interval": 60,
                "message_template": "Hello from {node_name} ({callsign}) - {location}"
            },
            "serial": {
                "port": "/dev/ttyACM1", 
                "baudrate": 115200,
                "timeout": 1
            }
        },
        
        "longrange": {
            "station": {
                "callsign": "LR1TEST",
                "node_name": "Long Range Node",
                "location": "Mountain Top"
            },
            "radio": {
                "frequency": 433000000,
                "tx_power": 20,
                "bandwidth_index": 6,     # 62.5 kHz
                "spreading_factor": 12,   # SF12 for max range
                "coding_rate": 8          # 4/8 for max error correction
            },
            "hello": {
                "enabled": True,
                "interval": 120,
                "message_template": "Long range beacon from {node_name} ({callsign})"
            },
            "serial": {
                "port": "/dev/ttyACM0",
                "baudrate": 115200,
                "timeout": 1
            }
        },
        
        "highspeed": {
            "station": {
                "callsign": "HS1TEST",
                "node_name": "High Speed Node", 
                "location": "Urban Area"
            },
            "radio": {
                "frequency": 915000000,
                "tx_power": 17,
                "bandwidth_index": 9,     # 500 kHz
                "spreading_factor": 7,    # SF7 for max speed  
                "coding_rate": 5          # 4/5 for max speed
            },
            "hello": {
                "enabled": True,
                "interval": 30,
                "message_template": "High speed node {node_name} ({callsign}) online"
            },
            "serial": {
                "port": "/dev/ttyACM0",
                "baudrate": 115200,
                "timeout": 1
            }
        }
    }
    
    # Create config files
    for node_name, config in nodes.items():
        filename = f"config_{node_name}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)
        print(f"✅ Created {filename}")
    
    return nodes

def show_available_ports():
    """Show available serial ports"""
    import glob
    
    patterns = ['/dev/ttyACM*', '/dev/ttyUSB*']
    ports = []
    
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    
    if ports:
        print("\n📡 Available serial ports:")
        for port in sorted(ports):
            print(f"   {port}")
    else:
        print("\n⚠️  No serial ports found")
    
    return ports

def launch_node(config_name, port=None):
    """Launch a chat node with specified config"""
    config_file = f"config_{config_name}.json"
    
    if not os.path.exists(config_file):
        print(f"❌ Config file {config_file} not found")
        return None
    
    # Override port if specified
    if port:
        with open(config_file, 'r') as f:
            config = json.load(f)
        config['serial']['port'] = port
        temp_config = f"temp_{config_name}.json"
        with open(temp_config, 'w') as f:
            json.dump(config, f, indent=2)
        config_file = temp_config
    
    cmd = ["python3", "lora_chat.py", port or "", config_file]
    
    print(f"🚀 Launching {config_name} node...")
    print(f"   Config: {config_file}")
    if port:
        print(f"   Port: {port}")
    
    try:
        # Launch in new terminal if possible
        if os.environ.get('DISPLAY'):  # We have a display
            subprocess.Popen([
                "gnome-terminal", "--", "python3", "lora_chat.py", 
                port or "", config_file
            ])
        else:
            # No display, just run directly
            subprocess.Popen(cmd)
        
        return True
    except Exception as e:
        print(f"❌ Failed to launch: {e}")
        return False

def show_menu():
    """Show the launcher menu"""
    print("""
🚀 LoRa Chat Launcher
=====================

Available node configurations:

1. node1      - Base Station (433MHz, Standard settings)
2. node2      - Remote Station (433MHz, Standard settings)  
3. longrange  - Long Range Node (433MHz, SF12, 62.5kHz BW)
4. highspeed  - High Speed Node (915MHz, SF7, 500kHz BW)

Commands:
  launch <config> [port]  - Launch a node with config
  ports                   - Show available serial ports
  configs                 - Create/recreate config files
  dual                    - Launch two nodes for testing
  help                    - Show this menu
  quit                    - Exit launcher

Examples:
  launch node1
  launch node2 /dev/ttyACM1
  dual
""")

def main():
    """Main launcher interface"""
    print("🚀 LoRa Chat Launcher v1.0")
    print("=" * 40)
    
    # Create configs if they don't exist
    if not any(os.path.exists(f"config_{name}.json") for name in ["node1", "node2", "longrange", "highspeed"]):
        print("📝 Creating default configurations...")
        create_node_configs()
    
    show_available_ports()
    show_menu()
    
    while True:
        try:
            cmd_input = input("\n🎯 launcher> ").strip().split()
            if not cmd_input:
                continue
            
            cmd = cmd_input[0].lower()
            
            if cmd == "quit" or cmd == "exit":
                print("👋 Goodbye!")
                break
            
            elif cmd == "help":
                show_menu()
            
            elif cmd == "ports":
                show_available_ports()
            
            elif cmd == "configs":
                create_node_configs()
            
            elif cmd == "launch":
                if len(cmd_input) < 2:
                    print("❌ Usage: launch <config> [port]")
                    continue
                
                config_name = cmd_input[1]
                port = cmd_input[2] if len(cmd_input) > 2 else None
                launch_node(config_name, port)
            
            elif cmd == "dual":
                ports = show_available_ports()
                if len(ports) >= 2:
                    print("🔄 Launching dual node test...")
                    launch_node("node1", ports[0])
                    time.sleep(1)
                    launch_node("node2", ports[1])
                    print("✅ Dual nodes launched!")
                else:
                    print("❌ Need at least 2 serial ports for dual node test")
            
            else:
                print(f"❌ Unknown command '{cmd}'. Type 'help' for help.")
        
        except KeyboardInterrupt:
            print("\n👋 Interrupted, goodbye!")
            break
        except EOFError:
            print("\n👋 Goodbye!")
            break

if __name__ == "__main__":
    main()