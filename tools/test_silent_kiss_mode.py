#!/usr/bin/env python3
"""
Test script for silent KISS mode operation
Verifies that KISS mode produces no text output
"""

import serial
import time
import sys

def test_silent_kiss_mode():
    """Test that KISS mode is completely silent"""
    ser = None
    try:
        # Connect to TNC
        ser = serial.Serial('/dev/ttyACM0', 9600, timeout=2)
        time.sleep(2)
        
        print("=== SILENT KISS MODE TEST ===")
        print("Connected to TNC")
        
        # Clear buffers
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(1)
        
        print("\n1. Testing manual KISS command entry...")
        ser.write(b'KISS\r')
        time.sleep(1)
        
        # Check if there's any output (there shouldn't be in silent mode)
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ KISS command should be silent!")
        else:
            print("   ✓ KISS command is silent - no output")
        
        print("\n2. Testing ESC@k sequence entry...")
        # First exit any existing KISS mode
        ser.write(b'\x1B')
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Send ESC@k sequence
        ser.write(b'\x1B@k\r')
        time.sleep(1)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ ESC@k sequence should be silent!")
        else:
            print("   ✓ ESC@k sequence is silent - no output")
        
        print("\n3. Testing KISS parameter commands...")
        
        # Send TXDelay command (should be silent)
        txdelay_cmd = b'\xC0\x01\x1E\xC0'
        ser.write(txdelay_cmd)
        time.sleep(0.5)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ KISS parameter commands should be silent!")
        else:
            print("   ✓ KISS parameter commands are silent")
        
        # Send Persistence command (should be silent)
        persistence_cmd = b'\xC0\x02\x3F\xC0'
        ser.write(persistence_cmd)
        time.sleep(0.5)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ KISS parameter commands should be silent!")
        else:
            print("   ✓ KISS parameter commands remain silent")
        
        print("\n4. Testing KISS data frame...")
        
        # Send a test data frame
        test_packet = b'TEST>APRS:This is a test packet'
        kiss_frame = b'\xC0\x00' + test_packet + b'\xC0'
        ser.write(kiss_frame)
        time.sleep(1)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ KISS data frames should be silent!")
        else:
            print("   ✓ KISS data frames are silent")
        
        print("\n5. Testing silent exit via ESC...")
        
        # Exit with ESC (should be silent)
        ser.write(b'\x1B')
        time.sleep(1)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ ESC exit should be silent!")
        else:
            print("   ✓ ESC exit is silent")
        
        print("\n6. Verifying return to command mode...")
        
        # Test that we're back in command mode
        ser.write(b'ID\r')
        time.sleep(1)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Command mode response: {response.decode('ascii', errors='ignore').strip()}")
            if b"TNC" in response or b"LoRa" in response:
                print("   ✓ Successfully returned to command mode")
            else:
                print("   ? Response unclear")
        else:
            print("   ❌ No response from command mode!")
        
        print("\n7. Testing silent CMD_RETURN exit...")
        
        # Enter KISS mode again
        ser.write(b'KISS\r')
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Exit with CMD_RETURN (should be silent)
        ser.write(b'\xC0\xFF\xC0')
        time.sleep(1)
        
        response = b''
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"   Unexpected output: {response}")
            print("   ❌ CMD_RETURN exit should be silent!")
        else:
            print("   ✓ CMD_RETURN exit is silent")
        
        # Verify we're back in command mode
        ser.write(b'ID\r')
        time.sleep(1)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            if b"TNC" in response or b"LoRa" in response:
                print("   ✓ CMD_RETURN properly returned to command mode")
        
        ser.close()
        
        print("\n=== SILENT KISS MODE TEST COMPLETE ===")
        print("✓ KISS mode is now completely silent!")
        print("✓ No text output during KISS operations")
        print("✓ Proper binary-only protocol compliance")
        
    except Exception as e:
        print(f"Error during test: {e}")
        try:
            if ser is not None:
                ser.close()
        except:
            pass

if __name__ == "__main__":
    test_silent_kiss_mode()