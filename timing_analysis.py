#!/usr/bin/env python3
"""
Timing analysis for KISS SetHardware commands
"""

import serial
import struct
import time

# KISS constants
FEND = 0xC0
KISS_CMD_SETHARDWARE = 0x06
LORA_HW_FREQUENCY = 0x00

def kiss_escape(data):
    result = []
    for byte in data:
        if byte == FEND:
            result.extend([0xDB, 0xDC])
        elif byte == 0xDB:
            result.extend([0xDB, 0xDD])
        else:
            result.append(byte)
    return bytes(result)

def create_kiss_frame(command, data=b''):
    frame = bytes([command])
    if data:
        frame += data
    frame = kiss_escape(frame)
    return bytes([FEND]) + frame + bytes([FEND])

def create_frequency_command(freq_hz):
    data = struct.pack('<BI', LORA_HW_FREQUENCY, freq_hz)
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def main():
    port = '/dev/ttyACM1'
    
    print("Timing Analysis for KISS SetHardware Commands")
    print("=" * 50)
    
    try:
        with serial.Serial(port, 115200, timeout=3) as ser:
            time.sleep(1)
            ser.reset_input_buffer()
            
            # Test command timing
            frequencies = [433000000, 868000000, 915000000]
            
            for i, freq in enumerate(frequencies, 1):
                print(f"\nTest {i}: Setting frequency to {freq/1e6:.1f} MHz")
                
                # Measure command send time
                start_time = time.time()
                
                cmd = create_frequency_command(freq)
                ser.write(cmd)
                
                send_time = time.time() - start_time
                print(f"  Command send time: {send_time*1000:.2f} ms")
                
                # Measure response time
                response_start = time.time()
                response = b''
                response_complete = False
                
                for _ in range(100):  # Wait up to 10 seconds
                    if ser.in_waiting > 0:
                        data = ser.read(ser.in_waiting)
                        response += data
                        try:
                            text = data.decode('utf-8', errors='ignore')
                            if 'successful' in text or 'failed' in text:
                                response_complete = True
                                break
                        except:
                            pass
                    time.sleep(0.1)
                
                response_time = time.time() - response_start
                print(f"  Response time: {response_time*1000:.2f} ms")
                
                if response_complete:
                    print(f"  Status: Command completed")
                else:
                    print(f"  Status: No completion confirmation received")
                
                total_time = time.time() - start_time
                print(f"  Total time: {total_time*1000:.2f} ms")
                
                time.sleep(0.5)  # Brief pause between tests
            
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()