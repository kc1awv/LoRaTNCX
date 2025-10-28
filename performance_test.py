#!/usr/bin/env python3
"""
Fast timing test for optimized KISS SetHardware commands
"""

import serial
import struct
import time

# KISS constants
FEND = 0xC0
KISS_CMD_SETHARDWARE = 0x06
LORA_HW_FREQUENCY = 0x00
LORA_HW_BANDWIDTH = 0x02

def create_kiss_frame(command, data=b''):
    frame = bytes([command]) + data
    return bytes([FEND]) + frame + bytes([FEND])

def create_frequency_command(freq_hz):
    data = struct.pack('<BI', LORA_HW_FREQUENCY, freq_hz)
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def create_bandwidth_command(bw_idx):
    data = struct.pack('<BB', LORA_HW_BANDWIDTH, bw_idx)
    return create_kiss_frame(KISS_CMD_SETHARDWARE, data)

def main():
    port = '/dev/ttyACM0'
    
    print("⚡ Optimized KISS SetHardware Performance Test")
    print("=" * 50)
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(0.5)
            ser.reset_input_buffer()
            
            # Test rapid-fire commands
            commands = [
                ("433 MHz", create_frequency_command(433000000)),
                ("125 kHz BW", create_bandwidth_command(7)),
                ("915 MHz", create_frequency_command(915000000)),
                ("250 kHz BW", create_bandwidth_command(8)),
                ("868 MHz", create_frequency_command(868000000)),
            ]
            
            print("🚀 Sending rapid-fire commands...")
            
            total_start = time.time()
            
            for desc, cmd in commands:
                cmd_start = time.time()
                
                # Send command
                ser.write(cmd)
                
                # Quick check for immediate response
                time.sleep(0.01)  # 10ms wait
                if ser.in_waiting > 0:
                    response = ser.read(ser.in_waiting)
                    # We don't print the response since debug is disabled
                
                cmd_time = (time.time() - cmd_start) * 1000
                print(f"  {desc}: {cmd_time:.1f} ms")
            
            total_time = (time.time() - total_start) * 1000
            print(f"\n⏱️ Total time for 5 commands: {total_time:.1f} ms")
            print(f"📊 Average per command: {total_time/5:.1f} ms")
            
            # Test burst mode (no delays)
            print(f"\n🔥 Burst mode test (no delays)...")
            burst_start = time.time()
            
            for desc, cmd in commands:
                ser.write(cmd)
            
            burst_time = (time.time() - burst_start) * 1000
            print(f"   5 commands sent in: {burst_time:.1f} ms")
            print(f"   Commands per second: {5000/burst_time:.0f}")
            
            # Brief monitoring
            time.sleep(0.5)
            if ser.in_waiting > 0:
                final_response = ser.read(ser.in_waiting)
                print(f"   Final response length: {len(final_response)} bytes")
            
            print(f"\n✅ Performance test complete!")
            
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()