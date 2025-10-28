#!/usr/bin/env python3
"""
Radio Configuration Test
Tests radio settings and verifies configuration
"""

import serial
import time
import sys

def test_radio_config():
    """Test radio configuration via KISS commands"""
    
    port = '/dev/ttyACM0'
    baud = 115200
    
    try:
        print(f"Connecting to {port} at {baud} baud...")  
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2)
        print("Connected!")
        
        # Test KISS configuration commands
        print("\n=== Testing KISS Configuration ===")
        
        # These are the commands the chat app sends:
        configs = [
            ("Frequency (915.0 MHz)", bytes([0xC0, 0x06, 0x8F, 0x5C, 0x28, 0xF5, 0xC2, 0x8F, 0x5C, 0xC0])),
            ("TX Power (8 dBm)", bytes([0xC0, 0x16, 0x08, 0xC0])),
            ("Bandwidth (125.0 kHz)", bytes([0xC0, 0x26, 0x7D, 0x00, 0xC0])),
            ("Spreading Factor (9)", bytes([0xC0, 0x36, 0x09, 0xC0])),
            ("Coding Rate (7)", bytes([0xC0, 0x46, 0x07, 0xC0]))
        ]
        
        for name, cmd in configs:
            print(f"Setting {name}: {cmd.hex()}")
            ser.write(cmd)
            time.sleep(0.5)
            
            # Check for any response
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting)
                print(f"  Response: {response.hex()}")
            else:
                print("  No response")
        
        print("\n=== Radio Status Check ===")
        
        # Send a data frame to trigger any debug output (if not suppressed)
        test_frame = bytes([0xC0, 0x00]) + b"TEST" + bytes([0xC0])
        print(f"Sending test frame: {test_frame.hex()}")
        ser.write(test_frame)
        time.sleep(2)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)  
            print(f"Response: {response.hex()}")
        else:
            print("No response to test frame")
            
        print("\n=== Monitoring for 30 seconds ===")
        print("Please transmit from another device...")
        
        start_time = time.time()
        total_bytes = 0
        
        while time.time() - start_time < 30:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                total_bytes += len(data)
                timestamp = time.strftime("%H:%M:%S")
                print(f"[{timestamp}] Received {len(data)} bytes: {data.hex()}")
            time.sleep(0.1)
            
        print(f"\nTotal bytes received: {total_bytes}")
        
        ser.close()
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    test_radio_config()