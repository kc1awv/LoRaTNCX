#!/usr/bin/env python3
"""
Quick test script to verify TNC connection
"""

import serial
import sys
import time

def test_connection(port, baudrate=115200):
    """Test basic serial connection to TNC"""
    print(f"Testing connection to {port} at {baudrate} baud...")
    
    try:
        # Try to open the serial port
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            timeout=2,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS
        )
        
        print("✓ Serial port opened successfully")
        
        # Send a simple command
        ser.write(b'\r\n')
        time.sleep(0.5)
        
        # Try to read any response
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"✓ Received data from TNC: '{response.strip()}'")
        else:
            print("⚠ No immediate response from TNC (this may be normal)")
        
        # Send HELP command
        ser.write(b'HELP\r\n')
        time.sleep(1)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"✓ TNC responded to HELP command")
            print(f"Response preview: {response[:100]}...")
        else:
            print("⚠ No response to HELP command")
        
        ser.close()
        print("✓ Connection test completed successfully")
        return True
        
    except serial.SerialException as e:
        print(f"✗ Serial connection failed: {e}")
        return False
    except Exception as e:
        print(f"✗ Unexpected error: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 test_connection.py <port> [baudrate]")
        print("Example: python3 test_connection.py /dev/ttyACM0")
        sys.exit(1)
    
    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    
    success = test_connection(port, baudrate)
    sys.exit(0 if success else 1)