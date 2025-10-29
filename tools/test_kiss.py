#!/usr/bin/env python3
"""
Test KISS protocol with the TNC
"""

import serial
import time
import sys

def send_kiss_frame(ser, data):
    """Send a KISS data frame"""
    frame = bytearray()
    frame.append(0xC0)  # FEND
    frame.append(0x00)  # Data frame command
    
    # Add data with escaping
    for byte in data:
        if byte == 0xC0:
            frame.append(0xDB)  # FESC
            frame.append(0xDC)  # TFEND
        elif byte == 0xDB:
            frame.append(0xDB)  # FESC
            frame.append(0xDD)  # TFESC
        else:
            frame.append(byte)
    
    frame.append(0xC0)  # FEND
    
    print(f"Sending KISS frame: {frame.hex()}")
    ser.write(frame)
    time.sleep(0.1)

def send_kiss_parameter(ser, cmd, value):
    """Send a KISS parameter command"""
    frame = bytearray([0xC0, cmd, value, 0xC0])
    print(f"Sending KISS parameter {cmd:02X} = {value:02X}: {frame.hex()}")
    ser.write(frame)
    time.sleep(0.1)

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 test_kiss.py <port>")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Connected to {port}")
        
        # Wait for TNC to boot
        time.sleep(2)
        
        # Test KISS parameter commands (like Xastir sends)
        print("\nSending KISS parameters...")
        send_kiss_parameter(ser, 0x01, 0x28)  # TX Delay
        send_kiss_parameter(ser, 0x02, 0x3F)  # Persistence  
        send_kiss_parameter(ser, 0x03, 0x14)  # Slot Time
        send_kiss_parameter(ser, 0x04, 0x1E)  # TX Tail
        send_kiss_parameter(ser, 0x05, 0x00)  # Full Duplex
        
        # Send a test data frame
        print("\nSending test AX.25 frame...")
        test_ax25 = bytes([
            0x82, 0xa0, 0xb0, 0x64, 0x64, 0x64, 0x60,  # Destination
            0x96, 0x86, 0x62, 0x82, 0xae, 0xac, 0x60,  # Source  
            0xae, 0x92, 0x88, 0x8a, 0x64, 0x40, 0x65,  # Via path
            0x03, 0xf0,  # Control and PID
            # Info field: "=0000.00N/00000.00Wx"
            0x3d, 0x30, 0x30, 0x30, 0x30, 0x2e, 0x30, 0x30, 0x4e,
            0x2f, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2e, 0x30, 0x30, 0x57, 0x78
        ])
        
        send_kiss_frame(ser, test_ax25)
        
        # Read responses
        print("\nReading responses...")
        for i in range(10):
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                print(f"Response: {data}")
                if data:
                    # Print as hex for KISS frames
                    print(f"Hex: {data.hex()}")
            time.sleep(0.5)
        
        ser.close()
        print("Test completed")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()