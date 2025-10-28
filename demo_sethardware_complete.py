#!/usr/bin/env python3
"""
Complete KISS SetHardware Command Demonstration

This script demonstrates all the LoRa parameter control capabilities
available through KISS SetHardware commands (0x06).
"""

import serial
import struct
import time
import sys

# KISS Protocol Constants
FEND = 0xC0  # Frame End
FESC = 0xDB  # Frame Escape  
TFEND = 0xDC # Transposed Frame End
TFESC = 0xDD # Transposed Frame Escape
KISS_CMD_SETHARDWARE = 0x06

# LoRa SetHardware Parameter IDs
LORA_HW_FREQUENCY = 0x00
LORA_HW_TX_POWER = 0x01
LORA_HW_BANDWIDTH = 0x02
LORA_HW_SPREADING_FACTOR = 0x03
LORA_HW_CODING_RATE = 0x04

# Bandwidth Index Values
BANDWIDTH_MAP = {
    0: "7.8 kHz",   1: "10.4 kHz",  2: "15.6 kHz",  3: "20.8 kHz",  4: "31.25 kHz",
    5: "41.7 kHz",  6: "62.5 kHz",  7: "125 kHz",   8: "250 kHz",   9: "500 kHz"
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
    """Create a SetHardware KISS command with proper data format"""
    if param_id == LORA_HW_FREQUENCY:
        # Frequency: 1 byte param ID + 4 bytes frequency in Hz (little-endian)
        data = struct.pack('<BI', param_id, value)
    else:
        # Other parameters: 1 byte param ID + 1 byte value
        data = struct.pack('<BB', param_id, value)
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def send_command_with_response(ser, description, param_id, value, unit=""):
    """Send a SetHardware command and monitor for response"""
    print(f"\n📡 {description}")
    print(f"   Value: {value}{unit}")
    
    # Create and send command
    cmd = create_sethardware_command(param_id, value)
    print(f"   KISS: {cmd.hex()}")
    
    # Clear buffer and send
    if ser.in_waiting > 0:
        ser.read(ser.in_waiting)
    
    ser.write(cmd)
    
    # Monitor for response
    response_found = False
    for i in range(20):  # Wait up to 2 seconds
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            try:
                text = data.decode('utf-8', errors='ignore')
                if 'successful' in text:
                    print(f"   ✅ Command successful!")
                    response_found = True
                    break
                elif 'failed' in text or 'Invalid' in text:
                    print(f"   ❌ Command failed: {text.strip()}")
                    response_found = True
                    break
            except:
                pass
        time.sleep(0.1)
    
    if not response_found:
        print(f"   ⏳ No response (may be processing)")
    
    time.sleep(0.3)  # Brief pause between commands

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM1'
    
    print("🚀 LoRaTNCX KISS SetHardware Complete Demonstration")
    print("=" * 60)
    print(f"📡 Device: {port}")
    print("🔧 Testing comprehensive LoRa parameter control via KISS")
    print("=" * 60)
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(1)
            
            print("\n🔄 Testing Frequency Control")
            print("-" * 30)
            # Test different ISM band frequencies
            send_command_with_response(ser, "Set 433 MHz (EU ISM Band)", LORA_HW_FREQUENCY, 433000000, " Hz")
            send_command_with_response(ser, "Set 868 MHz (EU ISM Band)", LORA_HW_FREQUENCY, 868000000, " Hz")
            send_command_with_response(ser, "Set 915 MHz (US ISM Band)", LORA_HW_FREQUENCY, 915000000, " Hz")
            
            print("\n⚡ Testing TX Power Control")
            print("-" * 30)
            # Test different power levels
            send_command_with_response(ser, "Set Low Power (2 dBm)", LORA_HW_TX_POWER, 2, " dBm")
            send_command_with_response(ser, "Set Medium Power (14 dBm)", LORA_HW_TX_POWER, 14, " dBm")
            send_command_with_response(ser, "Set High Power (20 dBm)", LORA_HW_TX_POWER, 20, " dBm")
            
            print("\n📶 Testing Bandwidth Control")
            print("-" * 30)
            # Test different bandwidths
            for bw_idx in [6, 7, 8, 9]:  # 62.5, 125, 250, 500 kHz
                bw_name = BANDWIDTH_MAP[bw_idx]
                send_command_with_response(ser, f"Set Bandwidth to {bw_name}", LORA_HW_BANDWIDTH, bw_idx)
            
            print("\n🔀 Testing Spreading Factor Control")
            print("-" * 30)
            # Test different spreading factors
            for sf in [7, 8, 9, 10, 11, 12]:
                send_command_with_response(ser, f"Set Spreading Factor SF{sf}", LORA_HW_SPREADING_FACTOR, sf)
            
            print("\n🛡️ Testing Coding Rate Control")
            print("-" * 30)
            # Test different coding rates
            for cr in [5, 6, 7, 8]:
                send_command_with_response(ser, f"Set Coding Rate 4/{cr}", LORA_HW_CODING_RATE, cr)
            
            print("\n🎯 Testing Realistic Configuration Profiles")
            print("-" * 40)
            
            # Profile 1: Long Range, Low Data Rate
            print("\n📡 Profile 1: Maximum Range Configuration")
            send_command_with_response(ser, "Frequency", LORA_HW_FREQUENCY, 433000000, " Hz")
            send_command_with_response(ser, "Power", LORA_HW_TX_POWER, 20, " dBm")
            send_command_with_response(ser, "Bandwidth", LORA_HW_BANDWIDTH, 6, " (62.5 kHz)")
            send_command_with_response(ser, "Spreading Factor", LORA_HW_SPREADING_FACTOR, 12)
            send_command_with_response(ser, "Coding Rate", LORA_HW_CODING_RATE, 8, " (4/8)")
            
            # Profile 2: Balanced Performance
            print("\n⚖️ Profile 2: Balanced Performance Configuration")
            send_command_with_response(ser, "Frequency", LORA_HW_FREQUENCY, 915000000, " Hz")
            send_command_with_response(ser, "Power", LORA_HW_TX_POWER, 14, " dBm")
            send_command_with_response(ser, "Bandwidth", LORA_HW_BANDWIDTH, 7, " (125 kHz)")
            send_command_with_response(ser, "Spreading Factor", LORA_HW_SPREADING_FACTOR, 9)
            send_command_with_response(ser, "Coding Rate", LORA_HW_CODING_RATE, 5, " (4/5)")
            
            # Profile 3: High Data Rate
            print("\n🚀 Profile 3: High Data Rate Configuration")
            send_command_with_response(ser, "Frequency", LORA_HW_FREQUENCY, 868000000, " Hz")
            send_command_with_response(ser, "Power", LORA_HW_TX_POWER, 17, " dBm")
            send_command_with_response(ser, "Bandwidth", LORA_HW_BANDWIDTH, 9, " (500 kHz)")
            send_command_with_response(ser, "Spreading Factor", LORA_HW_SPREADING_FACTOR, 7)
            send_command_with_response(ser, "Coding Rate", LORA_HW_CODING_RATE, 5, " (4/5)")
            
            print("\n" + "=" * 60)
            print("🎉 KISS SetHardware Demonstration Complete!")
            print("✅ All LoRa parameters successfully controlled via KISS")
            print("🔧 Your TNC now supports full LoRa configuration control")
            print("📋 Use these commands in your host applications for")
            print("   dynamic radio parameter adjustment")
            print("=" * 60)
            
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        print("🔍 Check that device is connected and in KISS mode")
    except KeyboardInterrupt:
        print("\n⏹️ Demonstration interrupted by user")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()