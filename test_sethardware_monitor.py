#!/usr/bin/env python3
"""
Monitor serial output and send KISS SetHardware commands simultaneously
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

def monitor_serial(ser, stop_event):
    """Monitor serial output in a separate thread"""
    print("=== Starting Serial Monitor ===")
    while not stop_event.is_set():
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    if text.strip():
                        # Print with timestamp
                        timestamp = time.strftime("%H:%M:%S")
                        for line in text.splitlines():
                            if line.strip():
                                print(f"[{timestamp}] {line}")
                except:
                    print(f"Raw data: {data.hex()}")
            time.sleep(0.05)
        except Exception as e:
            if not stop_event.is_set():
                print(f"Monitor error: {e}")
            break

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM1'
    
    print(f"LoRa SetHardware Test with Serial Monitoring")
    print(f"Connecting to {port}")
    print("=" * 60)
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(2)
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Start monitoring thread
            stop_event = threading.Event()
            monitor_thread = threading.Thread(target=monitor_serial, args=(ser, stop_event))
            monitor_thread.daemon = True
            monitor_thread.start()
            
            time.sleep(1)  # Let monitor start
            
            print("Sending KISS SetHardware commands...")
            print("=" * 60)
            
            # Test 1: Set frequency to 433.000 MHz
            print("1. Setting frequency to 433.000 MHz")
            cmd = create_sethardware_command(LORA_HW_FREQUENCY, 433000000)
            ser.write(cmd)
            time.sleep(1)
            
            # Test 2: Set TX power to 17 dBm
            print("2. Setting TX power to 17 dBm")
            cmd = create_sethardware_command(LORA_HW_TX_POWER, 17)
            ser.write(cmd)
            time.sleep(1)
            
            # Test 3: Set bandwidth to 125 kHz (index 7)
            print("3. Setting bandwidth to 125 kHz")
            cmd = create_sethardware_command(LORA_HW_BANDWIDTH, 7)
            ser.write(cmd)
            time.sleep(1)
            
            # Test 4: Set spreading factor to SF9
            print("4. Setting spreading factor to SF9")
            cmd = create_sethardware_command(LORA_HW_SPREADING_FACTOR, 9)
            ser.write(cmd)
            time.sleep(1)
            
            # Test 5: Set coding rate to 4/7
            print("5. Setting coding rate to 4/7")
            cmd = create_sethardware_command(LORA_HW_CODING_RATE, 7)
            ser.write(cmd)
            time.sleep(2)
            
            print("\nAll commands sent. Monitoring for 5 more seconds...")
            time.sleep(5)
            
            stop_event.set()
            monitor_thread.join(timeout=1)
            
            print("\nTest completed!")
            
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()