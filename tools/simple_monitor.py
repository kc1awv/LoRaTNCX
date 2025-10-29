#!/usr/bin/env python3
"""
Simple TNC monitor for debugging connection issues
"""

import serial
import sys
import time
import threading
from datetime import datetime

class SimpleTNCMonitor:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.running = False
        
    def connect(self):
        """Connect to TNC"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1
            )
            print(f"Connected to {self.port}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def reader_thread(self):
        """Read from TNC and display"""
        while self.running:
            try:
                if self.serial_conn and self.serial_conn.in_waiting:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    text = data.decode('utf-8', errors='ignore')
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    
                    # Print each line with timestamp
                    for line in text.split('\n'):
                        if line.strip():
                            print(f"[{timestamp}] RX: {repr(line.strip())}")
                            
            except Exception as e:
                print(f"Read error: {e}")
                break
            time.sleep(0.01)
    
    def send_command(self, cmd):
        """Send command to TNC"""
        try:
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] TX: {repr(cmd)}")
            self.serial_conn.write((cmd + '\r\n').encode('utf-8'))
            return True
        except Exception as e:
            print(f"Send error: {e}")
            return False
    
    def run(self):
        """Main loop"""
        if not self.connect():
            return
        
        self.running = True
        
        # Start reader thread
        reader = threading.Thread(target=self.reader_thread)
        reader.daemon = True
        reader.start()
        
        print("TNC Monitor started. Type commands or 'quit' to exit.")
        print("Use 'connect <callsign>' to test connection...")
        
        try:
            while self.running:
                try:
                    cmd = input("> ").strip()
                    if cmd.lower() in ['quit', 'exit']:
                        break
                    if cmd:
                        self.send_command(cmd)
                except KeyboardInterrupt:
                    break
                except Exception as e:
                    print(f"Input error: {e}")
        except Exception as e:
            print(f"Main loop error: {e}")
        finally:
            self.running = False
            if self.serial_conn:
                self.serial_conn.close()
            print("Disconnected.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 simple_monitor.py <port>")
        print("Example: python3 simple_monitor.py /dev/ttyACM0")
        sys.exit(1)
    
    monitor = SimpleTNCMonitor(sys.argv[1])
    monitor.run()