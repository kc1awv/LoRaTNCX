#!/usr/bin/env python3
"""
Test Xastir initialization sequence with TNC
"""

import serial
import time
import sys

def test_xastir_init(ser):
    """Test the exact initialization sequence that Xastir sends"""
    print("Sending Xastir initialization sequence...")
    
    # Send the ESC @ k CR sequence
    init_sequence = bytes([0x1B, 0x40, 0x6B, 0x0D])
    print(f"Sending init: {init_sequence.hex()} (ESC @ k CR)")
    ser.write(init_sequence)
    time.sleep(0.5)
    
    # Read any response
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"Init response: {response}")
    
    # Now send KISS frame
    print("Sending KISS frame...")
    kiss_frame = bytes([0xC0])
    ser.write(kiss_frame)
    time.sleep(0.5)
    
    # Read response
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"KISS response: {response}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 test_xastir_init.py <port>")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Connected to {port}")
        
        # Wait for TNC to boot
        time.sleep(2)
        
        # Test the initialization sequence
        test_xastir_init(ser)
        
        # Send a KISS parameter to verify KISS mode is working
        print("\nTesting KISS parameter command...")
        param_frame = bytes([0xC0, 0x01, 0x28, 0xC0])  # TX Delay
        ser.write(param_frame)
        time.sleep(0.5)
        
        if ser.in_waiting:
            response = ser.read(ser.in_waiting)
            print(f"Parameter response: {response}")
        
        ser.close()
        print("Test completed")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()