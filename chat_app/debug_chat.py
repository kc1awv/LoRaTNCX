#!/usr/bin/env python3
"""
LoRaTNCX Debug Chat Application

Debug version with extensive logging to troubleshoot receive issues.
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
            print(f"[DEBUG] Parsing packet string: '{packet_str}'")
            
            if '>' in packet_str and ':' in packet_str:
                header, message = packet_str.split(':', 1)
                if '>' in header:
                    from_call, to_call = header.split('>', 1)
                    print(f"[DEBUG] Parsed: FROM={from_call}, TO={to_call}, MSG={message}")
                    return cls(from_call, to_call, message)
            
            print(f"[DEBUG] Could not parse packet format")
        except Exception as e:
            print(f"[DEBUG] Error parsing packet: {e}")
        return None

class DebugLoRaChat:
    """Debug version of chat application with extensive logging"""
    
    def __init__(self):
        self.serial_port = None
        self.callsign = ""
        self.node_name = ""
        self.running = False
        self.receive_queue = queue.Queue()
        self.hello_interval = 300
        self.last_hello = 0
        
        # Default radio config
        self.frequency = 915.0
        self.tx_power = 8
        self.bandwidth = 125.0
        self.spreading_factor = 9
        self.coding_rate = 7
        
        # Debug counters
        self.bytes_received = 0
        self.frames_received = 0
        self.data_frames_received = 0
    
    def setup_serial(self):
        """Setup serial connection to TNC"""
        ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1']
        
        print("Scanning for LoRaTNCX device...")
        for port in ports:
            try:
                ser = serial.Serial(port, 115200, timeout=1)
                time.sleep(2)
                print(f"[DEBUG] Connected to {port}")
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
                print(f"[DEBUG] Connected to {port}")
                self.serial_port = ser
                return True
            except Exception as e:
                print(f"[DEBUG] Failed to connect to {port}: {e}")
    
    def setup_user_info(self):
        """Get user information"""
        print("\n=== User Setup ===")
        self.node_name = input("Enter your node name: ").strip()
        self.callsign = input("Enter your callsign: ").strip().upper()
    
    def configure_tnc(self):
        """Send configuration commands to TNC"""
        if not self.serial_port:
            return
        
        print("[DEBUG] Configuring TNC...")
        
        try:
            # Set frequency
            freq_bytes = struct.pack('<f', self.frequency)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE, 
                                         bytes([KISSFrame.SET_FREQUENCY]) + freq_bytes)
            print(f"[DEBUG] Sending frequency frame: {frame.hex()}")
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            # Set TX power
            power_bytes = struct.pack('b', self.tx_power)
            frame = KISSFrame.create_frame(KISSFrame.SET_HARDWARE,
                                         bytes([KISSFrame.SET_TX_POWER]) + power_bytes)
            print(f"[DEBUG] Sending power frame: {frame.hex()}")
            self.serial_port.write(frame)
            time.sleep(0.1)
            
            print("[DEBUG] TNC configured successfully")
            
        except Exception as e:
            print(f"[DEBUG] Error configuring TNC: {e}")
    
    def send_message(self, message):
        """Send a chat message"""
        if not self.serial_port or not message.strip():
            return False
        
        try:
            chat_msg = ChatMessage(self.callsign, "ALL", message.strip())
            packet_data = chat_msg.to_packet()
            frame = KISSFrame.create_data_frame(packet_data)
            
            print(f"[DEBUG] Sending packet: {packet_data}")
            print(f"[DEBUG] Sending frame: {frame.hex()}")
            
            self.serial_port.write(frame)
            
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] {self.callsign}> {message}")
            return True
        except Exception as e:
            print(f"[DEBUG] Error sending message: {e}")
            return False
    
    def send_hello(self):
        """Send periodic hello packet"""
        hello_msg = f"Hello from {self.node_name} ({self.callsign})"
        if self.send_message(hello_msg):
            print("[HELLO] Beacon sent")
    
    def parse_kiss_frame(self, data):
        """Parse incoming KISS frame with debug output"""
        print(f"[DEBUG] Parsing frame data: {data.hex()} (length: {len(data)})")
        
        if len(data) < 1:
            print(f"[DEBUG] Frame too short")
            return None, None
        
        command = data[0]
        payload = data[1:] if len(data) > 1 else b''
        
        print(f"[DEBUG] Command byte: 0x{command:02x}")
        print(f"[DEBUG] Payload: {payload.hex()} ({len(payload)} bytes)")
        
        # Handle data frames
        if (command & 0x0F) == KISSFrame.DATA_FRAME:
            port = (command & 0xF0) >> 4
            print(f"[DEBUG] Data frame on port {port}")
            
            # Unescape the payload
            unescaped_payload = KISSFrame.unescape_data(payload)
            print(f"[DEBUG] Unescaped payload: {unescaped_payload.hex()}")
            print(f"[DEBUG] Unescaped as text: '{unescaped_payload.decode('utf-8', errors='ignore')}'")
            
            return 'data', unescaped_payload
        else:
            print(f"[DEBUG] Non-data frame, command: 0x{command:02x}")
        
        return 'command', (command, payload)
    
    def receive_thread(self):
        """Thread for receiving data from serial port with debug output"""
        frame_buffer = bytearray()
        in_frame = False
        raw_byte_count = 0
        
        print("[DEBUG] Receive thread started")
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    raw_byte_count += len(data)
                    
                    print(f"[DEBUG] Received {len(data)} bytes: {data.hex()}")
                    
                    for byte in data:
                        self.bytes_received += 1
                        
                        if byte == KISSFrame.FEND:
                            print(f"[DEBUG] FEND byte received")
                            if in_frame and len(frame_buffer) > 0:
                                print(f"[DEBUG] End of frame, buffer: {frame_buffer.hex()}")
                                self.frames_received += 1
                                
                                frame_type, frame_data = self.parse_kiss_frame(frame_buffer)
                                if frame_type == 'data':
                                    print(f"[DEBUG] Adding data frame to queue")
                                    self.data_frames_received += 1
                                    self.receive_queue.put(frame_data)
                                else:
                                    print(f"[DEBUG] Non-data frame received")
                                    
                                frame_buffer.clear()
                            else:
                                print(f"[DEBUG] Frame start marker")
                            in_frame = True
                        elif in_frame:
                            print(f"[DEBUG] Frame byte: 0x{byte:02x}")
                            frame_buffer.append(byte)
                        else:
                            print(f"[DEBUG] Byte outside frame: 0x{byte:02x}")
                
                time.sleep(0.01)
                
            except Exception as e:
                if self.running:
                    print(f"[DEBUG] Receive error: {e}")
                break
        
        print(f"[DEBUG] Receive thread ended. Stats: {raw_byte_count} raw bytes, {self.frames_received} frames, {self.data_frames_received} data frames")
    
    def process_received_messages(self):
        """Process messages from receive queue with debug output"""
        processed = 0
        while not self.receive_queue.empty():
            try:
                packet_data = self.receive_queue.get_nowait()
                processed += 1
                
                print(f"[DEBUG] Processing message {processed} from queue")
                print(f"[DEBUG] Raw packet data: {packet_data.hex()}")
                
                chat_msg = ChatMessage.from_packet(packet_data)
                
                if chat_msg:
                    timestamp = chat_msg.timestamp.strftime("%H:%M:%S")
                    print(f"[{timestamp}] {chat_msg.from_call}> {chat_msg.message}")
                else:
                    try:
                        raw_msg = packet_data.decode('utf-8', errors='ignore')
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        print(f"[{timestamp}] RAW> {raw_msg}")
                    except Exception as e:
                        print(f"[DEBUG] Could not decode as text: {e}")
                        
            except queue.Empty:
                break
            except Exception as e:
                print(f"[DEBUG] Error processing message: {e}")
        
        if processed > 0:
            print(f"[DEBUG] Processed {processed} messages from queue")
    
    def chat_loop(self):
        """Main chat interface loop with debug output"""
        print(f"\n=== Debug LoRaTNCX Chat Active ===")
        print(f"Node: {self.node_name} ({self.callsign})")
        print(f"Frequency: {self.frequency} MHz")
        print("Commands: /hello, /stats, /quit")
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
                    elif message == '/stats':
                        print(f"[STATS] Bytes received: {self.bytes_received}")
                        print(f"[STATS] Frames received: {self.frames_received}")
                        print(f"[STATS] Data frames: {self.data_frames_received}")
                        print(f"[STATS] Queue size: {self.receive_queue.qsize()}")
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
            print("\n[DEBUG] Shutting down...")
    
    def run(self):
        """Main application entry point"""
        print("LoRaTNCX Debug Chat v1.0")
        print("With extensive debug logging")
        print("=" * 40)
        
        if not self.setup_serial():
            print("[DEBUG] Failed to connect to TNC. Exiting.")
            return 1
        
        self.setup_user_info()
        self.configure_tnc()
        
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
        app = DebugLoRaChat()
        return app.run()
    except KeyboardInterrupt:
        print("\n[DEBUG] Interrupted by user")
        return 1
    except Exception as e:
        print(f"[DEBUG] Unexpected error: {e}")
        return 1

if __name__ == '__main__':
    sys.exit(main())