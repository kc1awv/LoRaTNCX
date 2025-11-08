#!/usr/bin/env python3
"""
LoRaTNCX Configuration Tool

Command-line utility to configure LoRa parameters on a LoRaTNCX KISS TNC.
This allows users to set up the radio before using standard KISS applications
that don't support hardware-specific configuration commands.

Usage:
    loratncx_config.py <port> --get-config
    loratncx_config.py <port> --get-battery
    loratncx_config.py <port> --get-board
    loratncx_config.py <port> --get-all
    loratncx_config.py <port> --frequency 915.0
    loratncx_config.py <port> --bandwidth 125
    loratncx_config.py <port> --spreading-factor 12
    loratncx_config.py <port> --coding-rate 7
    loratncx_config.py <port> --power 20
    loratncx_config.py <port> --syncword 0x1424
    loratncx_config.py <port> --save
    loratncx_config.py <port> --reset
    
Examples:
    # Get current configuration
    loratncx_config.py /dev/ttyUSB0 --get-config
    
    # Get battery voltage
    loratncx_config.py /dev/ttyUSB0 --get-battery
    
    # Get all hardware info (config + battery + board)
    loratncx_config.py /dev/ttyUSB0 --get-all
    
    # Set frequency to 915 MHz
    loratncx_config.py /dev/ttyUSB0 --frequency 915.0
    
    # Configure for long range (SF12, low bandwidth)
    loratncx_config.py /dev/ttyUSB0 --spreading-factor 12 --bandwidth 62.5
    
    # Save current config to NVS
    loratncx_config.py /dev/ttyUSB0 --save
    
    # Reset to factory defaults
    loratncx_config.py /dev/ttyUSB0 --reset
"""

import serial
import time
import sys
import argparse
import struct

# KISS Protocol Constants
FEND = 0xC0
FESC = 0xDB
TFEND = 0xDC
TFESC = 0xDD

# KISS Commands
CMD_DATA = 0x00
CMD_SETHARDWARE = 0x06
CMD_GETHARDWARE = 0x07

# SETHARDWARE Subcommands
HW_SET_FREQUENCY = 0x01
HW_SET_BANDWIDTH = 0x02
HW_SET_SPREADING = 0x03
HW_SET_CODINGRATE = 0x04
HW_SET_POWER = 0x05
HW_GET_CONFIG = 0x06
HW_SAVE_CONFIG = 0x07
HW_SET_SYNCWORD = 0x08
HW_RESET_CONFIG = 0xFF

# GETHARDWARE Subcommands
HW_QUERY_CONFIG = 0x01
HW_QUERY_BATTERY = 0x02
HW_QUERY_BOARD = 0x03
HW_QUERY_ALL = 0xFF


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


class LoRaTNCConfig:
    """LoRaTNCX configuration interface"""
    
    def __init__(self, port, baud=115200, timeout=2.0):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
    
    def open(self):
        """Open serial connection"""
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=self.timeout)
            time.sleep(0.5)  # Let port stabilize
            # Flush any pending data
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            return True
        except Exception as e:
            print(f"Error opening {self.port}: {e}")
            return False
    
    def close(self):
        """Close serial connection"""
        if self.ser:
            self.ser.close()
    
    def send_command(self, subcmd, data=None):
        """Send SETHARDWARE command"""
        frame = KISSFrame.encode_command(CMD_SETHARDWARE, subcmd, data)
        self.ser.write(frame)
        time.sleep(0.2)  # Give TNC time to process and reconfigure radio
    
    def send_query(self, subcmd, data=None):
        """Send GETHARDWARE query command"""
        frame = KISSFrame.encode_command(CMD_GETHARDWARE, subcmd, data)
        self.ser.write(frame)
        time.sleep(0.1)  # Brief delay for query processing
    
    def receive_frame(self, timeout=2.0):
        """Receive a KISS frame"""
        buffer = bytearray()
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                buffer.extend(data)
                
                # Look for complete KISS frame
                while len(buffer) >= 2:
                    # Find frame start
                    if buffer[0] != FEND:
                        buffer.pop(0)
                        continue
                    
                    # Look for frame end
                    try:
                        end_idx = buffer.index(FEND, 1)
                        frame_data = bytes(buffer[1:end_idx])
                        buffer = buffer[end_idx+1:]
                        
                        # Unescape and return
                        unescaped = KISSFrame.unescape(frame_data)
                        if len(unescaped) > 0:
                            return unescaped
                    except ValueError:
                        break  # No end marker yet
            
            time.sleep(0.01)
        
        return None
    
    def get_config(self):
        """Get current configuration from TNC"""
        # Send QUERY_CONFIG command
        self.send_query(HW_QUERY_CONFIG)
        
        # Wait for response
        frame = self.receive_frame(timeout=3.0)
        
        if not frame:
            return None
        
        # Parse response
        # Format: CMD_GETHARDWARE, HW_QUERY_CONFIG, freq(4), bw(4), sf(1), cr(1), pwr(1), sync(2)
        if len(frame) < 15 or frame[0] != CMD_GETHARDWARE or frame[1] != HW_QUERY_CONFIG:
            return None
        
        config = {}
        config['frequency'] = struct.unpack('<f', frame[2:6])[0]
        config['bandwidth'] = struct.unpack('<f', frame[6:10])[0]
        config['spreading_factor'] = frame[10]
        config['coding_rate'] = frame[11]
        config['power'] = struct.unpack('<b', bytes([frame[12]]))[0]
        config['syncword'] = struct.unpack('<H', frame[13:15])[0]
        
        return config
    
    def get_battery(self):
        """Get battery voltage from TNC"""
        # Send QUERY_BATTERY command
        self.send_query(HW_QUERY_BATTERY)
        
        # Wait for response
        frame = self.receive_frame(timeout=3.0)
        
        if not frame:
            return None
        
        # Parse response
        # Format: CMD_GETHARDWARE, HW_QUERY_BATTERY, voltage(4 bytes float)
        if len(frame) < 6 or frame[0] != CMD_GETHARDWARE or frame[1] != HW_QUERY_BATTERY:
            return None
        
        voltage = struct.unpack('<f', frame[2:6])[0]
        return voltage
    
    def get_board(self):
        """Get board information from TNC"""
        # Send QUERY_BOARD command
        self.send_query(HW_QUERY_BOARD)
        
        # Wait for response
        frame = self.receive_frame(timeout=3.0)
        
        if not frame:
            return None
        
        # Parse response
        # Format: CMD_GETHARDWARE, HW_QUERY_BOARD, board_type(1), board_name(string)
        if len(frame) < 3 or frame[0] != CMD_GETHARDWARE or frame[1] != HW_QUERY_BOARD:
            return None
        
        board = {}
        board['type'] = frame[2]
        board['name'] = frame[3:].decode('ascii', errors='ignore')
        
        return board
    
    def get_all(self):
        """Get all hardware information from TNC"""
        # Send QUERY_ALL command
        self.send_query(HW_QUERY_ALL)
        
        # Wait for multiple responses (config, battery, board)
        all_info = {
            'config': None,
            'battery': None,
            'board': None
        }
        
        # Receive up to 3 frames
        for _ in range(3):
            frame = self.receive_frame(timeout=2.0)
            if not frame or frame[0] != CMD_GETHARDWARE:
                continue
            
            subcmd = frame[1]
            
            if subcmd == HW_QUERY_CONFIG and len(frame) >= 15:
                config = {}
                config['frequency'] = struct.unpack('<f', frame[2:6])[0]
                config['bandwidth'] = struct.unpack('<f', frame[6:10])[0]
                config['spreading_factor'] = frame[10]
                config['coding_rate'] = frame[11]
                config['power'] = struct.unpack('<b', bytes([frame[12]]))[0]
                config['syncword'] = struct.unpack('<H', frame[13:15])[0]
                all_info['config'] = config
            
            elif subcmd == HW_QUERY_BATTERY and len(frame) >= 6:
                all_info['battery'] = struct.unpack('<f', frame[2:6])[0]
            
            elif subcmd == HW_QUERY_BOARD and len(frame) >= 3:
                board = {}
                board['type'] = frame[2]
                board['name'] = frame[3:].decode('ascii', errors='ignore')
                all_info['board'] = board
        
        return all_info
    
    def set_frequency(self, freq_mhz):
        """Set frequency in MHz (e.g., 915.0)"""
        data = struct.pack('<f', float(freq_mhz))
        self.send_command(HW_SET_FREQUENCY, data)
        print(f"✓ Set frequency to {freq_mhz} MHz")
    
    def set_bandwidth(self, bw_khz):
        """Set bandwidth in kHz (e.g., 125.0)"""
        data = struct.pack('<f', float(bw_khz))
        self.send_command(HW_SET_BANDWIDTH, data)
        print(f"✓ Set bandwidth to {bw_khz} kHz")
    
    def set_spreading_factor(self, sf):
        """Set spreading factor (6-12)"""
        data = bytes([int(sf)])
        self.send_command(HW_SET_SPREADING, data)
        print(f"✓ Set spreading factor to SF{sf}")
    
    def set_coding_rate(self, cr):
        """Set coding rate (5-8 for 4/5 to 4/8)"""
        data = bytes([int(cr)])
        self.send_command(HW_SET_CODINGRATE, data)
        print(f"✓ Set coding rate to 4/{cr}")
    
    def set_power(self, power_dbm):
        """Set output power in dBm (e.g., 20)"""
        data = struct.pack('<b', int(power_dbm))
        self.send_command(HW_SET_POWER, data)
        print(f"✓ Set output power to {power_dbm} dBm")
    
    def set_syncword(self, syncword):
        """Set sync word (2-byte value, e.g., 0x1424)"""
        if isinstance(syncword, str):
            syncword = int(syncword, 16)
        data = struct.pack('<H', int(syncword))
        self.send_command(HW_SET_SYNCWORD, data)
        print(f"✓ Set sync word to 0x{syncword:04X}")
    
    def save_config(self):
        """Save current configuration to NVS"""
        self.send_command(HW_SAVE_CONFIG)
        print("✓ Configuration saved to NVS (will persist across reboots)")
    
    def reset_config(self):
        """Reset to factory defaults"""
        self.send_command(HW_RESET_CONFIG)
        print("✓ Configuration reset to factory defaults")


def print_config(config):
    """Pretty-print configuration"""
    if not config:
        print("✗ Could not retrieve configuration")
        return
    
    print("\n" + "=" * 60)
    print("LoRaTNCX Current Configuration")
    print("=" * 60)
    print(f"  Frequency:        {config['frequency']:.3f} MHz")
    print(f"  Bandwidth:        {config['bandwidth']:.1f} kHz")
    print(f"  Spreading Factor: SF{config['spreading_factor']}")
    print(f"  Coding Rate:      4/{config['coding_rate']}")
    print(f"  Output Power:     {config['power']} dBm")
    print(f"  Sync Word:        0x{config['syncword']:04X}")
    print("=" * 60)
    
    # Calculate approximate air time for 50-byte packet
    # Simplified formula
    symbols = (8 + max(0, 8 + 16 - 4 * config['spreading_factor'])) + 4.25
    symbol_time = (2 ** config['spreading_factor']) / (config['bandwidth'] * 1000)
    airtime_ms = symbols * symbol_time * 1000
    print(f"\nEstimated 50-byte packet air time: {airtime_ms:.0f} ms")
    print()


def print_battery(voltage):
    """Pretty-print battery voltage"""
    if voltage is None:
        print("✗ Could not retrieve battery voltage")
        return
    
    print("\n" + "=" * 60)
    print("LoRaTNCX Battery Status")
    print("=" * 60)
    print(f"  Battery Voltage:  {voltage:.2f} V")
    
    # Battery status indicator
    if voltage >= 4.1:
        status = "Fully Charged"
    elif voltage >= 3.9:
        status = "Good"
    elif voltage >= 3.7:
        status = "Medium"
    elif voltage >= 3.4:
        status = "Low"
    else:
        status = "Critical - Charge Soon!"
    
    print(f"  Status:           {status}")
    print("=" * 60)
    print()


def print_board(board):
    """Pretty-print board information"""
    if not board:
        print("✗ Could not retrieve board information")
        return
    
    print("\n" + "=" * 60)
    print("LoRaTNCX Board Information")
    print("=" * 60)
    print(f"  Board Type:       V{board['type']}")
    print(f"  Board Name:       {board['name']}")
    print("=" * 60)
    print()


def print_all(all_info):
    """Pretty-print all hardware information"""
    if not all_info:
        print("✗ Could not retrieve hardware information")
        return
    
    if all_info['board']:
        print_board(all_info['board'])
    
    if all_info['config']:
        print_config(all_info['config'])
    
    if all_info['battery'] is not None:
        print_battery(all_info['battery'])


def main():
    parser = argparse.ArgumentParser(
        description='LoRaTNCX Configuration Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s /dev/ttyUSB0 --get-config
  %(prog)s /dev/ttyUSB0 --get-battery
  %(prog)s /dev/ttyUSB0 --get-all
  %(prog)s /dev/ttyUSB0 --frequency 915.0 --save
  %(prog)s /dev/ttyUSB0 --spreading-factor 7 --bandwidth 250
  %(prog)s /dev/ttyUSB0 --reset
        """
    )
    
    parser.add_argument('port', help='Serial port (e.g., /dev/ttyUSB0, COM3)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate (default: 115200)')
    
    # Query commands
    parser.add_argument('--get-config', '-g', action='store_true', help='Get current radio configuration')
    parser.add_argument('--get-battery', '-B', action='store_true', help='Get battery voltage')
    parser.add_argument('--get-board', '-I', action='store_true', help='Get board information')
    parser.add_argument('--get-all', '-A', action='store_true', help='Get all hardware info (config + battery + board)')
    
    # Configuration commands
    parser.add_argument('--frequency', '-f', type=float, metavar='MHZ', help='Set frequency in MHz (e.g., 915.0)')
    parser.add_argument('--bandwidth', '-b', type=float, metavar='KHZ', help='Set bandwidth in kHz (e.g., 125.0)')
    parser.add_argument('--spreading-factor', '-s', type=int, metavar='SF', choices=range(6, 13), help='Set spreading factor (6-12)')
    parser.add_argument('--coding-rate', '-c', type=int, metavar='CR', choices=range(5, 9), help='Set coding rate (5-8 for 4/5 to 4/8)')
    parser.add_argument('--power', '-p', type=int, metavar='DBM', help='Set output power in dBm (e.g., 20)')
    parser.add_argument('--syncword', '-w', metavar='WORD', help='Set sync word (e.g., 0x1424)')
    parser.add_argument('--save', action='store_true', help='Save configuration to NVS')
    parser.add_argument('--reset', action='store_true', help='Reset to factory defaults')
    
    args = parser.parse_args()
    
    # Check if any action was specified
    has_action = (args.get_config or args.get_battery or args.get_board or args.get_all or
                  args.frequency or args.bandwidth or args.spreading_factor or 
                  args.coding_rate or args.power or args.syncword or args.save or args.reset)
    
    if not has_action:
        parser.print_help()
        sys.exit(1)
    
    # Open TNC
    tnc = LoRaTNCConfig(args.port, args.baud)
    
    print(f"Opening {args.port}...")
    if not tnc.open():
        sys.exit(1)
    
    print("✓ Connected to LoRaTNCX\n")
    
    try:
        # Handle queries first (if only querying, do it and exit)
        if args.get_all:
            all_info = tnc.get_all()
            print_all(all_info)
            return
        
        if args.get_battery:
            voltage = tnc.get_battery()
            print_battery(voltage)
            if not (args.get_config or args.get_board):
                return
        
        if args.get_board:
            board = tnc.get_board()
            print_board(board)
            if not args.get_config:
                return
        
        # Apply configuration changes
        if args.frequency:
            tnc.set_frequency(args.frequency)
        
        if args.bandwidth:
            tnc.set_bandwidth(args.bandwidth)
        
        if args.spreading_factor:
            tnc.set_spreading_factor(args.spreading_factor)
        
        if args.coding_rate:
            tnc.set_coding_rate(args.coding_rate)
        
        if args.power:
            tnc.set_power(args.power)
        
        if args.syncword:
            tnc.set_syncword(args.syncword)
        
        if args.reset:
            tnc.reset_config()
        
        if args.save:
            tnc.save_config()
        
        # Always show config at the end (or if explicitly requested)
        if args.get_config or has_action:
            time.sleep(1.0)  # Let changes settle and radio reconfigure
            config = tnc.get_config()
            print_config(config)
    
    finally:
        tnc.close()


if __name__ == '__main__':
    main()
