#!/usr/bin/env python3
"""
LoRa SetHardware KISS Command Test Script

This script tests the KISS SetHardware commands for controlling LoRa radio parameters.
It demonstrates how to change frequency, TX power, bandwidth, spreading factor, and coding rate.
"""

import serial
import struct
import time
import sys

# KISS frame constants
FEND = 0xC0  # Frame End
FESC = 0xDB  # Frame Escape
TFEND = 0xDC  # Transposed Frame End
TFESC = 0xDD  # Transposed Frame Escape

# KISS commands
KISS_CMD_DATA = 0x00
KISS_CMD_TXDELAY = 0x01
KISS_CMD_PERSISTENCE = 0x02
KISS_CMD_SLOTTIME = 0x03
KISS_CMD_TXTAIL = 0x04
KISS_CMD_FULLDUPLEX = 0x05
KISS_CMD_SETHARDWARE = 0x06

# LoRa SetHardware parameter IDs
LORA_HW_FREQUENCY = 0x00
LORA_HW_TX_POWER = 0x01
LORA_HW_BANDWIDTH = 0x02
LORA_HW_SPREADING_FACTOR = 0x03
LORA_HW_CODING_RATE = 0x04

# Bandwidth index values
LORA_BW_7_8 = 0     # 7.8 kHz
LORA_BW_10_4 = 1    # 10.4 kHz
LORA_BW_15_6 = 2    # 15.6 kHz
LORA_BW_20_8 = 3    # 20.8 kHz
LORA_BW_31_25 = 4   # 31.25 kHz
LORA_BW_41_7 = 5    # 41.7 kHz
LORA_BW_62_5 = 6    # 62.5 kHz
LORA_BW_125_0 = 7   # 125 kHz
LORA_BW_250_0 = 8   # 250 kHz
LORA_BW_500_0 = 9   # 500 kHz

BANDWIDTH_NAMES = {
    0: "7.8 kHz", 1: "10.4 kHz", 2: "15.6 kHz", 3: "20.8 kHz", 4: "31.25 kHz",
    5: "41.7 kHz", 6: "62.5 kHz", 7: "125 kHz", 8: "250 kHz", 9: "500 kHz"
}

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
    """Create a KISS frame with command and data"""
    frame = bytes([command])
    if data:
        frame += data
    frame = kiss_escape(frame)
    return bytes([FEND]) + frame + bytes([FEND])

def create_sethardware_command(param_id, value):
    """Create a SetHardware KISS command"""
    if param_id == LORA_HW_FREQUENCY:
        # Frequency as 4-byte little-endian Hz value
        data = struct.pack('<BI', param_id, value)
    elif param_id == LORA_HW_TX_POWER:
        # TX Power as 1-byte dBm value
        data = struct.pack('<BB', param_id, value)
    elif param_id in [LORA_HW_BANDWIDTH, LORA_HW_SPREADING_FACTOR, LORA_HW_CODING_RATE]:
        # Other parameters as 1-byte values
        data = struct.pack('<BB', param_id, value)
    else:
        raise ValueError(f"Unknown parameter ID: {param_id}")
    
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def test_lora_parameters(ser):
    """Test various LoRa parameter configurations"""
    
    print("Testing LoRa SetHardware Commands via KISS Protocol")
    print("=" * 50)
    
    # Test 1: Set frequency to 915 MHz (915000000 Hz)
    print("\nTest 1: Setting frequency to 915 MHz")
    freq_cmd = create_sethardware_command(LORA_HW_FREQUENCY, 915000000)
    ser.write(freq_cmd)
    print(f"Sent: {freq_cmd.hex()}")
    time.sleep(0.1)
    
    # Test 2: Set TX power to 14 dBm
    print("\nTest 2: Setting TX power to 14 dBm")
    power_cmd = create_sethardware_command(LORA_HW_TX_POWER, 14)
    ser.write(power_cmd)
    print(f"Sent: {power_cmd.hex()}")
    time.sleep(0.1)
    
    # Test 3: Set bandwidth to 125 kHz
    print("\nTest 3: Setting bandwidth to 125 kHz")
    bw_cmd = create_sethardware_command(LORA_HW_BANDWIDTH, LORA_BW_125_0)
    ser.write(bw_cmd)
    print(f"Sent: {bw_cmd.hex()}")
    time.sleep(0.1)
    
    # Test 4: Set spreading factor to SF7
    print("\nTest 4: Setting spreading factor to SF7")
    sf_cmd = create_sethardware_command(LORA_HW_SPREADING_FACTOR, 7)
    ser.write(sf_cmd)
    print(f"Sent: {sf_cmd.hex()}")
    time.sleep(0.1)
    
    # Test 5: Set coding rate to 4/5
    print("\nTest 5: Setting coding rate to 4/5")
    cr_cmd = create_sethardware_command(LORA_HW_CODING_RATE, 5)
    ser.write(cr_cmd)
    print(f"Sent: {cr_cmd.hex()}")
    time.sleep(0.1)
    
    # Test 6: Try different bandwidth settings
    print("\nTest 6: Testing different bandwidth settings")
    for bw_idx in [LORA_BW_62_5, LORA_BW_125_0, LORA_BW_250_0]:
        print(f"  Setting bandwidth to {BANDWIDTH_NAMES[bw_idx]}")
        bw_cmd = create_sethardware_command(LORA_HW_BANDWIDTH, bw_idx)
        ser.write(bw_cmd)
        print(f"  Sent: {bw_cmd.hex()}")
        time.sleep(0.2)
    
    # Test 7: Try different spreading factors
    print("\nTest 7: Testing different spreading factors")
    for sf in [7, 8, 9, 10]:
        print(f"  Setting spreading factor to SF{sf}")
        sf_cmd = create_sethardware_command(LORA_HW_SPREADING_FACTOR, sf)
        ser.write(sf_cmd)
        print(f"  Sent: {sf_cmd.hex()}")
        time.sleep(0.2)
    
    print("\nSetHardware command tests completed!")
    print("Check the TNC console output for parameter change confirmations.")

def monitor_responses(ser):
    """Monitor serial port for responses"""
    print("\nMonitoring for responses (Ctrl+C to stop)...")
    try:
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                print(f"Received: {data}")
                # Also try to decode as text
                try:
                    text = data.decode('utf-8', errors='ignore')
                    if text.strip():
                        print(f"Text: {repr(text)}")
                except:
                    pass
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nMonitoring stopped.")

def main():
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = '/dev/ttyACM1'  # Default port
    
    print(f"Connecting to LoRaTNCX on {port}")
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(2)  # Wait for device to be ready
            
            # Clear any existing data
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Run parameter tests
            test_lora_parameters(ser)
            
            # Monitor for responses
            monitor_responses(ser)
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        print("Make sure the device is connected and the port is correct.")
    except KeyboardInterrupt:
        print("\nTest interrupted by user.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()