#!/usr/bin/env python3
"""
KISS Receive Test Tool
Tests the KISS TNC receive functionality in detail
"""

import serial
import time
import sys
import threading
from datetime import datetime

class KISSReceiveTest:
    def __init__(self, port='/dev/ttyACM0', baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        self.total_bytes = 0
        self.frames_received = 0
        
        # KISS constants
        self.FEND = 0xC0
        self.FESC = 0xDB
        self.TFEND = 0xDC
        self.TFESC = 0xDD
        
    def connect(self):
        """Connect to the TNC"""
        try:
            print(f"Connecting to {self.port} at {self.baud} baud...")
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            time.sleep(2)  # Wait for device to be ready
            print("Connected successfully!")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def unstuff_kiss_data(self, data):
        """Remove KISS byte stuffing"""
        result = bytearray()
        i = 0
        while i < len(data):
            if data[i] == self.FESC:
                if i + 1 < len(data):
                    if data[i + 1] == self.TFEND:
                        result.append(self.FEND)
                        i += 2
                    elif data[i + 1] == self.TFESC:
                        result.append(self.FESC)
                        i += 2
                    else:
                        result.append(data[i])
                        i += 1
                else:
                    result.append(data[i])
                    i += 1
            else:
                result.append(data[i])
                i += 1
        return bytes(result)
    
    def parse_kiss_frame(self, frame_data):
        """Parse a complete KISS frame"""
        if len(frame_data) < 2:
            return "INVALID: Frame too short"
        
        # First byte is command/port
        cmd_port = frame_data[0]
        port = (cmd_port >> 4) & 0x0F
        cmd = cmd_port & 0x0F
        
        cmd_names = {
            0: "DATA",
            1: "TXDELAY", 
            2: "PERSISTENCE",
            3: "SLOTTIME",
            4: "TXTAIL",
            5: "FULLDUPLEX",
            6: "SETHARDWARE",
            0xFF: "RETURN"
        }
        
        cmd_name = cmd_names.get(cmd, f"UNKNOWN({cmd})")
        
        if cmd == 0:  # Data frame
            payload = frame_data[1:]
            return f"DATA Frame (Port {port}): {len(payload)} bytes - {payload.hex()}"
        else:
            return f"{cmd_name} (Port {port}): {frame_data[1:].hex()}"
    
    def monitor_receive(self):
        """Monitor for incoming KISS frames"""
        print("\n=== KISS Receive Monitor ===")
        print("Waiting for incoming data...")
        print("Press Ctrl+C to stop")
        print("-" * 50)
        
        buffer = bytearray()
        in_frame = False
        frame_start = 0
        
        try:
            while self.running:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting)
                    self.total_bytes += len(data)
                    
                    # Print raw data
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    print(f"[{timestamp}] RX: {len(data)} bytes: {data.hex()}")
                    
                    # Add to buffer for frame parsing
                    buffer.extend(data)
                    
                    # Look for KISS frames
                    i = 0
                    while i < len(buffer):
                        if buffer[i] == self.FEND:
                            if in_frame:
                                # End of frame
                                frame_data = buffer[frame_start+1:i]
                                if len(frame_data) > 0:
                                    # Unstuff the frame
                                    unstuffed = self.unstuff_kiss_data(frame_data)
                                    print(f"[{timestamp}] KISS FRAME: {self.parse_kiss_frame(unstuffed)}")
                                    self.frames_received += 1
                                in_frame = False
                            else:
                                # Start of new frame
                                in_frame = True
                                frame_start = i
                        i += 1
                    
                    # Keep only data after last processed FEND
                    if in_frame and frame_start > 0:
                        buffer = buffer[frame_start:]
                    elif not in_frame:
                        buffer.clear()
                
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\nStopping monitor...")
            
    def send_test_commands(self):
        """Send test commands to TNC"""
        print("\nSending test commands...")
        
        # Exit KISS mode and back to see if TNC responds
        kiss_exit = bytes([0xC0, 0xFF, 0xC0])  # KISS Return command
        print(f"Sending KISS Return: {kiss_exit.hex()}")
        self.ser.write(kiss_exit)
        time.sleep(1)
        
        # Enter KISS mode
        kiss_enter = bytes([0xC0, 0x00, 0xC0])  # Empty data frame to enter KISS
        print(f"Sending KISS Enter: {kiss_enter.hex()}")
        self.ser.write(kiss_enter)
        time.sleep(1)
        
    def run_test(self):
        """Run the receive test"""
        if not self.connect():
            return False
            
        print("\n1. Sending initial test commands...")
        self.send_test_commands()
        
        print("\n2. Starting receive monitor...")
        self.running = True
        monitor_thread = threading.Thread(target=self.monitor_receive)
        monitor_thread.daemon = True
        monitor_thread.start()
        
        try:
            print("\nTest running - transmit packets from another device...")
            print("Statistics will be shown every 10 seconds")
            
            last_stats = time.time()
            while True:
                time.sleep(1)
                
                if time.time() - last_stats > 10:
                    print(f"\n--- Stats: {self.total_bytes} bytes, {self.frames_received} frames ---")
                    last_stats = time.time()
                    
        except KeyboardInterrupt:
            print("\nTest stopped by user")
        finally:
            self.running = False
            if self.ser:
                self.ser.close()
            print(f"\nFinal Stats:")
            print(f"  Total bytes received: {self.total_bytes}")
            print(f"  KISS frames parsed: {self.frames_received}")

if __name__ == "__main__":
    test = KISSReceiveTest()
    test.run_test()