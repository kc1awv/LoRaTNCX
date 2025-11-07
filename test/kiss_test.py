#!/usr/bin/env python3
"""
KISS TNC Test Program for LoRaTNCX
Tests bidirectional communication between two LoRa devices
"""

import serial
import time
import sys
import argparse
from datetime import datetime

# KISS Protocol Constants
FEND = 0xC0  # Frame End
FESC = 0xDB  # Frame Escape
TFEND = 0xDC  # Transposed Frame End
TFESC = 0xDD  # Transposed Frame Escape

# KISS Commands
CMD_DATA = 0x00
CMD_TXDELAY = 0x01
CMD_PERSISTENCE = 0x02
CMD_SLOTTIME = 0x03
CMD_TXTAIL = 0x04
CMD_FULLDUPLEX = 0x05
CMD_SETHARDWARE = 0x06
CMD_RETURN = 0xFF

# SETHARDWARE Subcommands
HW_SET_FREQUENCY = 0x01
HW_SET_BANDWIDTH = 0x02
HW_SET_SPREADING = 0x03
HW_SET_CODINGRATE = 0x04
HW_SET_POWER = 0x05
HW_GET_CONFIG = 0x06
HW_SAVE_CONFIG = 0x07
HW_RESET_CONFIG = 0x08
HW_SET_SYNCWORD = 0x09


class KISSFrame:
    """KISS frame encoder/decoder"""
    
    @staticmethod
    def escape(data):
        """Apply KISS escaping to data"""
        result = bytearray()
        for byte in data:
            if byte == FEND:
                result.extend([FESC, TFEND])
            elif byte == FESC:
                result.extend([FESC, TFESC])
            else:
                result.append(byte)
        return bytes(result)
    
    @staticmethod
    def unescape(data):
        """Remove KISS escaping from data"""
        result = bytearray()
        i = 0
        while i < len(data):
            if data[i] == FESC:
                if i + 1 < len(data):
                    if data[i + 1] == TFEND:
                        result.append(FEND)
                        i += 2
                    elif data[i + 1] == TFESC:
                        result.append(FESC)
                        i += 2
                    else:
                        i += 1
                else:
                    i += 1
            else:
                result.append(data[i])
                i += 1
        return bytes(result)
    
    @staticmethod
    def encode_data_frame(data):
        """Encode data as KISS frame"""
        frame = bytes([CMD_DATA]) + data
        escaped = KISSFrame.escape(frame)
        return bytes([FEND]) + escaped + bytes([FEND])
    
    @staticmethod
    def encode_command(cmd, subcmd=None, data=None):
        """Encode KISS command frame"""
        if subcmd is not None:
            frame = bytes([cmd, subcmd])
            if data:
                frame += data
        else:
            frame = bytes([cmd])
            if data:
                frame += data
        
        escaped = KISSFrame.escape(frame)
        return bytes([FEND]) + escaped + bytes([FEND])
    
    @staticmethod
    def decode(raw_data):
        """Decode received KISS frames from raw data"""
        frames = []
        current = bytearray()
        in_frame = False
        
        for byte in raw_data:
            if byte == FEND:
                if in_frame and len(current) > 0:
                    # End of frame
                    unescaped = KISSFrame.unescape(bytes(current))
                    if len(unescaped) > 0:
                        frames.append(unescaped)
                    current = bytearray()
                in_frame = True
            elif in_frame:
                current.append(byte)
        
        return frames


class LoRaTNC:
    """LoRa TNC Interface"""
    
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
    def open(self):
        """Open serial connection"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=0.1)
            time.sleep(2)  # Wait for device to be ready
            # Flush any startup data
            self.ser.reset_input_buffer()
            return True
        except Exception as e:
            print(f"Error opening {self.port}: {e}")
            return False
    
    def close(self):
        """Close serial connection"""
        if self.ser:
            self.ser.close()
    
    def send_data(self, data):
        """Send data frame"""
        if isinstance(data, str):
            data = data.encode('utf-8')
        
        frame = KISSFrame.encode_data_frame(data)
        self.ser.write(frame)
        self.ser.flush()
    
    def receive(self, timeout=1.0):
        """Receive frames with timeout"""
        start_time = time.time()
        buffer = bytearray()
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                buffer.extend(self.ser.read(self.ser.in_waiting))
                time.sleep(0.05)  # Small delay to accumulate data
            else:
                time.sleep(0.01)
        
        if len(buffer) > 0:
            frames = KISSFrame.decode(bytes(buffer))
            return frames
        return []
    
    def get_config(self):
        """Request current configuration"""
        frame = KISSFrame.encode_command(CMD_SETHARDWARE, HW_GET_CONFIG)
        self.ser.write(frame)
        self.ser.flush()
        time.sleep(0.5)
        
        frames = self.receive(timeout=2.0)
        if frames:
            for frame in frames:
                if len(frame) >= 11 and frame[0] == CMD_DATA and frame[1] == HW_GET_CONFIG:
                    # Parse config response
                    import struct
                    freq = struct.unpack('<f', frame[2:6])[0]
                    bw_idx = frame[6]
                    sf = frame[7]
                    cr = frame[8]
                    power = struct.unpack('b', bytes([frame[9]]))[0]
                    syncword = struct.unpack('<H', frame[10:12])[0]
                    
                    bw_map = {0: 125.0, 1: 250.0, 2: 500.0}
                    bw = bw_map.get(bw_idx, 125.0)
                    
                    return {
                        'frequency': freq,
                        'bandwidth': bw,
                        'spreading_factor': sf,
                        'coding_rate': cr,
                        'power': power,
                        'syncword': syncword
                    }
        return None


def test_bidirectional(port1, port2):
    """Test bidirectional communication between two TNCs"""
    
    print("=" * 70)
    print("LoRaTNCX Bidirectional Communication Test")
    print("=" * 70)
    print()
    
    # Open both TNCs
    print(f"Opening TNC 1: {port1}")
    tnc1 = LoRaTNC(port1)
    if not tnc1.open():
        print("Failed to open TNC 1")
        return False
    
    print(f"Opening TNC 2: {port2}")
    tnc2 = LoRaTNC(port2)
    if not tnc2.open():
        print("Failed to open TNC 2")
        tnc1.close()
        return False
    
    print("✓ Both TNCs opened successfully")
    print()
    
    # Get configurations
    print("Getting TNC 1 configuration...")
    config1 = tnc1.get_config()
    if config1:
        print(f"  Frequency: {config1['frequency']:.3f} MHz")
        print(f"  Bandwidth: {config1['bandwidth']:.1f} kHz")
        print(f"  SF: {config1['spreading_factor']}")
        print(f"  CR: 4/{config1['coding_rate']}")
        print(f"  Power: {config1['power']} dBm")
        print(f"  Sync Word: 0x{config1['syncword']:04X}")
    else:
        print("  Warning: Could not get config")
    print()
    
    print("Getting TNC 2 configuration...")
    config2 = tnc2.get_config()
    if config2:
        print(f"  Frequency: {config2['frequency']:.3f} MHz")
        print(f"  Bandwidth: {config2['bandwidth']:.1f} kHz")
        print(f"  SF: {config2['spreading_factor']}")
        print(f"  CR: 4/{config2['coding_rate']}")
        print(f"  Power: {config2['power']} dBm")
        print(f"  Sync Word: 0x{config2['syncword']:04X}")
    else:
        print("  Warning: Could not get config")
    print()
    
    # Test 1: TNC1 -> TNC2
    print("-" * 70)
    print("Test 1: TNC1 → TNC2")
    print("-" * 70)
    
    test_message1 = f"Hello from TNC1 at {datetime.now().strftime('%H:%M:%S')}"
    print(f"Sending: '{test_message1}'")
    
    tnc1.send_data(test_message1)
    time.sleep(1.0)  # Give time for transmission
    
    print("Listening on TNC2...")
    frames = tnc2.receive(timeout=3.0)
    
    if frames:
        print(f"✓ Received {len(frames)} frame(s)")
        for i, frame in enumerate(frames):
            if frame[0] == CMD_DATA:
                payload = frame[1:]
                try:
                    text = payload.decode('utf-8')
                    print(f"  Frame {i+1}: '{text}'")
                    if text == test_message1:
                        print("  ✓ Message matches!")
                    else:
                        print("  ✗ Message mismatch")
                except:
                    print(f"  Frame {i+1}: {payload.hex()}")
    else:
        print("✗ No frames received")
    print()
    
    # Test 2: TNC2 -> TNC1
    print("-" * 70)
    print("Test 2: TNC2 → TNC1")
    print("-" * 70)
    
    test_message2 = f"Hello from TNC2 at {datetime.now().strftime('%H:%M:%S')}"
    print(f"Sending: '{test_message2}'")
    
    tnc2.send_data(test_message2)
    time.sleep(1.0)  # Give time for transmission
    
    print("Listening on TNC1...")
    frames = tnc1.receive(timeout=3.0)
    
    if frames:
        print(f"✓ Received {len(frames)} frame(s)")
        for i, frame in enumerate(frames):
            if frame[0] == CMD_DATA:
                payload = frame[1:]
                try:
                    text = payload.decode('utf-8')
                    print(f"  Frame {i+1}: '{text}'")
                    if text == test_message2:
                        print("  ✓ Message matches!")
                    else:
                        print("  ✗ Message mismatch")
                except:
                    print(f"  Frame {i+1}: {payload.hex()}")
    else:
        print("✗ No frames received")
    print()
    
    # Test 3: Multiple rapid messages
    print("-" * 70)
    print("Test 3: Multiple Messages (TNC1 → TNC2)")
    print("-" * 70)
    
    messages = [
        "Message 1: Short test",
        "Message 2: Testing multiple frames in sequence",
        "Message 3: Can you receive all of these?",
        "Message 4: Final test message"
    ]
    
    for msg in messages:
        print(f"Sending: '{msg}'")
        tnc1.send_data(msg)
        time.sleep(0.5)  # Brief delay between messages
    
    print("Listening on TNC2 for all messages...")
    time.sleep(2.0)
    frames = tnc2.receive(timeout=5.0)
    
    if frames:
        print(f"✓ Received {len(frames)} frame(s)")
        for i, frame in enumerate(frames):
            if frame[0] == CMD_DATA:
                payload = frame[1:]
                try:
                    text = payload.decode('utf-8')
                    print(f"  Frame {i+1}: '{text}'")
                except:
                    print(f"  Frame {i+1}: {payload.hex()}")
    else:
        print("✗ No frames received")
    print()
    
    # Cleanup
    print("-" * 70)
    print("Test Complete")
    print("-" * 70)
    tnc1.close()
    tnc2.close()
    
    return True


def interactive_mode(port1, port2):
    """Interactive chat mode between two TNCs"""
    
    print("=" * 70)
    print("LoRaTNCX Interactive Chat Mode")
    print("=" * 70)
    print()
    print("Commands:")
    print("  1:<message>  - Send from TNC1")
    print("  2:<message>  - Send from TNC2")
    print("  r            - Check for received messages")
    print("  q            - Quit")
    print()
    
    # Open both TNCs
    tnc1 = LoRaTNC(port1)
    tnc2 = LoRaTNC(port2)
    
    if not tnc1.open() or not tnc2.open():
        print("Failed to open TNCs")
        return
    
    print(f"TNC1: {port1}")
    print(f"TNC2: {port2}")
    print("Ready!")
    print()
    
    try:
        while True:
            cmd = input("> ").strip()
            
            if cmd == 'q':
                break
            elif cmd == 'r':
                # Check both TNCs for received frames
                print("Checking TNC1...")
                frames1 = tnc1.receive(timeout=0.5)
                if frames1:
                    for frame in frames1:
                        if frame[0] == CMD_DATA:
                            print(f"  TNC1 << {frame[1:].decode('utf-8', errors='ignore')}")
                
                print("Checking TNC2...")
                frames2 = tnc2.receive(timeout=0.5)
                if frames2:
                    for frame in frames2:
                        if frame[0] == CMD_DATA:
                            print(f"  TNC2 << {frame[1:].decode('utf-8', errors='ignore')}")
                
                if not frames1 and not frames2:
                    print("  No messages")
            
            elif cmd.startswith('1:'):
                msg = cmd[2:]
                print(f"TNC1 >> {msg}")
                tnc1.send_data(msg)
                time.sleep(0.5)
                
                # Auto-check TNC2 for response
                frames = tnc2.receive(timeout=2.0)
                if frames:
                    for frame in frames:
                        if frame[0] == CMD_DATA:
                            print(f"  TNC2 << {frame[1:].decode('utf-8', errors='ignore')}")
            
            elif cmd.startswith('2:'):
                msg = cmd[2:]
                print(f"TNC2 >> {msg}")
                tnc2.send_data(msg)
                time.sleep(0.5)
                
                # Auto-check TNC1 for response
                frames = tnc1.receive(timeout=2.0)
                if frames:
                    for frame in frames:
                        if frame[0] == CMD_DATA:
                            print(f"  TNC1 << {frame[1:].decode('utf-8', errors='ignore')}")
            
            else:
                print("Unknown command")
    
    except KeyboardInterrupt:
        print("\nInterrupted")
    
    finally:
        tnc1.close()
        tnc2.close()
        print("Closed")


def main():
    parser = argparse.ArgumentParser(
        description='Test LoRaTNCX KISS TNC communication'
    )
    parser.add_argument('port1', help='First TNC serial port (e.g., /dev/ttyUSB0)')
    parser.add_argument('port2', help='Second TNC serial port (e.g., /dev/ttyACM0)')
    parser.add_argument('-i', '--interactive', action='store_true',
                       help='Interactive chat mode')
    
    args = parser.parse_args()
    
    if args.interactive:
        interactive_mode(args.port1, args.port2)
    else:
        test_bidirectional(args.port1, args.port2)


if __name__ == '__main__':
    main()
