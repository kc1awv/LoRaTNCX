#!/usr/bin/env python3
"""
Quick demonstration of LoRa Chat functionality
"""

import subprocess
import time
import sys

def demo_chat():
    """Demonstrate chat functionality with automated commands"""
    
    print("🎬 LoRa Chat Demonstration")
    print("=" * 40)
    print("This demo will:")
    print("  1. Start the chat application")
    print("  2. Show current configuration")
    print("  3. Send a hello message")
    print("  4. Change radio parameters")
    print("  5. Send test messages")
    print("  6. Exit")
    print()
    
    # Commands to demonstrate
    demo_commands = [
        "/config",           # Show current config
        "/hello",            # Send hello packet
        "Hello World! This is a test message from the LoRa chat app.",
        "/set freq 915000000",  # Change to 915 MHz
        "/set power 20",        # Increase power
        "/set sf 7",            # Change to SF7 for faster data
        "Testing parameter changes - now on 915MHz, 20dBm, SF7",
        "/nodes",               # Check for discovered nodes
        "If you have another device running, it should appear in /nodes",
        "/set freq 433000000",  # Back to 433 MHz
        "Final test message on 433 MHz",
        "/quit"
    ]
    
    print("🚀 Starting demonstration...")
    print("⏱️  Each command will be sent every 3 seconds")
    print("🛑 Press Ctrl+C to stop early")
    print()
    
    try:
        # Start the chat application
        process = subprocess.Popen(
            ["python3", "lora_chat.py", "/dev/ttyACM0", "config_node1.json"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        
        # Give it time to start
        time.sleep(4)
        
        # Send demo commands
        for i, cmd in enumerate(demo_commands, 1):
            print(f"📤 Step {i}/{len(demo_commands)}: {cmd}")
            
            try:
                process.stdin.write(cmd + "\n")
                process.stdin.flush()
                time.sleep(3)  # Wait between commands
            except Exception as e:
                print(f"❌ Error sending command: {e}")
                break
        
        # Give final command time to process
        time.sleep(2)
        
        # Try to get output
        try:
            stdout, stderr = process.communicate(timeout=5)
            if stdout:
                print("\n📋 Application Output:")
                print("-" * 40)
                print(stdout[-1000:])  # Show last 1000 characters
        except subprocess.TimeoutExpired:
            process.kill()
            print("⚠️  Application timed out (normal for demo)")
        
    except KeyboardInterrupt:
        print("\n🛑 Demo interrupted by user")
        try:
            process.terminate()
        except:
            pass
    except Exception as e:
        print(f"❌ Demo error: {e}")
    
    print("\n✅ Demonstration complete!")
    print("\n🎯 To run the chat application manually:")
    print("   python3 lora_chat.py")
    print("\n🚀 To launch with specific config:")
    print("   python3 lora_chat_launcher.py")

if __name__ == "__main__":
    demo_chat()