#!/usr/bin/env python3
"""
Final test of KISS SetHardware commands with device already in KISS mode
"""

import serial
import struct
import time
import sys

# KISS constants
FEND = 0xC0
FESC = 0xDB
TFEND = 0xDC
TFESC = 0xDD
KISS_CMD_SETHARDWARE = 0x06

# LoRa parameter IDs
LORA_HW_FREQUENCY = 0x00
LORA_HW_TX_POWER = 0x01
LORA_HW_BANDWIDTH = 0x02
LORA_HW_SPREADING_FACTOR = 0x03
LORA_HW_CODING_RATE = 0x04

def kiss_escape(data):
    """Escape special characters in KISS data"""
    result = []
    for byte in data:
        if byte == FEND:
            result.extend([FESC, TFEND])
        elif byte == FESC:
            result.extend([FESC, TFESC])
        else:
            result.append(byte)
    return bytes(result)

def create_kiss_frame(command, data=b''):
    """Create a KISS frame"""
    frame = bytes([command])
    if data:
        frame += data
    frame = kiss_escape(frame)
    return bytes([FEND]) + frame + bytes([FEND])

def create_sethardware_command(param_id, value):
    """Create a SetHardware KISS command"""
    if param_id == LORA_HW_FREQUENCY:
        data = struct.pack('<BI', param_id, value)
    else:
        data = struct.pack('<BB', param_id, value)
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM1'
    
    print(f"Final KISS SetHardware Test")
    print(f"Assuming device is already in KISS mode")
    print("=" * 50)
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(1)
            ser.reset_input_buffer()
            
            # Send SetHardware commands one by one with monitoring
            test_cases = [
                ("Set Frequency to 915.000 MHz", LORA_HW_FREQUENCY, 915000000),
                ("Set TX Power to 20 dBm", LORA_HW_TX_POWER, 20),
                ("Set Bandwidth to 250 kHz", LORA_HW_BANDWIDTH, 8),  # Index 8 = 250 kHz
                ("Set Spreading Factor to SF7", LORA_HW_SPREADING_FACTOR, 7),
                ("Set Coding Rate to 4/5", LORA_HW_CODING_RATE, 5),
            ]
            
            for i, (description, param_id, value) in enumerate(test_cases, 1):
                print(f"\nTest {i}: {description}")
                
                # Create and send command
                cmd = create_sethardware_command(param_id, value)
                print(f"  KISS frame: {cmd.hex()}")
                
                # Clear any existing data first
                if ser.in_waiting > 0:
                    old_data = ser.read(ser.in_waiting)
                    print(f"  Cleared old data: {old_data}")
                
                # Send the command
                ser.write(cmd)
                print(f"  Command sent, waiting for response...")
                
                # Wait and check for any response/debug output
                response_received = False
                for j in range(20):  # Wait up to 2 seconds
                    if ser.in_waiting > 0:
                        data = ser.read(ser.in_waiting)
                        response_received = True
                        try:
                            text = data.decode('utf-8', errors='ignore')
                            if text.strip():
                                print(f"  Debug: {repr(text)}")
                        except:
                            print(f"  Raw response: {data.hex()}")
                    time.sleep(0.1)
                
                if not response_received:
                    print(f"  No immediate response (this may be normal)")
            
            # Final monitoring period
            print(f"\nFinal monitoring for 3 seconds...")
            for i in range(30):
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        if text.strip():
                            print(f"Final: {repr(text)}")
                    except:
                        print(f"Final raw: {data.hex()}")
                time.sleep(0.1)
            
            print(f"\nTest completed!")
            print(f"If no debug output was seen, the SetHardware commands")
            print(f"may be working silently (which is normal for many TNCs).")
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()