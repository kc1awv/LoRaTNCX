#!/usr/bin/env python3
"""
LoRaTNCX Simple Chat Application

A minimal keyboard-to-keyboard chat application using the KISS TNC protocol.
This version has no external dependencies beyond pyserial.
"""

import serial
import threading
import time
import struct
import json
import os
import sys
from datetime import datetime
import queue

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

class ChatMessage:
    """Represents a chat message"""
    
    def __init__(self, from_call, to_call, message, timestamp=None):
        self.from_call = from_call
        self.to_call = to_call
        self.message = message
        self.timestamp = timestamp or datetime.now()
    
    def to_packet(self):
        """Convert to packet format for transmission"""
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

class SimpleLoRaChat:
    """Simple chat application class"""
    
    def __init__(self):
        self.serial_port = None
        self.callsign = ""
        self.node_name = ""
        self.running = False
        self.receive_queue = queue.Queue()
        self.hello_interval = 300  # 5 minutes
        self.last_hello = 0
        
        # Default radio config
        self.frequency = 915.0
        self.tx_power = 8
        self.bandwidth = 125.0
        self.spreading_factor = 9
        self.coding_rate = 7
    
    def setup_serial(self):
        """Setup serial connection to TNC"""
        ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1']
        
        print("Scanning for LoRaTNCX device...")
        for port in ports:
            try:
                ser = serial.Serial(port, 115200, timeout=1)
                time.sleep(2)
                print(f"Connected to {port}")
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
                print(f"Connected to {port}")
                self.serial_port = ser
                return True
            except Exception as e:
                print(f"Failed to connect to {port}: {e}")
    
    def setup_user_info(self):
        """Get user information"""
        print("\n=== User Setup ===")
        self.node_name = input("Enter your node name: ").strip()
        self.callsign = input("Enter your callsign: ").strip().upper()
    
    def setup_radio(self):
        """Setup radio parameters"""
        print("\n=== Radio Configuration ===")
        print(f"Frequency: {self.frequency} MHz")
        print(f"TX Power: {self.tx_power} dBm")
        print(f"Bandwidth: {self.bandwidth} kHz")
        print(f"Spreading Factor: {self.spreading_factor}")
        print(f"Coding Rate: {self.coding_rate}")
        
        if input("\nChange radio settings? (y/N): ").lower().startswith('y'):
            try:
                freq = input(f"Frequency [{self.frequency}] MHz: ").strip()
                if freq:
                    self.frequency = float(freq)
                
                power = input(f"TX Power [{self.tx_power}] dBm: ").strip()
                if power:
                    self.tx_power = int(power)
                
            except ValueError as e:
                print(f"Invalid input: {e}")
                return False
        
        self.configure_tnc()
        return True
    
    def configure_tnc(self):
        """Send configuration commands to TNC"""
        if not self.serial_port:
            return
        
        print("Configuring TNC...")
        
        try:
            # Set frequency
            freq_bytes = struct.pack('<f', self.frequency)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE, 
                                         bytes([KISSFrame.SET_FREQUENCY]) + freq_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set TX power
            power_bytes = struct.pack('b', self.tx_power)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_TX_POWER]) + power_bytes)
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            print("TNC configured successfully")
            
        except Exception as e:
            print(f"Error configuring TNC: {e}")
    
    def send_message(self, message):
        """Send a chat message"""
        if not self.serial_port or not message.strip():
            return False
        
        try:
            chat_msg = ChatMessage(self.callsign, "ALL", message.strip())
            packet_data = chat_msg.to_packet()
            frame = KISSFrame.create_data_frame(packet_data)
            self.serial_port.write(frame)
            
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] {self.callsign}> {message}")
            return True
        except Exception as e:
            print(f"Error sending message: {e}")
            return False
    
    def send_hello(self):
        """Send periodic hello packet"""
        hello_msg = f"Hello from {self.node_name} ({self.callsign})"
        if self.send_message(hello_msg):
            print("[HELLO] Beacon sent")
    
    def parse_kiss_frame(self, data):
        """Parse incoming KISS frame"""
        if len(data) < 2:
            return None, None
        
        command = data[0]
        payload = data[1:] if len(data) > 1 else b''
        
        if (command & 0x0F) == KISSFrame.DATA_FRAME:
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
                                frame_type, frame_data = self.parse_kiss_frame(frame_buffer)
                                if frame_type == 'data':
                                    self.receive_queue.put(frame_data)
                                frame_buffer.clear()
                            in_frame = True
                        elif in_frame:
                            frame_buffer.append(byte)
                
                time.sleep(0.01)
                
            except Exception as e:
                if self.running:
                    print(f"Receive error: {e}")
                break
    
    def process_received_messages(self):
        """Process messages from receive queue"""
        while not self.receive_queue.empty():
            try:
                packet_data = self.receive_queue.get_nowait()
                chat_msg = ChatMessage.from_packet(packet_data)
                
                if chat_msg:
                    timestamp = chat_msg.timestamp.strftime("%H:%M:%S")
                    print(f"[{timestamp}] {chat_msg.from_call}> {chat_msg.message}")
                else:
                    try:
                        raw_msg = packet_data.decode('utf-8', errors='ignore')
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        print(f"[{timestamp}] RAW> {raw_msg}")
                    except:
                        pass
                        
            except queue.Empty:
                break
            except Exception as e:
                print(f"Error processing message: {e}")
    
    def chat_loop(self):
        """Main chat interface loop"""
        print(f"\n=== LoRaTNCX Chat Active ===")
        print(f"Node: {self.node_name} ({self.callsign})")
        print(f"Frequency: {self.frequency} MHz")
        print("Type messages to send, '/hello' for beacon, '/quit' to exit")
        print("=" * 50)
        
        self.running = True
        receive_thread = threading.Thread(target=self.receive_thread, daemon=True)
        receive_thread.start()
        
        self.send_hello()
        self.last_hello = time.time()
        
        try:
            while self.running:
                self.process_received_messages()
                
                current_time = time.time()
                if current_time - self.last_hello > self.hello_interval:
                    self.send_hello()
                    self.last_hello = current_time
                
                try:
                    message = input().strip()
                    
                    if message == '/quit':
                        break
                    elif message == '/hello':
                        self.send_hello()
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
            print("\nShutting down...")
    
    def run(self):
        """Main application entry point"""
        print("LoRaTNCX Simple Chat v1.0")
        print("Amateur Radio Packet Chat")
        print("=" * 40)
        
        if not self.setup_serial():
            print("Failed to connect to TNC. Exiting.")
            return 1
        
        self.setup_user_info()
        
        if not self.setup_radio():
            print("Radio setup failed. Exiting.")
            return 1
        
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
        app = SimpleLoRaChat()
        return app.run()
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        return 1
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1

if __name__ == '__main__':
    sys.exit(main())