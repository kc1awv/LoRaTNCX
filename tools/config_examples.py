#!/usr/bin/env python3
"""
LoRaTNCX Configuration Examples

This script demonstrates common configuration scenarios for the LoRaTNCX
using the loratncx_config.py tool. Each example shows different use cases
and parameter combinations.

Author: LoRaTNCX Project
"""

import subprocess
import sys
import time
from typing import List


def run_config_command(args: List[str], description: str) -> bool:
    """Run a configuration command and display results.
    
    Args:
        args: Command line arguments for loratncx_config.py
        description: Description of what the command does
        
    Returns:
        True if command succeeded, False otherwise
    """
    print(f"\n=== {description} ===")
    print(f"Command: python3 loratncx_config.py {' '.join(args)}")
    
    try:
        result = subprocess.run(
            ["python3", "loratncx_config.py"] + args,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        if result.stdout:
            print("Output:", result.stdout.strip())
        if result.stderr:
            print("Errors:", result.stderr.strip())
            
        return result.returncode == 0
        
    except subprocess.TimeoutExpired:
        print("Error: Command timed out")
        return False
    except FileNotFoundError:
        print("Error: loratncx_config.py not found. Make sure you're in the tools directory.")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False


def main():
    """Run configuration examples."""
    print("LoRaTNCX Configuration Examples")
    print("=" * 40)
    
    # Check if port is provided
    if len(sys.argv) < 2:
        print("Usage: python3 config_examples.py <serial_port>")
        print("Example: python3 config_examples.py /dev/ttyACM0")
        print("Example: python3 config_examples.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    print(f"Using serial port: {port}")
    print("These examples will configure your LoRaTNCX device.")
    print("Press Ctrl+C at any time to stop.")
    
    try:
        # Example configurations
        examples = [
            {
                "args": ["--port", port, "--get-config"],
                "description": "Get Current Configuration"
            },
            {
                "args": ["--port", port, "--frequency", "433.175", "--power", "14"],
                "description": "Basic 433 MHz APRS Setup"
            },
            {
                "args": ["--port", port, "--sf", "9", "--bw", "125.0", "--cr", "7"],
                "description": "Standard LoRa Parameters for APRS"
            },
            {
                "args": ["--port", port, "--txdelay", "30", "--persist", "63", "--slottime", "10"],
                "description": "CSMA Parameters for Medium Traffic"
            },
            {
                "args": ["--port", port, "--frequency", "915.0", "--sf", "7", "--bw", "250.0", "--power", "8"],
                "description": "High Speed Short Range (915 MHz ISM)"
            },
            {
                "args": ["--port", port, "--frequency", "433.175", "--sf", "12", "--bw", "62.5", "--cr", "8", "--power", "20"],
                "description": "Long Range Low Speed Configuration"
            },
            {
                "args": ["--port", port, "--frequency", "868.1", "--sf", "10", "--bw", "125.0", "--cr", "6", "--power", "14"],
                "description": "European 868 MHz Band Configuration"
            },
            {
                "args": ["--port", port, "--txdelay", "50", "--persist", "31", "--slottime", "20"],
                "description": "Conservative CSMA for Busy Networks"
            },
            {
                "args": ["--port", port, "--get-config"],
                "description": "Final Configuration Check"
            }
        ]
        
        # Run each example
        for i, example in enumerate(examples, 1):
            print(f"\n[{i}/{len(examples)}]", end=" ")
            success = run_config_command(example["args"], example["description"])
            
            if not success:
                print("Command failed. Continuing with next example...")
            
            # Pause between commands
            if i < len(examples):
                print("\nWaiting 2 seconds before next command...")
                time.sleep(2)
        
        print("\n" + "=" * 40)
        print("All examples completed!")
        print("\nYour LoRaTNCX is now configured with the final settings.")
        print("Use 'python3 loratncx_config.py --port {} --get-config' to verify.".format(port))
        
    except KeyboardInterrupt:
        print("\n\nExamples interrupted by user.")
        print("Your device may be partially configured.")


if __name__ == '__main__':
    main()