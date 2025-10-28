#!/usr/bin/env python3
"""
LoRaTNCX KISS Reception Diagnostic Tool

This tool helps diagnose why you're not seeing received packets in your KISS TNC.
It performs systematic testing to identify where the issue lies.
"""

import serial
import time
import sys
import struct
from threading import Thread, Event
import queue

class KISSReceptionDiagnostic:
    """Diagnostic tool for KISS reception issues"""
    
    # KISS constants
    FEND = 0xC0
    FESC = 0xDB
    TFEND = 0xDC
    TFESC = 0xDD
    
    # KISS commands
    DATA_FRAME = 0x00
    SET_HARDWARE = 0x06
    GET_CONFIG = 0x10  # Hardware sub-command
    
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.port_name = port
        self.baudrate = baudrate
        self.serial_port = None
        self.running = False
        self.rx_thread = None
        self.raw_data_queue = queue.Queue()
        self.kiss_frames_queue = queue.Queue()
        
        # Statistics
        self.stats = {
            'raw_bytes': 0,
            'kiss_frames': 0,
            'data_frames': 0,
            'other_frames': 0,
            'frame_errors': 0,
            'test_start_time': 0.0
        }
    
    def connect(self):
        """Connect to the TNC"""
        try:
            self.serial_port = serial.Serial(
                self.port_name,
                self.baudrate,
                timeout=1,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False
            )
            print(f"✓ Connected to {self.port_name} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"✗ Failed to connect to {self.port_name}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from TNC"""
        self.running = False
        if self.rx_thread:
            self.rx_thread.join(timeout=2)
        if self.serial_port:
            self.serial_port.close()
            print("✓ Disconnected from TNC")
    
    def escape_kiss_data(self, data):
        """Apply KISS byte stuffing"""
        result = bytearray()
        for byte in data:
            if byte == self.FEND:
                result.extend([self.FESC, self.TFEND])
            elif byte == self.FESC:
                result.extend([self.FESC, self.TFESC])
            else:
                result.append(byte)
        return bytes(result)
    
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
                    # Invalid escape sequence
                    result.append(byte)
                escaped = False
            elif byte == self.FESC:
                escaped = True
            else:
                result.append(byte)
        return bytes(result)
    
    def send_kiss_command(self, cmd, data=b''):
        """Send a KISS command"""
        if not self.serial_port:
            return False
        
        frame = bytearray([self.FEND])
        frame.extend(self.escape_kiss_data(bytes([cmd]) + data))
        frame.append(self.FEND)
        
        self.serial_port.write(frame)
        self.serial_port.flush()
        print(f"→ Sent KISS command 0x{cmd:02X} with {len(data)} data bytes")
        return True
    
    def request_config(self):
        """Request current TNC configuration"""
        print("\n=== Requesting TNC Configuration ===")
        data = bytes([self.GET_CONFIG])
        return self.send_kiss_command(self.SET_HARDWARE, data)
    
    def rx_thread_func(self):
        """Background thread for receiving data"""
        frame_buffer = bytearray()
        in_frame = False
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    
                    # Log all raw data
                    for byte in data:
                        self.raw_data_queue.put(byte)
                        self.stats['raw_bytes'] += 1
                        
                        # KISS frame parsing
                        if byte == self.FEND:
                            if in_frame and len(frame_buffer) > 0:
                                # Complete frame received
                                self.kiss_frames_queue.put(bytes(frame_buffer))
                                self.stats['kiss_frames'] += 1
                                frame_buffer.clear()
                            in_frame = True
                        elif in_frame:
                            frame_buffer.append(byte)
                
                time.sleep(0.001)  # Small delay
                
            except Exception as e:
                if self.running:
                    print(f"RX thread error: {e}")
                break
    
    def process_raw_data(self):
        """Process and display raw data"""
        raw_bytes = []
        while not self.raw_data_queue.empty():
            try:
                byte = self.raw_data_queue.get_nowait()
                raw_bytes.append(byte)
            except queue.Empty:
                break
        
        if raw_bytes:
            # Display as hex
            hex_str = ' '.join(f'{b:02X}' for b in raw_bytes[:50])  # Limit display
            if len(raw_bytes) > 50:
                hex_str += f' ... (+{len(raw_bytes)-50} bytes)'
            
            print(f"📡 Raw data ({len(raw_bytes)} bytes): {hex_str}")
            
            # Check for patterns
            if any(b == self.FEND for b in raw_bytes):
                fend_count = raw_bytes.count(self.FEND)
                print(f"   ✓ Contains {fend_count} FEND markers (0xC0)")
            else:
                print("   ⚠ No FEND markers found - not KISS data")
            
            # Check for printable ASCII (debug output)
            try:
                ascii_str = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in raw_bytes[:100])
                if any(32 <= b <= 126 for b in raw_bytes):
                    print(f"   📝 ASCII content: {ascii_str}")
            except:
                pass
    
    def process_kiss_frames(self):
        """Process and display KISS frames"""
        while not self.kiss_frames_queue.empty():
            try:
                frame = self.kiss_frames_queue.get_nowait()
                self.analyze_kiss_frame(frame)
            except queue.Empty:
                break
    
    def analyze_kiss_frame(self, frame):
        """Analyze a complete KISS frame"""
        if len(frame) == 0:
            return
        
        # Unescape the frame
        try:
            unescaped = self.unescape_kiss_data(frame)
            if len(unescaped) == 0:
                return
            
            cmd = unescaped[0]
            data = unescaped[1:] if len(unescaped) > 1 else b''
            
            print(f"📦 KISS Frame: cmd=0x{cmd:02X}, data_len={len(data)}")
            
            # Analyze command
            if cmd == self.DATA_FRAME:
                self.stats['data_frames'] += 1
                print(f"   ✓ DATA FRAME - This is a received packet!")
                if data:
                    # Try to decode as text
                    try:
                        text = data.decode('utf-8', errors='ignore').strip()
                        if text:
                            print(f"   📄 Content: '{text}'")
                    except:
                        pass
                    
                    # Show hex dump
                    hex_dump = ' '.join(f'{b:02X}' for b in data[:32])
                    if len(data) > 32:
                        hex_dump += '...'
                    print(f"   🔍 Hex: {hex_dump}")
            else:
                self.stats['other_frames'] += 1
                print(f"   ℹ Command frame (0x{cmd:02X})")
                if data:
                    hex_dump = ' '.join(f'{b:02X}' for b in data[:16])
                    print(f"   📊 Data: {hex_dump}")
                    
        except Exception as e:
            self.stats['frame_errors'] += 1
            print(f"   ✗ Frame parse error: {e}")
    
    def print_statistics(self):
        """Print current statistics"""
        elapsed = time.time() - self.stats['test_start_time']
        print(f"\n=== Statistics (after {elapsed:.1f}s) ===")
        print(f"Raw bytes received: {self.stats['raw_bytes']}")
        print(f"KISS frames parsed: {self.stats['kiss_frames']}")
        print(f"Data frames (RX packets): {self.stats['data_frames']}")
        print(f"Other frames (commands): {self.stats['other_frames']}")
        print(f"Frame parse errors: {self.stats['frame_errors']}")
    
    def run_diagnostic(self):
        """Run the complete diagnostic sequence"""
        print("LoRaTNCX KISS Reception Diagnostic Tool")
        print("="*50)
        
        if not self.connect():
            return False
        
        # Start monitoring thread
        self.running = True
        self.rx_thread = Thread(target=self.rx_thread_func)
        self.rx_thread.daemon = True
        self.rx_thread.start()
        self.stats['test_start_time'] = time.time()
        
        try:
            print("\n=== Phase 1: Initial Configuration Check ===")
            self.request_config()
            time.sleep(2)
            self.process_raw_data()
            self.process_kiss_frames()
            
            print("\n=== Phase 2: Monitoring for Received Packets ===")
            print("Listening for received data...")
            print("1. Have another station transmit on your frequency")
            print("2. Or use a LoRa module to send test packets")
            print("3. Press Ctrl+C to stop monitoring")
            
            last_stats_time = time.time()
            
            while True:
                time.sleep(0.1)
                self.process_raw_data()
                self.process_kiss_frames()
                
                # Print stats every 10 seconds
                if time.time() - last_stats_time > 10:
                    self.print_statistics()
                    last_stats_time = time.time()
                    
        except KeyboardInterrupt:
            print("\n\n=== Diagnostic Complete ===")
            self.print_statistics()
            
            # Analysis and recommendations
            print("\n=== Analysis ===")
            if self.stats['raw_bytes'] == 0:
                print("✗ No data received from TNC")
                print("  - Check serial port connection")
                print("  - Verify TNC is powered and running")
                print("  - Try different baud rate")
            elif self.stats['kiss_frames'] == 0:
                print("✗ Raw data received but no KISS frames")
                print("  - TNC may be in debug/console mode")
                print("  - Try sending KISS commands to switch modes")
                print("  - Check for ASCII debug output above")
            elif self.stats['data_frames'] == 0:
                print("✗ KISS frames received but no data frames")
                print("  - TNC is responding but not receiving radio packets")
                print("  - Check radio configuration (frequency, SF, BW)")
                print("  - Verify antenna connection")
                print("  - Have another station transmit test packets")
            else:
                print("✓ Success! Received data frames detected")
                print(f"  - Found {self.stats['data_frames']} received packets")
                print("  - KISS reception is working correctly")
        
        finally:
            self.disconnect()
        
        return True

def main():
    """Main function"""
    port = '/dev/ttyUSB0'
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    diagnostic = KISSReceptionDiagnostic(port)
    diagnostic.run_diagnostic()

if __name__ == '__main__':
    main()