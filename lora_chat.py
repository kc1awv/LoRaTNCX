#!/usr/bin/env python3
"""
LoRa KISS Keyboard-to-Keyboard Chat Application

A comprehensive chat application for testing LoRaTNCX devices with:
- Real-time keyboard-to-keyboard communication
- KISS SetHardware parameter control
- Node discovery via periodic hello packets
- Configuration management
- Multi-device support

Usage: python3 lora_chat.py [port] [config_file]
"""

import serial
import struct
import time
import threading
import json
import sys
import os
import select
from datetime import datetime
from pathlib import Path

# KISS Protocol Constants
FEND = 0xC0     # Frame End
FESC = 0xDB     # Frame Escape
TFEND = 0xDC    # Transposed Frame End
TFESC = 0xDD    # Transposed Frame Escape

# KISS Commands
KISS_CMD_DATA = 0x00
KISS_CMD_SETHARDWARE = 0x06

# LoRa SetHardware Parameter IDs
LORA_HW_FREQUENCY = 0x00
LORA_HW_TX_POWER = 0x01
LORA_HW_BANDWIDTH = 0x02
LORA_HW_SPREADING_FACTOR = 0x03
LORA_HW_CODING_RATE = 0x04

# Bandwidth mappings
BANDWIDTH_MAP = {
    0: 7.8, 1: 10.4, 2: 15.6, 3: 20.8, 4: 31.25,
    5: 41.7, 6: 62.5, 7: 125.0, 8: 250.0, 9: 500.0
}

class LoRaChatConfig:
    """Configuration management for LoRa chat application"""
    
    def __init__(self, config_file="lora_chat_config.json"):
        self.config_file = config_file
        self.default_config = {
            "station": {
                "callsign": "NOCALL",
                "node_name": "LoRa Node",
                "location": "Unknown"
            },
            "radio": {
                "frequency": 433000000,
                "tx_power": 14,
                "bandwidth_index": 7,  # 125 kHz
                "spreading_factor": 8,
                "coding_rate": 5
            },
            "hello": {
                "enabled": True,
                "interval": 30,  # seconds
                "message_template": "Hello from {node_name} ({callsign})"
            },
            "serial": {
                "port": "/dev/ttyACM0",
                "baudrate": 115200,
                "timeout": 1
            }
        }
        self.config = self.load_config()
    
    def load_config(self):
        """Load configuration from file or create default"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                # Merge with defaults for any missing keys
                return self._merge_config(self.default_config, config)
            else:
                self.save_config(self.default_config)
                return self.default_config.copy()
        except Exception as e:
            print(f"Error loading config: {e}, using defaults")
            return self.default_config.copy()
    
    def save_config(self, config=None):
        """Save configuration to file"""
        try:
            if config is None:
                config = self.config
            with open(self.config_file, 'w') as f:
                json.dump(config, f, indent=2)
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def _merge_config(self, default, user):
        """Recursively merge user config with defaults"""
        result = default.copy()
        for key, value in user.items():
            if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                result[key] = self._merge_config(result[key], value)
            else:
                result[key] = value
        return result

class KISSFrameHandler:
    """Handle KISS frame encoding/decoding and hardware commands"""
    
    @staticmethod
    def kiss_escape(data):
        """Escape special characters in KISS data"""
        result = []
        for byte in data:
            if byte == FEND:
                result.extend([FESC, TFEND])
            elif byte == FESC:
                result.extend([FESC, TFESC])
            else:
                result.append(byte)
        return bytes(result)
    
    @staticmethod
    def create_kiss_frame(command, data=b'', port=0):
        """Create a KISS frame with command and data"""
        cmd_port = (port << 4) | (command & 0x0F)
        frame = bytes([cmd_port])
        if data:
            frame += data
        frame = KISSFrameHandler.kiss_escape(frame)
        return bytes([FEND]) + frame + bytes([FEND])
    
    @staticmethod
    def create_data_frame(data, port=0):
        """Create a KISS data frame"""
        if isinstance(data, str):
            data = data.encode('utf-8')
        return KISSFrameHandler.create_kiss_frame(KISS_CMD_DATA, data, port)
    
    @staticmethod
    def create_sethardware_frame(param_id, value):
        """Create a SetHardware KISS frame"""
        if param_id == LORA_HW_FREQUENCY:
            data = struct.pack('<BI', param_id, value)
        else:
            data = struct.pack('<BB', param_id, value)
        return KISSFrameHandler.create_kiss_frame(KISS_CMD_SETHARDWARE, data)

class NodeDiscovery:
    """Handle node discovery and hello packets"""
    
    def __init__(self):
        self.discovered_nodes = {}
        self.last_hello_time = 0
    
    def add_discovered_node(self, callsign, node_name, location="Unknown", rssi=None):
        """Add or update a discovered node"""
        self.discovered_nodes[callsign] = {
            "node_name": node_name,
            "location": location,
            "last_seen": datetime.now(),
            "rssi": rssi
        }
    
    def get_nodes_list(self):
        """Get formatted list of discovered nodes"""
        if not self.discovered_nodes:
            return "No nodes discovered"
        
        lines = ["Discovered Nodes:"]
        for callsign, info in self.discovered_nodes.items():
            age = (datetime.now() - info['last_seen']).total_seconds()
            rssi_str = f" (RSSI: {info['rssi']} dBm)" if info['rssi'] else ""
            lines.append(f"  {callsign}: {info['node_name']} - {age:.0f}s ago{rssi_str}")
        return "\n".join(lines)
    
    def parse_hello_message(self, message):
        """Parse hello message to extract node info"""
        # Simple parsing for hello messages
        if "Hello from" in message:
            try:
                # Extract node name and callsign from "Hello from NodeName (CALLSIGN)"
                parts = message.split("Hello from ")[1]
                if "(" in parts and ")" in parts:
                    node_name = parts.split("(")[0].strip()
                    callsign = parts.split("(")[1].split(")")[0].strip()
                    return callsign, node_name
            except:
                pass
        return None, None

class LoRaChat:
    """Main LoRa chat application"""
    
    def __init__(self, port=None, config_file=None):
        self.config_manager = LoRaChatConfig(config_file or "lora_chat_config.json")
        self.config = self.config_manager.config
        
        # Override port if provided
        if port:
            self.config['serial']['port'] = port
        
        self.serial_port = None
        self.running = False
        self.discovery = NodeDiscovery()
        
        # Threading events
        self.shutdown_event = threading.Event()
        
        print("🚀 LoRa KISS Keyboard-to-Keyboard Chat")
        print("=" * 50)
    
    def connect_radio(self):
        """Connect to the LoRa TNC via serial"""
        try:
            self.serial_port = serial.Serial(
                self.config['serial']['port'],
                self.config['serial']['baudrate'],
                timeout=self.config['serial']['timeout']
            )
            time.sleep(2)  # Wait for device initialization
            print(f"✅ Connected to {self.config['serial']['port']}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def enter_kiss_mode(self):
        """Enter KISS mode on the TNC"""
        try:
            # Send KISSM command
            self.serial_port.write(b'KISSM\r\n')
            time.sleep(2)
            
            # Clear response buffer
            if self.serial_port.in_waiting > 0:
                response = self.serial_port.read(self.serial_port.in_waiting)
                if b"KISS mode" in response:
                    print("✅ Entered KISS mode")
                    return True
            
            print("⚠️  KISS mode entry unclear, continuing...")
            return True
        except Exception as e:
            print(f"❌ Failed to enter KISS mode: {e}")
            return False
    
    def configure_radio(self):
        """Configure radio parameters using KISS SetHardware commands"""
        print("\n🔧 Configuring radio parameters...")
        
        radio_config = self.config['radio']
        commands = [
            ("Frequency", LORA_HW_FREQUENCY, radio_config['frequency']),
            ("TX Power", LORA_HW_TX_POWER, radio_config['tx_power']),
            ("Bandwidth", LORA_HW_BANDWIDTH, radio_config['bandwidth_index']),
            ("Spreading Factor", LORA_HW_SPREADING_FACTOR, radio_config['spreading_factor']),
            ("Coding Rate", LORA_HW_CODING_RATE, radio_config['coding_rate'])
        ]
        
        for desc, param_id, value in commands:
            try:
                frame = KISSFrameHandler.create_sethardware_frame(param_id, value)
                self.serial_port.write(frame)
                time.sleep(0.1)  # Brief delay between commands
                
                # Format value for display
                if param_id == LORA_HW_FREQUENCY:
                    value_str = f"{value/1e6:.3f} MHz"
                elif param_id == LORA_HW_TX_POWER:
                    value_str = f"{value} dBm"
                elif param_id == LORA_HW_BANDWIDTH:
                    value_str = f"{BANDWIDTH_MAP.get(value, value)} kHz"
                elif param_id == LORA_HW_SPREADING_FACTOR:
                    value_str = f"SF{value}"
                elif param_id == LORA_HW_CODING_RATE:
                    value_str = f"4/{value}"
                else:
                    value_str = str(value)
                
                print(f"  ✅ {desc}: {value_str}")
                
            except Exception as e:
                print(f"  ❌ Failed to set {desc}: {e}")
        
        print("🔧 Radio configuration complete")
    
    def send_message(self, message):
        """Send a message via KISS"""
        try:
            # Add timestamp and station info
            timestamp = datetime.now().strftime("%H:%M:%S")
            callsign = self.config['station']['callsign']
            formatted_msg = f"[{timestamp}] {callsign}: {message}"
            
            frame = KISSFrameHandler.create_data_frame(formatted_msg)
            self.serial_port.write(frame)
            
            # Echo to local console
            print(f"📤 {formatted_msg}")
            return True
        except Exception as e:
            print(f"❌ Failed to send message: {e}")
            return False
    
    def receive_messages(self):
        """Background thread to receive and process messages"""
        rx_buffer = b''
        
        while not self.shutdown_event.is_set():
            try:
                if self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    rx_buffer += data
                    
                    # Look for complete KISS frames
                    while FEND in rx_buffer:
                        fend_pos = rx_buffer.find(FEND)
                        if fend_pos > 0:
                            rx_buffer = rx_buffer[fend_pos:]
                        
                        # Look for frame end
                        next_fend = rx_buffer.find(FEND, 1)
                        if next_fend > 0:
                            frame = rx_buffer[1:next_fend]  # Extract frame content
                            rx_buffer = rx_buffer[next_fend:]
                            
                            self.process_received_frame(frame)
                        else:
                            break
                
                time.sleep(0.01)  # Small delay to prevent CPU spinning
                
            except Exception as e:
                if not self.shutdown_event.is_set():
                    print(f"❌ Receive error: {e}")
                break
    
    def process_received_frame(self, frame):
        """Process a received KISS frame"""
        if len(frame) < 1:
            return
        
        cmd_port = frame[0]
        command = cmd_port & 0x0F
        port = (cmd_port >> 4) & 0x0F
        
        if command == KISS_CMD_DATA and len(frame) > 1:
            try:
                message = frame[1:].decode('utf-8', errors='ignore')
                print(f"📥 {message}")
                
                # Check if it's a hello message for node discovery
                callsign, node_name = self.discovery.parse_hello_message(message)
                if callsign and node_name:
                    self.discovery.add_discovered_node(callsign, node_name)
                
            except Exception as e:
                print(f"❌ Error processing message: {e}")
    
    def send_hello_packets(self):
        """Background thread to send periodic hello packets"""
        while not self.shutdown_event.is_set():
            try:
                if self.config['hello']['enabled']:
                    current_time = time.time()
                    if current_time - self.discovery.last_hello_time >= self.config['hello']['interval']:
                        hello_msg = self.config['hello']['message_template'].format(
                            node_name=self.config['station']['node_name'],
                            callsign=self.config['station']['callsign'],
                            location=self.config['station']['location']
                        )
                        
                        # Send hello packet (but don't echo to console)
                        frame = KISSFrameHandler.create_data_frame(hello_msg)
                        self.serial_port.write(frame)
                        
                        self.discovery.last_hello_time = current_time
                
                time.sleep(1)  # Check every second
                
            except Exception as e:
                if not self.shutdown_event.is_set():
                    print(f"❌ Hello packet error: {e}")
                break
    
    def print_help(self):
        """Print help information"""
        station = self.config['station']
        radio = self.config['radio']
        
        print(f"""
📋 LoRa Chat Commands:
==========================================
  /help                 - Show this help
  /config               - Show current configuration  
  /nodes                - Show discovered nodes
  /hello                - Send hello packet now
  /quit                 - Exit application
  /set <param> <value>  - Set radio parameter
  
📻 Radio Parameters:
  freq <Hz>            - Set frequency (e.g., /set freq 433000000)
  power <dBm>          - Set TX power (e.g., /set power 14)
  bw <index>           - Set bandwidth (e.g., /set bw 7 for 125kHz)  
  sf <value>           - Set spreading factor (e.g., /set sf 8)
  cr <value>           - Set coding rate (e.g., /set cr 5 for 4/5)

📡 Current Station:
  Callsign: {station['callsign']}
  Node: {station['node_name']}
  Location: {station['location']}

📻 Current Radio Config:
  Frequency: {radio['frequency']/1e6:.3f} MHz
  TX Power: {radio['tx_power']} dBm
  Bandwidth: {BANDWIDTH_MAP.get(radio['bandwidth_index'], radio['bandwidth_index'])} kHz
  Spreading Factor: SF{radio['spreading_factor']}
  Coding Rate: 4/{radio['coding_rate']}

Just type messages and press Enter to send!
==========================================
""")
    
    def handle_command(self, command):
        """Handle special commands starting with /"""
        parts = command[1:].split()
        if not parts:
            return
        
        cmd = parts[0].lower()
        
        if cmd == "help":
            self.print_help()
        
        elif cmd == "config":
            print(json.dumps(self.config, indent=2))
        
        elif cmd == "nodes":
            print(self.discovery.get_nodes_list())
        
        elif cmd == "hello":
            hello_msg = self.config['hello']['message_template'].format(
                node_name=self.config['station']['node_name'],
                callsign=self.config['station']['callsign'],
                location=self.config['station']['location']
            )
            self.send_message(hello_msg)
        
        elif cmd == "quit":
            print("👋 Goodbye!")
            self.shutdown_event.set()
            return False
        
        elif cmd == "set" and len(parts) >= 3:
            param = parts[1].lower()
            try:
                value = int(parts[2])
                self.set_radio_parameter(param, value)
            except ValueError:
                print("❌ Invalid value, must be a number")
        
        else:
            print("❌ Unknown command. Type /help for help")
        
        return True
    
    def set_radio_parameter(self, param, value):
        """Set a radio parameter via KISS SetHardware"""
        param_map = {
            'freq': (LORA_HW_FREQUENCY, 'frequency'),
            'power': (LORA_HW_TX_POWER, 'tx_power'), 
            'bw': (LORA_HW_BANDWIDTH, 'bandwidth_index'),
            'sf': (LORA_HW_SPREADING_FACTOR, 'spreading_factor'),
            'cr': (LORA_HW_CODING_RATE, 'coding_rate')
        }
        
        if param not in param_map:
            print(f"❌ Unknown parameter '{param}'")
            return
        
        param_id, config_key = param_map[param]
        
        try:
            frame = KISSFrameHandler.create_sethardware_frame(param_id, value)
            self.serial_port.write(frame)
            
            # Update config
            self.config['radio'][config_key] = value
            self.config_manager.save_config()
            
            print(f"✅ Set {param} to {value}")
        except Exception as e:
            print(f"❌ Failed to set {param}: {e}")
    
    def run(self):
        """Main application loop"""
        # Connect and configure
        if not self.connect_radio():
            return
        
        if not self.enter_kiss_mode():
            return
        
        self.configure_radio()
        
        # Start background threads
        rx_thread = threading.Thread(target=self.receive_messages, daemon=True)
        hello_thread = threading.Thread(target=self.send_hello_packets, daemon=True)
        
        rx_thread.start()
        hello_thread.start()
        
        self.print_help()
        print("\n🎯 Chat ready! Type messages or commands (/help for help)")
        print("=" * 50)
        
        # Main input loop
        try:
            while not self.shutdown_event.is_set():
                try:
                    # Use select for non-blocking input on Unix systems
                    if select.select([sys.stdin], [], [], 0.1)[0]:
                        user_input = sys.stdin.readline().strip()
                        
                        if not user_input:
                            continue
                        
                        if user_input.startswith('/'):
                            if not self.handle_command(user_input):
                                break
                        else:
                            self.send_message(user_input)
                
                except KeyboardInterrupt:
                    print("\n🛑 Interrupted by user")
                    break
                except EOFError:
                    print("\n🛑 Input closed")
                    break
        
        finally:
            self.shutdown_event.set()
            
            if self.serial_port:
                self.serial_port.close()
            
            print("👋 Chat session ended")

def main():
    """Main entry point"""
    port = sys.argv[1] if len(sys.argv) > 1 else None
    config_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    chat = LoRaChat(port, config_file)
    chat.run()

if __name__ == "__main__":
    main()