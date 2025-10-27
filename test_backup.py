#!/usr/bin/env python3
"""
Simple test script to check if the backup configuration button works
by monitoring WebSocket messages.
"""

import asyncio
import websockets
import json
import sys

async def test_backup():
    uri = "ws://192.168.4.1/ws"  # Default AP mode IP
    
    try:
        print("Connecting to WebSocket...")
        async with websockets.connect(uri) as websocket:
            print("Connected! Sending backup request...")
            
            # Send backup configuration request
            request = {
                "type": "request",
                "data": "backup_config"
            }
            
            await websocket.send(json.dumps(request))
            print("Request sent:", json.dumps(request, indent=2))
            
            # Wait for response
            print("Waiting for response...")
            response = await websocket.recv()
            data = json.loads(response)
            
            print("Response received:")
            print(json.dumps(data, indent=2))
            
            if data.get("type") == "backup_config":
                print("\n✅ SUCCESS: Backup configuration response received!")
                payload = data.get("payload", {})
                if "configuration" in payload:
                    print("✅ Configuration data is present in response")
                    print("✅ Backup functionality appears to be working correctly")
                else:
                    print("❌ WARNING: Configuration data missing from response")
            else:
                print(f"❌ ERROR: Expected 'backup_config' response, got '{data.get('type')}'")
                
    except ConnectionRefusedError:
        print("❌ ERROR: Could not connect to WebSocket. Is the device running and accessible?")
        print("   Make sure the device is in AP mode or connected to your network.")
    except Exception as e:
        print(f"❌ ERROR: {e}")

if __name__ == "__main__":
    print("LoRaTNCX Backup Configuration Test")
    print("==================================")
    asyncio.run(test_backup())