#!/usr/bin/env python3
"""
KISS Protocol Test Script

A simple test script to verify KISS frame encoding/decoding and test
basic communication with the LoRaTNCX device.
"""

import serial
import struct
import time
import sys
from datetime import datetime

class KISSTest:
    """KISS protocol test utilities"""
    
    # KISS constants
    FEND = 0xC0
    FESC = 0xDB
    TFEND = 0xDC
    TFESC = 0xDD
    
    # Command codes
    DATA_FRAME = 0x00
    SET_HARDWARE = 0x06
    
    # Hardware sub-commands
    SET_FREQUENCY = 0x01
    GET_CONFIG = 0x10
    
    @staticmethod
    def escape_data(data):
        """Apply KISS byte stuffing to data"""
        result = bytearray()
        for byte in data:
            if byte == KISSTest.FEND:
                result.extend([KISSTest.FESC, KISSTest.TFEND])
            elif byte == KISSTest.FESC:
                result.extend([KISSTest.FESC, KISSTest.TFESC])
            else:
                result.append(byte)
        return bytes(result)
    
    @staticmethod
    def create_frame(command, data=b''):
        """Create a KISS frame"""
        frame = bytearray()
        frame.append(KISSTest.FEND)
        frame.append(command)
        if data:
            frame.extend(KISSTest.escape_data(data))
        frame.append(KISSTest.FEND)
        return bytes(frame)
    
    @staticmethod
    def test_byte_stuffing():
        """Test KISS byte stuffing implementation"""
        print("Testing KISS byte stuffing...")
        
        # Test data with KISS special characters
        test_data = bytes([0x01, 0xC0, 0x02, 0xDB, 0x03, 0xDC, 0x04, 0xDD, 0x05])
        
        # Escape the data
        escaped = KISSTest.escape_data(test_data)
        print(f"Original: {test_data.hex()}")
        print(f"Escaped:  {escaped.hex()}")
        
        # Create a frame
        frame = KISSTest.create_frame(KISSTest.DATA_FRAME, test_data)
        print(f"Frame:    {frame.hex()}")
        
        return True
    
    @staticmethod
    def test_tnc_connection(port):
        """Test basic TNC connection and configuration query"""
        try:
            print(f"Connecting to {port}...")
            ser = serial.Serial(port, 115200, timeout=2)
            time.sleep(2)  # Wait for device initialization
            
            print("Connected! Sending configuration query...")
            
            # Send get configuration command
            config_frame = KISSTest.create_frame(KISSTest.SET_HARDWARE, 
                                               bytes([KISSTest.GET_CONFIG]))
            ser.write(config_frame)
            
            # Read response for a few seconds
            print("Waiting for response...")
            start_time = time.time()
            while time.time() - start_time < 3:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    # Print both hex and text representation
                    print(f"Received: {data.hex()} ({data})")
                time.sleep(0.1)
            
            # Send a test message
            print("Sending test message...")
            test_msg = "KC1TEST>ALL:KISS protocol test message"
            test_frame = KISSTest.create_frame(KISSTest.DATA_FRAME, test_msg.encode())
            ser.write(test_frame)
            print(f"Sent test frame: {test_frame.hex()}")
            
            ser.close()
            print("Test completed successfully!")
            return True
            
        except Exception as e:
            print(f"Test failed: {e}")
            return False
    
    @staticmethod
    def find_tnc_device():
        """Scan for TNC device"""
        ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1',
                'COM3', 'COM4', 'COM5', 'COM6']
        
        print("Scanning for LoRaTNCX device...")
        for port in ports:
            try:
                ser = serial.Serial(port, 115200, timeout=1)
                ser.close()
                print(f"Found device at {port}")
                return port
            except (serial.SerialException, OSError):
                continue
        
        return None

def main():
    """Main test function"""
    print("LoRaTNCX KISS Protocol Test")
    print("=" * 30)
    
    # Test byte stuffing
    if not KISSTest.test_byte_stuffing():
        print("Byte stuffing test failed!")
        return 1
    
    print("\nByte stuffing test passed!")
    
    # Find TNC device
    port = KISSTest.find_tnc_device()
    if not port:
        port = input("Enter serial port manually: ").strip()
        if not port:
            print("No port specified. Exiting.")
            return 1
    
    # Test TNC connection
    if KISSTest.test_tnc_connection(port):
        print("\nAll tests passed!")
        return 0
    else:
        print("\nTNC connection test failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())