#!/usr/bin/env python3
"""
Simple KISS receive monitor - just shows what's coming from the serial port
"""

import serial
import time
import sys

def hex_dump(data):
    """Format data as hex dump"""
    hex_str = ' '.join(f'{b:02x}' for b in data)
    ascii_str = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in data)
    return f"{hex_str:<48} |{ascii_str}|"

def main():
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = '/dev/ttyUSB0'  # Default port
    
    try:
        print(f"Opening {port} at 115200 baud...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for device initialization
        
        print("Monitoring serial port for incoming data...")
        print("Press Ctrl+C to exit")
        print("=" * 80)
        
        byte_count = 0
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                byte_count += len(data)
                
                timestamp = time.strftime("%H:%M:%S")
                print(f"[{timestamp}] Received {len(data)} bytes (total: {byte_count}):")
                print(f"  {hex_dump(data)}")
                
                # Look for KISS frames
                for i, byte in enumerate(data):
                    if byte == 0xC0:  # FEND
                        print(f"  FEND at offset {i}")
                
                print()
            
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == '__main__':
    main()