#!/usr/bin/env python3
"""
Comprehensive LoRa SetHardware Test with Status Monitoring

This script tests KISS SetHardware commands and monitors TNC responses.
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

def send_tnc_command(ser, command):
    """Send a TNC command and get response"""
    print(f"Sending TNC command: {command}")
    ser.write((command + '\r').encode())
    time.sleep(0.5)
    
    response = b''
    start_time = time.time()
    while time.time() - start_time < 2:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            response += data
            time.sleep(0.1)
        else:
            time.sleep(0.1)
    
    try:
        text = response.decode('utf-8', errors='ignore')
        print(f"Response: {repr(text)}")
        return text
    except:
        print(f"Raw response: {response.hex()}")
        return ""

def send_kiss_command(ser, param_name, param_id, value):
    """Send a KISS SetHardware command"""
    print(f"\nSending KISS SetHardware: {param_name} = {value}")
    kiss_cmd = create_sethardware_command(param_id, value)
    print(f"KISS frame: {kiss_cmd.hex()}")
    ser.write(kiss_cmd)
    time.sleep(0.2)
    
    # Check for any immediate response
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting)
        try:
            text = response.decode('utf-8', errors='ignore')
            if text.strip():
                print(f"Immediate response: {repr(text)}")
        except:
            print(f"Raw response: {response.hex()}")

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM1'
    
    print(f"Comprehensive LoRa SetHardware Test")
    print(f"Connecting to {port}")
    print("=" * 50)
    
    try:
        with serial.Serial(port, 115200, timeout=2) as ser:
            time.sleep(2)
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Start with TNC banner
            send_tnc_command(ser, "BANNER")
            
            # Test frequency setting (433 MHz)
            send_kiss_command(ser, "Frequency", LORA_HW_FREQUENCY, 433000000)
            
            # Test TX power (10 dBm)
            send_kiss_command(ser, "TX Power", LORA_HW_TX_POWER, 10)
            
            # Test bandwidth (125 kHz = index 7)
            send_kiss_command(ser, "Bandwidth", LORA_HW_BANDWIDTH, 7)
            
            # Test spreading factor (SF8)
            send_kiss_command(ser, "Spreading Factor", LORA_HW_SPREADING_FACTOR, 8)
            
            # Test coding rate (4/6)
            send_kiss_command(ser, "Coding Rate", LORA_HW_CODING_RATE, 6)
            
            # Try to get help or status
            print("\n" + "="*50)
            print("Getting TNC status...")
            send_tnc_command(ser, "HELP")
            
            print("\nTest completed! Check above for any parameter confirmations.")
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()