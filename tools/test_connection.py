#!/usr/bin/env python3
"""
LoRaTNCX Connection Test

Simple script to test connectivity and basic functionality with a LoRaTNCX device.
This script performs basic checks and shows device information.

Usage:
    python3 test_connection.py <serial_port>

Example:
    python3 test_connection.py /dev/ttyACM0
    python3 test_connection.py COM3

Author: LoRaTNCX Project
"""

import sys
import time
from loratncx_config import LoRaTNCXConfig


def test_connection(port: str):
    """Test connection to LoRaTNCX device."""
    print(f"LoRaTNCX Connection Test - Port: {port}")
    print("=" * 50)
    
    # Create config tool
    config = LoRaTNCXConfig(port, debug=True)
    
    # Test 1: Connection
    print("\n1. Testing serial connection...")
    if not config.connect():
        print("❌ Connection failed!")
        return False
    print("✅ Connection successful!")
    
    # Test 2: Monitor debug output
    print("\n2. Monitoring device output (5 seconds)...")
    config.monitor_debug_output(5)
    
    # Test 3: Send get config command
    print("\n3. Testing KISS configuration query...")
    config.get_configuration()
    
    # Test 4: Send a simple parameter change (and revert it)
    print("\n4. Testing parameter setting (TX delay)...")
    print("   Saving current TX delay, setting to 35, then restoring...")
    
    # Set TX delay to 35 (350ms)
    if config.set_tx_delay(35):
        print("   ✅ TX delay set to 35")
        time.sleep(1)
        
        # Set it back to default (30)
        if config.set_tx_delay(30):
            print("   ✅ TX delay restored to 30")
        else:
            print("   ⚠️  Could not restore TX delay")
    else:
        print("   ❌ Failed to set TX delay")
    
    # Test 5: Final config query
    print("\n5. Final configuration check...")
    config.get_configuration()
    
    config.disconnect()
    
    print("\n" + "=" * 50)
    print("Connection test completed!")
    print("\nIf you saw debug output and configuration responses,")
    print("your LoRaTNCX is working correctly with this tool.")
    
    return True


def main():
    """Main entry point."""
    if len(sys.argv) != 2:
        print("Usage: python3 test_connection.py <serial_port>")
        print("Example: python3 test_connection.py /dev/ttyACM0")
        print("Example: python3 test_connection.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        test_connection(port)
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
    except Exception as e:
        print(f"\nTest failed with error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()