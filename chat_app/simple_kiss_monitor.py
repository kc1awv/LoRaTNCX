#!/usr/bin/env python3
"""
Quick KISS Reception Test

This test simply monitors the KISS TNC for received packets without requiring
user interaction. It will display any received data frames.
"""

import serial
import time
import sys
from threading import Thread, Event

class SimpleKISSMonitor:
    def __init__(self, port='/dev/ttyACM0', baudrate=115200):
        self.port_name = port
        self.baudrate = baudrate
        self.serial_port = None
        self.running = False
        self.rx_thread = None
        
        # KISS constants
        self.FEND = 0xC0
        self.FESC = 0xDB
        self.TFEND = 0xDC
        self.TFESC = 0xDD
        self.DATA_FRAME = 0x00
        
        # Statistics
        self.packets_received = 0
        self.start_time = time.time()
    
    def connect(self):
        """Connect to TNC"""
        try:
            self.serial_port = serial.Serial(
                self.port_name,
                self.baudrate,
                timeout=1
            )
            print(f"✓ Connected to {self.port_name}")
            return True
        except Exception as e:
            print(f"✗ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from TNC"""
        self.running = False
        if self.rx_thread:
            self.rx_thread.join(timeout=2)
        if self.serial_port:
            self.serial_port.close()
    
    def unescape_kiss_data(self, data):
        """Remove KISS byte stuffing"""
        result = bytearray()
        escaped = False
        for byte in data:
            if escaped:
                if byte == self.TFEND:
                    result.append(self.FEND)
                elif byte == self.TFESC:
                    result.append(self.FESC)
                else:
                    result.append(byte)
                escaped = False
            elif byte == self.FESC:
                escaped = True
            else:
                result.append(byte)
        return bytes(result)
    
    def process_data_frame(self, frame_data):
        """Process a received data frame"""
        self.packets_received += 1
        elapsed = time.time() - self.start_time
        
        print(f"\n🎉 PACKET #{self.packets_received} RECEIVED! (after {elapsed:.1f}s)")
        print(f"📦 Length: {len(frame_data)} bytes")
        
        # Try to decode as text
        try:
            text = frame_data.decode('utf-8', errors='ignore').strip()
            if text:
                print(f"📄 Content: '{text}'")
        except:
            pass
        
        # Show hex dump
        hex_dump = ' '.join(f'{b:02X}' for b in frame_data[:64])
        if len(frame_data) > 64:
            hex_dump += '...'
        print(f"🔍 Hex: {hex_dump}")
        print("-" * 50)
    
    def rx_thread_func(self):
        """Background thread for receiving data"""
        frame_buffer = bytearray()
        in_frame = False
        
        print("👂 Listening for packets...")
        print("🔹 Have another TNC transmit on the same frequency")
        print("🔹 Press Ctrl+C to stop")
        print("-" * 50)
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    
                    for byte in data:
                        if byte == self.FEND:
                            if in_frame and len(frame_buffer) > 0:
                                # Complete frame received
                                try:
                                    unescaped = self.unescape_kiss_data(frame_buffer)
                                    if len(unescaped) > 0:
                                        cmd = unescaped[0]
                                        payload = unescaped[1:] if len(unescaped) > 1 else b''
                                        
                                        if cmd == self.DATA_FRAME:
                                            self.process_data_frame(payload)
                                        else:
                                            print(f"ℹ Non-data frame: cmd=0x{cmd:02X}")
                                except Exception as e:
                                    print(f"⚠ Frame processing error: {e}")
                                
                                frame_buffer.clear()
                            in_frame = True
                        elif in_frame:
                            frame_buffer.append(byte)
                        # Ignore bytes outside of frames
                
                time.sleep(0.001)
                
            except Exception as e:
                if self.running:
                    print(f"RX error: {e}")
                break
    
    def run_monitor(self):
        """Run the packet monitor"""
        print("LoRaTNCX Simple KISS Packet Monitor")
        print("=" * 40)
        
        if not self.connect():
            return False
        
        # Start monitoring
        self.running = True
        self.rx_thread = Thread(target=self.rx_thread_func)
        self.rx_thread.daemon = True
        self.rx_thread.start()
        
        try:
            # Show periodic statistics
            last_stats = 0
            while True:
                time.sleep(1)
                now = time.time()
                if now - last_stats > 10:  # Every 10 seconds
                    elapsed = now - self.start_time
                    print(f"📊 Runtime: {elapsed:.0f}s, Packets: {self.packets_received}")
                    last_stats = now
                    
        except KeyboardInterrupt:
            print(f"\n\n📊 Final Statistics:")
            print(f"Runtime: {time.time() - self.start_time:.1f} seconds")
            print(f"Packets received: {self.packets_received}")
            if self.packets_received > 0:
                print("✅ SUCCESS: Packet reception is working!")
            else:
                print("❌ No packets received")
                print("💡 Possible issues:")
                print("   - Other TNC not transmitting")
                print("   - Frequency mismatch between TNCs")
                print("   - Radio configuration mismatch (SF, BW, CR)")
                print("   - Antenna connection issues")
        
        finally:
            self.disconnect()
        
        return True

def main():
    port = '/dev/ttyACM0'
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    monitor = SimpleKISSMonitor(port)
    monitor.run_monitor()

if __name__ == '__main__':
    main()