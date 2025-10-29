#!/usr/bin/env python3
"""
Test both KISS mode exit mechanisms
"""

import serial
import time
import sys

def test_esc_exit(ser):
    """Test ESC character exit from KISS mode"""
    print("\n=== Testing ESC (0x1B) Exit ===")
    
    # Enter KISS mode first
    print("Entering KISS mode...")
    kiss_frame = bytes([0xC0, 0x01, 0x28, 0xC0])  # TX Delay parameter
    ser.write(kiss_frame)
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"KISS entry response: {response.decode('utf-8', errors='ignore').strip()}")
    
    # Send ESC to exit
    print("Sending ESC (0x1B) to exit KISS mode...")
    ser.write(bytes([0x1B]))
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"ESC exit response: {response.decode('utf-8', errors='ignore').strip()}")
    
    # Test that we're back in command mode
    print("Testing command mode with HELP...")
    ser.write(b"HELP\r\n")
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        response_text = response.decode('utf-8', errors='ignore')
        if "LoRaTNCX" in response_text:
            print("✓ Successfully returned to command mode via ESC")
        else:
            print("✗ Failed to return to command mode")
        print(f"HELP response: {response_text[:100]}...")

def test_cmd_return_exit(ser):
    """Test CMD_RETURN (0xFF) exit from KISS mode"""
    print("\n=== Testing CMD_RETURN (0xFF) Exit ===")
    
    # Enter KISS mode first
    print("Entering KISS mode...")
    kiss_frame = bytes([0xC0, 0x02, 0x3F, 0xC0])  # Persistence parameter
    ser.write(kiss_frame)
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"KISS entry response: {response.decode('utf-8', errors='ignore').strip()}")
    
    # Send CMD_RETURN to exit
    print("Sending CMD_RETURN (0xFF) to exit KISS mode...")
    cmd_return_frame = bytes([0xC0, 0xFF, 0xC0])
    ser.write(cmd_return_frame)
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"CMD_RETURN exit response: {response.decode('utf-8', errors='ignore').strip()}")
    
    # Test that we're back in command mode
    print("Testing command mode with VER...")
    ser.write(b"VER\r\n")
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        response_text = response.decode('utf-8', errors='ignore')
        if "Version" in response_text or "LoRaTNCX" in response_text:
            print("✓ Successfully returned to command mode via CMD_RETURN")
        else:
            print("✗ Failed to return to command mode")
        print(f"VER response: {response_text[:100]}...")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 test_kiss_exit.py <port>")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Connected to {port}")
        
        # Wait for TNC to boot
        time.sleep(2)
        
        # Test both exit mechanisms
        test_esc_exit(ser)
        test_cmd_return_exit(ser)
        
        print("\n=== Test Summary ===")
        print("Both KISS exit mechanisms tested:")
        print("1. ESC (0x1B) - TAPR TNC2 standard escape character")
        print("2. CMD_RETURN (0xFF) - KISS protocol return command")
        
        ser.close()
        print("Test completed")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()