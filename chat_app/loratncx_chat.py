#!/usr/bin/env python3
"""
LoRaTNCX Chat Application

A simple keyboard-to-keyboard chat application using the KISS TNC protocol.
This application provides a user interface for amateur radio packet chat
over LoRa using the LoRaTNCX device.
"""

import serial
import threading
import time
import struct
import json
import os
import sys
from datetime import datetime
from colorama import init, Fore, Back, Style
import queue

# Initialize colorama for cross-platform colored output
init()

class KISSFrame:
    """KISS protocol frame handler"""
    
    # KISS constants
    FEND = 0xC0
    FESC = 0xDB
    TFEND = 0xDC
    TFESC = 0xDD
    
    # Command codes
    DATA_FRAME = 0x00
    TX_DELAY = 0x01
    PERSISTENCE = 0x02 
    SLOT_TIME = 0x03
    SET_HARDWARE = 0x06
    
    # Hardware sub-commands
    SET_FREQUENCY = 0x01
    SET_TX_POWER = 0x02
    SET_BANDWIDTH = 0x03
    SET_SPREADING_FACTOR = 0x04
    SET_CODING_RATE = 0x05
    GET_CONFIG = 0x10
    
    @staticmethod
    def escape_data(data):
        """Apply KISS byte stuffing to data"""
        result = bytearray()
        for byte in data:
            if byte == KISSFrame.FEND:
                result.extend([KISSFrame.FESC, KISSFrame.TFEND])
            elif byte == KISSFrame.FESC:
                result.extend([KISSFrame.FESC, KISSFrame.TFESC])
            else:
                result.append(byte)
        return bytes(result)
    
    @staticmethod
    def unescape_data(data):
        """Remove KISS byte stuffing from data"""
        result = bytearray()
        i = 0
        while i < len(data):
            if data[i] == KISSFrame.FESC and i + 1 < len(data):
                if data[i + 1] == KISSFrame.TFEND:
                    result.append(KISSFrame.FEND)
                    i += 2
                elif data[i + 1] == KISSFrame.TFESC:
                    result.append(KISSFrame.FESC)
                    i += 2
                else:
                    result.append(data[i])
                    i += 1
            else:
                result.append(data[i])
                i += 1
        return bytes(result)
    
    @staticmethod
    def create_frame(command, data=b''):
        """Create a KISS frame"""
        frame = bytearray()
        frame.append(KISSFrame.FEND)
        frame.append(command)
        if data:
            frame.extend(KISSFrame.escape_data(data))
        frame.append(KISSFrame.FEND)
        return bytes(frame)
    
    @staticmethod
    def create_data_frame(data, port=0):
        """Create a data frame for transmission"""
        cmd = (port << 4) | KISSFrame.DATA_FRAME
        return KISSFrame.create_frame(cmd, data)

class RadioConfig:
    """Radio configuration management"""
    
    def __init__(self):
        self.frequency = 915.0  # MHz
        self.tx_power = 8       # dBm
        self.bandwidth = 125.0  # kHz
        self.spreading_factor = 9
        self.coding_rate = 7    # 4/7
        self.tx_delay = 30      # x 10ms
        self.persistence = 63   # ~25%
        self.slot_time = 10     # x 10ms
    
    def to_dict(self):
        return {
            'frequency': self.frequency,
            'tx_power': self.tx_power,
            'bandwidth': self.bandwidth,
            'spreading_factor': self.spreading_factor,
            'coding_rate': self.coding_rate,
            'tx_delay': self.tx_delay,
            'persistence': self.persistence,
            'slot_time': self.slot_time
        }
    
    def from_dict(self, config_dict):
        for key, value in config_dict.items():
            if hasattr(self, key):
                setattr(self, key, value)

class ChatMessage:
    """Represents a chat message"""
    
    def __init__(self, from_call, to_call, message, timestamp=None):
        self.from_call = from_call
        self.to_call = to_call
        self.message = message
        self.timestamp = timestamp or datetime.now()
    
    def to_packet(self):
        """Convert to packet format for transmission"""
        # Simple packet format: FROM>TO:MESSAGE
        return f"{self.from_call}>{self.to_call}:{self.message}".encode('utf-8')
    
    @classmethod
    def from_packet(cls, packet_data):
        """Parse packet data into ChatMessage"""
        try:
            packet_str = packet_data.decode('utf-8', errors='ignore')
            if '>' in packet_str and ':' in packet_str:
                header, message = packet_str.split(':', 1)
                if '>' in header:
                    from_call, to_call = header.split('>', 1)
                    return cls(from_call, to_call, message)
        except Exception as e:
            print(f"Error parsing packet: {e}")
        return None

class LoRaTNCXChat:
    """Main chat application class"""
    
    def __init__(self):
        self.serial_port = None
        self.config = RadioConfig()
        self.callsign = ""
        self.node_name = ""
        self.running = False
        self.receive_queue = queue.Queue()
        self.hello_interval = 300  # 5 minutes
        self.last_hello = 0
        
        # Load configuration if it exists
        self.load_config()
    
    def load_config(self):
        """Load configuration from file"""
        config_file = "chat_config.json"
        if os.path.exists(config_file):
            try:
                with open(config_file, 'r') as f:
                    config_data = json.load(f)
                    self.callsign = config_data.get('callsign', '')
                    self.node_name = config_data.get('node_name', '')
                    if 'radio_config' in config_data:
                        self.config.from_dict(config_data['radio_config'])
                    print(f"{Fore.GREEN}Loaded configuration from {config_file}{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.YELLOW}Warning: Could not load config: {e}{Style.RESET_ALL}")
    
    def save_config(self):
        """Save configuration to file"""
        config_file = "chat_config.json"
        try:
            config_data = {
                'callsign': self.callsign,
                'node_name': self.node_name,
                'radio_config': self.config.to_dict()
            }
            with open(config_file, 'w') as f:
                json.dump(config_data, f, indent=2)
            print(f"{Fore.GREEN}Configuration saved to {config_file}{Style.RESET_ALL}")
        except Exception as e:
            print(f"{Fore.RED}Error saving config: {e}{Style.RESET_ALL}")
    
    def setup_serial(self):
        """Setup serial connection to TNC"""
        ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1', 'COM3', 'COM4', 'COM5']
        
        print("Scanning for LoRaTNCX device...")
        for port in ports:
            try:
                ser = serial.Serial(port, 115200, timeout=1)
                time.sleep(2)  # Wait for device to initialize
                print(f"{Fore.GREEN}Connected to {port}{Style.RESET_ALL}")
                self.serial_port = ser
                return True
            except (serial.SerialException, OSError):
                continue
        
        # Manual port entry
        while True:
            port = input("Enter serial port manually (or 'quit' to exit): ").strip()
            if port.lower() == 'quit':
                return False
            try:
                ser = serial.Serial(port, 115200, timeout=1)
                time.sleep(2)
                print(f"{Fore.GREEN}Connected to {port}{Style.RESET_ALL}")
                self.serial_port = ser
                return True
            except Exception as e:
                print(f"{Fore.RED}Failed to connect to {port}: {e}{Style.RESET_ALL}")
    
    def setup_user_info(self):
        """Get user information"""
        print(f"\n{Style.BRIGHT}=== User Setup ==={Style.RESET_ALL}")
        
        if not self.node_name:
            self.node_name = input("Enter your node name: ").strip()
        else:
            new_name = input(f"Node name [{self.node_name}]: ").strip()
            if new_name:
                self.node_name = new_name
        
        if not self.callsign:
            self.callsign = input("Enter your callsign: ").strip().upper()
        else:
            new_call = input(f"Callsign [{self.callsign}]: ").strip().upper()
            if new_call:
                self.callsign = new_call
    
    def setup_radio(self):
        """Setup radio parameters"""
        print(f"\n{Style.BRIGHT}=== Radio Configuration ==={Style.RESET_ALL}")
        print("Current settings:")
        print(f"  Frequency: {self.config.frequency} MHz")
        print(f"  TX Power: {self.config.tx_power} dBm")
        print(f"  Bandwidth: {self.config.bandwidth} kHz")
        print(f"  Spreading Factor: {self.config.spreading_factor}")
        print(f"  Coding Rate: {self.config.coding_rate} (4/{self.config.coding_rate})")
        
        if input("\nChange radio settings? (y/N): ").lower().startswith('y'):
            try:
                freq = input(f"Frequency [{self.config.frequency}] MHz: ").strip()
                if freq:
                    self.config.frequency = float(freq)
                
                power = input(f"TX Power [{self.config.tx_power}] dBm: ").strip()
                if power:
                    self.config.tx_power = int(power)
                
                bw = input(f"Bandwidth [{self.config.bandwidth}] kHz: ").strip()
                if bw:
                    self.config.bandwidth = float(bw)
                
                sf = input(f"Spreading Factor [{self.config.spreading_factor}]: ").strip()
                if sf:
                    self.config.spreading_factor = int(sf)
                
                cr = input(f"Coding Rate [{self.config.coding_rate}]: ").strip()
                if cr:
                    self.config.coding_rate = int(cr)
                
            except ValueError as e:
                print(f"{Fore.RED}Invalid input: {e}{Style.RESET_ALL}")
                return False
        
        # Send configuration to TNC
        self.configure_tnc()
        return True
    
    def configure_tnc(self):
        """Send configuration commands to TNC"""
        if not self.serial_port:
            return
        
        print("Configuring TNC...")
        
        try:
            # Set frequency
            freq_bytes = struct.pack('<f', self.config.frequency)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE, 
                                         bytes([KISSFrame.SET_FREQUENCY]) + freq_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set TX power
            power_bytes = struct.pack('b', self.config.tx_power)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_TX_POWER]) + power_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set bandwidth
            bw_bytes = struct.pack('<f', self.config.bandwidth)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_BANDWIDTH]) + bw_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set spreading factor
            sf_bytes = bytes([self.config.spreading_factor])
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_SPREADING_FACTOR]) + sf_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set coding rate
            cr_bytes = bytes([self.config.coding_rate])
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_CODING_RATE]) + cr_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set TX delay
            frame = KISSFrame.create_frame(KISSFrame.TX_DELAY, bytes([self.config.tx_delay]))
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set persistence
            frame = KISSFrame.create_frame(KISSFrame.PERSISTENCE, bytes([self.config.persistence]))
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set slot time
            frame = KISSFrame.create_frame(KISSFrame.SLOT_TIME, bytes([self.config.slot_time]))
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            print(f"{Fore.GREEN}TNC configured successfully{Style.RESET_ALL}")
            
        except Exception as e:
            print(f"{Fore.RED}Error configuring TNC: {e}{Style.RESET_ALL}")
    
    def send_message(self, message):
        """Send a chat message"""
        if not self.serial_port or not message.strip():
            return False
        
        try:
            # Create chat message
            chat_msg = ChatMessage(self.callsign, "ALL", message.strip())
            packet_data = chat_msg.to_packet()
            
            # Create KISS frame
            frame = KISSFrame.create_data_frame(packet_data)
            
            # Send frame
            self.serial_port.write(frame)
            
            # Display sent message
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"{Fore.BLUE}[{timestamp}] {self.callsign}>{Style.RESET_ALL} {message}")
            
            return True
        except Exception as e:
            print(f"{Fore.RED}Error sending message: {e}{Style.RESET_ALL}")
            return False
    
    def send_hello(self):
        """Send periodic hello packet"""
        hello_msg = f"Hello from {self.node_name} ({self.callsign})"
        if self.send_message(hello_msg):
            print(f"{Fore.MAGENTA}[HELLO] Beacon sent{Style.RESET_ALL}")
    
    def parse_kiss_frame(self, data):
        """Parse incoming KISS frame"""
        if len(data) < 2:
            return None, None
        
        command = data[0]
        payload = data[1:] if len(data) > 1 else b''
        
        # Handle data frames
        if (command & 0x0F) == KISSFrame.DATA_FRAME:
            port = (command & 0xF0) >> 4
            # Unescape the payload
            unescaped_payload = KISSFrame.unescape_data(payload)
            return 'data', unescaped_payload
        
        return 'command', (command, payload)
    
    def receive_thread(self):
        """Thread for receiving data from serial port"""
        frame_buffer = bytearray()
        in_frame = False
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    
                    for byte in data:
                        if byte == KISSFrame.FEND:
                            if in_frame and len(frame_buffer) > 0:
                                # End of frame
                                frame_type, frame_data = self.parse_kiss_frame(frame_buffer)
                                if frame_type == 'data':
                                    self.receive_queue.put(frame_data)
                                frame_buffer.clear()
                            in_frame = True
                        elif in_frame:
                            frame_buffer.append(byte)
                
                time.sleep(0.01)  # Small delay to prevent busy waiting
                
            except Exception as e:
                if self.running:  # Only print error if we're still supposed to be running
                    print(f"{Fore.RED}Receive error: {e}{Style.RESET_ALL}")
                break
    
    def process_received_messages(self):
        """Process messages from receive queue"""
        while not self.receive_queue.empty():
            try:
                packet_data = self.receive_queue.get_nowait()
                chat_msg = ChatMessage.from_packet(packet_data)
                
                if chat_msg:
                    timestamp = chat_msg.timestamp.strftime("%H:%M:%S")
                    print(f"{Fore.GREEN}[{timestamp}] {chat_msg.from_call}>{Style.RESET_ALL} {chat_msg.message}")
                else:
                    # Display raw packet if we can't parse it
                    try:
                        raw_msg = packet_data.decode('utf-8', errors='ignore')
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        print(f"{Fore.YELLOW}[{timestamp}] RAW>{Style.RESET_ALL} {raw_msg}")
                    except:
                        pass
                        
            except queue.Empty:
                break
            except Exception as e:
                print(f"{Fore.RED}Error processing message: {e}{Style.RESET_ALL}")
    
    def chat_loop(self):
        """Main chat interface loop"""
        print(f"\n{Style.BRIGHT}=== LoRaTNCX Chat Active ==={Style.RESET_ALL}")
        print(f"Node: {self.node_name} ({self.callsign})")
        print(f"Frequency: {self.config.frequency} MHz")
        print("Type messages to send, or:")
        print("  /hello - Send hello beacon")
        print("  /config - Show current configuration")
        print("  /quit - Exit chat")
        print(f"{Style.DIM}{'='*50}{Style.RESET_ALL}")
        
        # Start receive thread
        self.running = True
        receive_thread = threading.Thread(target=self.receive_thread, daemon=True)
        receive_thread.start()
        
        # Send initial hello
        self.send_hello()
        self.last_hello = time.time()
        
        try:
            while self.running:
                # Process any received messages
                self.process_received_messages()
                
                # Send periodic hello
                current_time = time.time()
                if current_time - self.last_hello > self.hello_interval:
                    self.send_hello()
                    self.last_hello = current_time
                
                # Check for user input (non-blocking)
                try:
                    # Simple input handling - in a real app you might want to use
                    # something more sophisticated like curses or asyncio
                    message = input().strip()
                    
                    if message.startswith('/'):
                        if message == '/quit':
                            break
                        elif message == '/hello':
                            self.send_hello()
                        elif message == '/config':
                            self.show_config()
                        else:
                            print(f"{Fore.YELLOW}Unknown command: {message}{Style.RESET_ALL}")
                    elif message:
                        self.send_message(message)
                        
                except EOFError:
                    break
                except KeyboardInterrupt:
                    break
                    
        except KeyboardInterrupt:
            pass
        finally:
            self.running = False
            print(f"\n{Fore.YELLOW}Shutting down...{Style.RESET_ALL}")
    
    def show_config(self):
        """Display current configuration"""
        print(f"\n{Style.BRIGHT}=== Current Configuration ==={Style.RESET_ALL}")
        print(f"Node Name: {self.node_name}")
        print(f"Callsign: {self.callsign}")
        print(f"Frequency: {self.config.frequency} MHz")
        print(f"TX Power: {self.config.tx_power} dBm")
        print(f"Bandwidth: {self.config.bandwidth} kHz")
        print(f"Spreading Factor: {self.config.spreading_factor}")
        print(f"Coding Rate: {self.config.coding_rate} (4/{self.config.coding_rate})")
        print(f"TX Delay: {self.config.tx_delay} x 10ms")
        print(f"Persistence: {self.config.persistence}")
        print(f"Slot Time: {self.config.slot_time} x 10ms")
        print()
    
    def run(self):
        """Main application entry point"""
        print(f"{Style.BRIGHT}{Fore.CYAN}")
        print("╔══════════════════════════════════════╗")
        print("║         LoRaTNCX Chat v1.0           ║")
        print("║    Amateur Radio Packet Chat        ║")
        print("╚══════════════════════════════════════╝")
        print(Style.RESET_ALL)
        
        # Setup serial connection
        if not self.setup_serial():
            print(f"{Fore.RED}Failed to connect to TNC. Exiting.{Style.RESET_ALL}")
            return 1
        
        # Setup user information
        self.setup_user_info()
        
        # Setup radio parameters
        if not self.setup_radio():
            print(f"{Fore.RED}Radio setup failed. Exiting.{Style.RESET_ALL}")
            return 1
        
        # Save configuration
        self.save_config()
        
        # Start chat
        try:
            self.chat_loop()
        except KeyboardInterrupt:
            pass
        finally:
            if self.serial_port:
                self.serial_port.close()
        
        return 0

def main():
    """Application entry point"""
    try:
        app = LoRaTNCXChat()
        return app.run()
    except KeyboardInterrupt:
        print(f"\n{Fore.YELLOW}Interrupted by user{Style.RESET_ALL}")
        return 1
    except Exception as e:
        print(f"{Fore.RED}Unexpected error: {e}{Style.RESET_ALL}")
        return 1

if __name__ == '__main__':
    sys.exit(main())