#!/usr/bin/env python3
"""
Test script for ESC@k KISS mode entry command
This tests the complete Xastir initialization sequence and KISS mode entry
"""

import serial
import time
import sys

def test_esc_at_k_entry():
    """Test the ESC@k sequence for KISS mode entry"""
    ser = None
    try:
        # Connect to TNC
        ser = serial.Serial('/dev/ttyACM0', 9600, timeout=2)
        time.sleep(2)  # Allow connection to stabilize
        
        print("Connected to TNC")
        
        # Clear any existing data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Wait a moment for TNC to be ready
        time.sleep(1)
        
        print("Sending ESC@k sequence for KISS mode entry...")
        
        # Send the ESC@k sequence (0x1B 0x40 0x6B 0x0D)
        esc_at_k_sequence = b'\x1B@k\r'
        ser.write(esc_at_k_sequence)
        
        print("Sent ESC@k sequence, waiting for response...")
        
        # Read response for up to 5 seconds
        response = b''
        start_time = time.time()
        while time.time() - start_time < 5:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                response += data
                print(f"Received: {data}")
                
                # Check if we got the KISS mode entry message
                if b"Entering KISS mode" in response or b"KISS mode" in response:
                    print("SUCCESS: TNC entered KISS mode via ESC@k command!")
                    break
            time.sleep(0.1)
        
        print(f"Total response: {response}")
        
        # Test if we're in KISS mode by sending a KISS command
        print("\nTesting KISS mode with command 0xFF (exit KISS)...")
        kiss_exit = b'\xC0\xFF\xC0'
        ser.write(kiss_exit)
        
        # Read response
        time.sleep(1)
        if ser.in_waiting > 0:
            exit_response = ser.read(ser.in_waiting)
            print(f"KISS exit response: {exit_response}")
            
            if b"Exiting KISS mode" in exit_response:
                print("SUCCESS: KISS exit command worked!")
            else:
                print("Note: No explicit exit message, but command may have worked")
        
        # Test normal command mode
        print("\nTesting normal command mode with HELLO...")
        ser.write(b'HELLO\r')
        time.sleep(1)
        
        if ser.in_waiting > 0:
            hello_response = ser.read(ser.in_waiting)
            print(f"HELLO response: {hello_response}")
            
            if b"Hello" in hello_response or b"TNC" in hello_response:
                print("SUCCESS: Back in normal command mode!")
        
        ser.close()
        print("\nTest completed successfully!")
        
    except Exception as e:
        print(f"Error during test: {e}")
        try:
            if ser is not None:
                ser.close()
        except:
            pass

if __name__ == "__main__":
    print("Testing ESC@k KISS mode entry command...")
    test_esc_at_k_entry()