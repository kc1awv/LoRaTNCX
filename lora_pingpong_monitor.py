#!/usr/bin/env python3
"""
LoRa Ping/Pong Test Monitor
A simple Python application to monitor and configure LoRa ping/pong devices.

Usage:
    python lora_pingpong_monitor.py [port]

Author: LoRaTNCX Project
Date: October 28, 2025
"""

import serial
import serial.tools.list_ports
import threading
import time
import sys
import re
from datetime import datetime

class LoRaPingPongMonitor:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_connection = None
        self.running = False
        self.stats = {
            'device_role': 'Unknown',
            'pings_sent': 0,
            'pongs_received': 0,
            'pongs_sent': 0,
            'success_rate': 0.0,
            'last_rssi': None,
            'last_snr': None,
            'uptime': 0
        }
        
    def find_serial_ports(self):
        """Find available serial ports"""
        ports = serial.tools.list_ports.comports()
        available_ports = []
        
        for port in ports:
            # Look for common ESP32 USB identifiers
            if any(keyword in port.description.lower() for keyword in 
                   ['usb', 'serial', 'cp210', 'ch340', 'ftdi']):
                available_ports.append(port.device)
                
        return available_ports
    
    def connect(self):
        """Connect to the serial port"""
        if not self.port:
            ports = self.find_serial_ports()
            if not ports:
                print("❌ No serial ports found!")
                return False
            
            print("Available serial ports:")
            for i, port in enumerate(ports):
                print(f"  {i+1}. {port}")
            
            try:
                choice = input("Select port (1-{}): ".format(len(ports)))
                self.port = ports[int(choice) - 1]
            except (ValueError, IndexError):
                print("❌ Invalid selection!")
                return False
        
        try:
            self.serial_connection = serial.Serial(
                self.port, 
                self.baudrate, 
                timeout=0.1
            )
            print(f"✅ Connected to {self.port} at {self.baudrate} baud")
            time.sleep(2)  # Wait for device to initialize
            return True
            
        except serial.SerialException as e:
            print(f"❌ Failed to connect to {self.port}: {e}")
            return False
    
    def parse_message(self, line):
        """Parse incoming serial messages and update statistics"""
        line = line.strip()
        
        # Device role detection
        if "Device Role: PING" in line:
            self.stats['device_role'] = 'PING'
        elif "Device Role: PONG" in line:
            self.stats['device_role'] = 'PONG'
        
        # Ping statistics
        ping_match = re.search(r'Sending ping #(\d+)', line)
        if ping_match:
            self.stats['pings_sent'] = int(ping_match.group(1))
        
        # Pong statistics
        pong_sent_match = re.search(r'Sending pong for ping #(\d+)', line)
        if pong_sent_match:
            self.stats['pongs_sent'] += 1
        
        # RSSI and SNR
        rssi_match = re.search(r'RSSI: ([-\d.]+) dBm', line)
        if rssi_match:
            self.stats['last_rssi'] = float(rssi_match.group(1))
        
        snr_match = re.search(r'SNR: ([-\d.]+) dB', line)
        if snr_match:
            self.stats['last_snr'] = float(snr_match.group(1))
        
        # Success rate
        success_match = re.search(r'Success rate: ([\d.]+)%', line)
        if success_match:
            self.stats['success_rate'] = float(success_match.group(1))
        
        # Pongs received (for ping devices)
        pongs_received_match = re.search(r'Pongs received: (\d+)', line)
        if pongs_received_match:
            self.stats['pongs_received'] = int(pongs_received_match.group(1))
        
        # Uptime
        uptime_match = re.search(r'Uptime: (\d+) seconds', line)
        if uptime_match:
            self.stats['uptime'] = int(uptime_match.group(1))
    
    def read_serial(self):
        """Read from serial port in a separate thread"""
        while self.running and self.serial_connection:
            try:
                if self.serial_connection.in_waiting:
                    line = self.serial_connection.readline().decode('utf-8', errors='ignore')
                    if line.strip():
                        # Print with timestamp
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        print(f"[{timestamp}] {line.strip()}")
                        
                        # Parse for statistics
                        self.parse_message(line)
                        
            except serial.SerialException:
                print("❌ Serial connection lost!")
                break
            except Exception as e:
                print(f"❌ Error reading serial: {e}")
                break
        
    def print_statistics(self):
        """Print current statistics"""
        print("\n" + "="*50)
        print("📊 STATISTICS")
        print("="*50)
        print(f"Device Role:     {self.stats['device_role']}")
        print(f"Uptime:          {self.stats['uptime']} seconds")
        
        if self.stats['device_role'] == 'PING':
            print(f"Pings Sent:      {self.stats['pings_sent']}")
            print(f"Pongs Received:  {self.stats['pongs_received']}")
            print(f"Success Rate:    {self.stats['success_rate']:.1f}%")
        elif self.stats['device_role'] == 'PONG':
            print(f"Pongs Sent:      {self.stats['pongs_sent']}")
        
        if self.stats['last_rssi'] is not None:
            print(f"Last RSSI:       {self.stats['last_rssi']:.1f} dBm")
        if self.stats['last_snr'] is not None:
            print(f"Last SNR:        {self.stats['last_snr']:.1f} dB")
        
        print("="*50)
    
    def run(self):
        """Main application loop"""
        print("🚀 LoRa Ping/Pong Monitor v1.0")
        print("Press Ctrl+C to exit")
        print("-" * 50)
        
        if not self.connect():
            return
        
        self.running = True
        
        # Start serial reading thread
        serial_thread = threading.Thread(target=self.read_serial, daemon=True)
        serial_thread.start()
        
        try:
            last_stats_time = 0
            
            while self.running:
                # Print statistics every 15 seconds
                if time.time() - last_stats_time > 15:
                    self.print_statistics()
                    last_stats_time = time.time()
                
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n👋 Exiting...")
        finally:
            self.running = False
            if self.serial_connection:
                self.serial_connection.close()

def main():
    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    monitor = LoRaPingPongMonitor(port)
    monitor.run()

if __name__ == "__main__":
    main()