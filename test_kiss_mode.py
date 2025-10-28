#!/usr/bin/env python3
"""
Test KISS SetHardware commands by first entering KISS mode
"""

import serial
import struct
import time
import threading
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
    
    print(f"LoRa SetHardware Test via KISS Mode")
    print(f"Connecting to {port}")
    print("=" * 50)
    
    try:
        with serial.Serial(port, 115200, timeout=2) as ser:
            time.sleep(2)
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Step 1: Enter KISS mode
            print("Step 1: Entering KISS mode...")
            ser.write(b'KISSM\r')
            time.sleep(1)
            
            # Check response
            response = b''
            start_time = time.time()
            while time.time() - start_time < 2:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    response += data
                    time.sleep(0.1)
                else:
                    time.sleep(0.1)
            
            print(f"KISS mode response: {response}")
            
            # Step 2: Send SetHardware commands
            print("\nStep 2: Sending SetHardware commands...")
            
            commands = [
                ("Frequency 433 MHz", LORA_HW_FREQUENCY, 433000000),
                ("TX Power 14 dBm", LORA_HW_TX_POWER, 14),
                ("Bandwidth 125 kHz", LORA_HW_BANDWIDTH, 7),
                ("Spreading Factor SF8", LORA_HW_SPREADING_FACTOR, 8),
                ("Coding Rate 4/6", LORA_HW_CODING_RATE, 6),
            ]
            
            for desc, param_id, value in commands:
                print(f"  Setting {desc}")
                cmd = create_sethardware_command(param_id, value)
                print(f"    KISS frame: {cmd.hex()}")
                ser.write(cmd)
                time.sleep(0.5)
                
                # Check for any response
                if ser.in_waiting > 0:
                    response = ser.read(ser.in_waiting)
                    print(f"    Response: {response}")
            
            # Step 3: Monitor for any additional output
            print("\nStep 3: Monitoring for debug output...")
            for i in range(10):
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        if text.strip():
                            print(f"Debug output: {repr(text)}")
                    except:
                        print(f"Raw data: {data.hex()}")
                time.sleep(0.5)
            
            print("\nTest completed!")
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()