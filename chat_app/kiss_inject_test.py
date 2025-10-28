#!/usr/bin/env python3
"""
KISS Frame Injection Test

This script can inject test KISS frames to verify the chat app parsing works correctly.
"""

import serial
import time
import sys

def create_test_kiss_frame():
    """Create a test KISS frame with a sample message"""
    # Test message: "KC1TEST>ALL:Hello from test script"
    test_message = b"KC1TEST>ALL:Hello from test script"
    
    # KISS frame format: FEND DATA_FRAME [escaped_data] FEND
    FEND = 0xC0
    DATA_FRAME = 0x00
    FESC = 0xDB
    TFEND = 0xDC
    TFESC = 0xDD
    
    # Escape the data
    escaped_data = bytearray()
    for byte in test_message:
        if byte == FEND:
            escaped_data.extend([FESC, TFEND])
        elif byte == FESC:
            escaped_data.extend([FESC, TFESC])
        else:
            escaped_data.append(byte)
    
    # Build complete frame
    frame = bytearray()
    frame.append(FEND)
    frame.append(DATA_FRAME)
    frame.extend(escaped_data)
    frame.append(FEND)
    
    return bytes(frame)

def main():
    if len(sys.argv) < 2:
        print("Usage: python kiss_inject_test.py <serial_port> [message]")
        print("Example: python kiss_inject_test.py /dev/ttyUSB0")
        return 1
    
    port = sys.argv[1]
    
    # Custom message if provided
    if len(sys.argv) > 2:
        custom_msg = ' '.join(sys.argv[2:])
        test_message = f"KC1TEST>ALL:{custom_msg}".encode('utf-8')
        
        # Create frame with custom message
        FEND = 0xC0
        DATA_FRAME = 0x00
        frame = bytearray([FEND, DATA_FRAME])
        frame.extend(test_message)  # For simplicity, not escaping custom messages
        frame.append(FEND)
        test_frame = bytes(frame)
    else:
        test_frame = create_test_kiss_frame()
    
    try:
        print(f"Connecting to {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1)
        
        print(f"Injecting test KISS frame: {test_frame.hex()}")
        print(f"Frame content: {test_frame}")
        
        # Write the frame as if it came from the TNC
        # Note: This simulates what the TNC firmware would send when it receives a packet
        ser.write(test_frame)
        
        print("Test frame injected!")
        print("\nNow run your chat app and you should see the test message appear.")
        print("If it doesn't appear, there's a bug in the chat app's KISS parsing.")
        
        # Keep port open briefly to ensure data is sent
        time.sleep(1)
        
    except Exception as e:
        print(f"Error: {e}")
        return 1
    finally:
        if 'ser' in locals():
            ser.close()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())