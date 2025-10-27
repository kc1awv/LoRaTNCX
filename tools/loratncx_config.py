#!/usr/bin/env python3
"""
LoRaTNCX Configuration Tool

A command-line utility for configuring LoRaTNCX devices using KISS protocol commands.
This tool provides an easy way to set radio parameters, timing values, and query
device configuration.

Usage:
    python3 loratncx_config.py [options]

Examples:
    # Set frequency to 433.175 MHz
    python3 loratncx_config.py --port /dev/ttyACM0 --frequency 433.175

    # Configure for long-range (SF12, CR 4/8)
    python3 loratncx_config.py --port /dev/ttyACM0 --sf 12 --cr 8 --power 20

    # Set CSMA parameters for busy network
    python3 loratncx_config.py --port /dev/ttyACM0 --txdelay 50 --persist 31 --slottime 20

    # Get current configuration
    python3 loratncx_config.py --port /dev/ttyACM0 --get-config

    # Interactive mode
    python3 loratncx_config.py --port /dev/ttyACM0 --interactive

Author: LoRaTNCX Project
License: Open Source
"""

import argparse
import serial
import struct
import time
import sys
from typing import Optional, Union


class KISSCommands:
    """KISS protocol command constants."""
    # Standard KISS commands
    DATA_FRAME = 0x00
    TX_DELAY = 0x01
    PERSISTENCE = 0x02
    SLOT_TIME = 0x03
    TX_TAIL = 0x04       # Not implemented
    FULL_DUPLEX = 0x05   # Not implemented
    
    # Hardware commands
    SET_HARDWARE = 0x06
    SET_FREQUENCY = 0x01
    SET_TX_POWER = 0x02
    SET_BANDWIDTH = 0x03
    SET_SPREADING_FACTOR = 0x04
    SET_CODING_RATE = 0x05
    GET_CONFIG = 0x10

    # Frame constants
    FEND = 0xC0  # Frame End
    FESC = 0xDB  # Frame Escape
    TFEND = 0xDC # Transposed Frame End
    TFESC = 0xDD # Transposed Frame Escape


class LoRaTNCXConfig:
    """LoRaTNCX configuration utility using KISS protocol."""
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0, debug: bool = False):
        """Initialize the configuration tool.
        
        Args:
            port: Serial port path (e.g., '/dev/ttyACM0', 'COM3')
            baudrate: Serial communication speed
            timeout: Serial read timeout in seconds
            debug: Whether to show debug output from device
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.debug = debug
        self.serial = None
        
        # Valid parameter ranges and options
        self.valid_bandwidths = [7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0]
        self.min_frequency = 150.0
        self.max_frequency = 960.0
        self.min_power = -9
        self.max_power = 22
        self.min_sf = 7
        self.max_sf = 12
        self.min_cr = 5
        self.max_cr = 8

    def connect(self) -> bool:
        """Establish serial connection to the LoRaTNCX device.
        
        Returns:
            True if connection successful, False otherwise
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            print(f"Connected to LoRaTNCX on {self.port}")
            time.sleep(1)  # Allow device to stabilize
            
            # Clear any existing debug output from the buffer
            self._clear_serial_buffer()
            
            return True
        except serial.SerialException as e:
            print(f"Error: Failed to connect to {self.port}: {e}")
            return False

    def disconnect(self):
        """Close the serial connection."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("Disconnected from LoRaTNCX")

    def _clear_serial_buffer(self):
        """Clear any existing data from the serial buffer."""
        if self.serial and self.serial.is_open:
            # Read and discard any pending data
            if self.serial.in_waiting > 0:
                discarded = self.serial.read(self.serial.in_waiting)
                if self.debug:
                    print(f"Debug output ({len(discarded)} bytes):")
                    try:
                        print(discarded.decode('utf-8', errors='ignore'))
                    except:
                        print(discarded.hex())
                else:
                    print(f"Cleared {len(discarded)} bytes of debug output from buffer")

    def _read_response_with_timeout(self, timeout_seconds: float = 2.0) -> bytes:
        """Read response from device, filtering out debug messages.
        
        Args:
            timeout_seconds: Maximum time to wait for response
            
        Returns:
            Response bytes (may be empty if no valid response)
        """
        if not self.serial or not self.serial.is_open:
            return b''
            
        start_time = time.time()
        response_data = b''
        
        while time.time() - start_time < timeout_seconds:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                response_data += data
                
                # Look for KISS frames (start with 0xC0)
                # or check if we have what looks like configuration output
                if b'Current Radio Configuration:' in response_data:
                    break
                    
            time.sleep(0.1)
        
        return response_data

    def _parse_config_response(self, response: bytes) -> str:
        """Parse configuration response, filtering debug output.
        
        Args:
            response: Raw response bytes from device
            
        Returns:
            Parsed configuration string
        """
        try:
            response_str = response.decode('utf-8', errors='ignore')
            
            # Look for configuration block
            if 'Current Radio Configuration:' in response_str:
                # Extract just the configuration part
                config_start = response_str.find('Current Radio Configuration:')
                config_lines = []
                lines = response_str[config_start:].split('\n')
                
                for line in lines:
                    # Skip debug messages that start with [LOOP], [DEBUG], etc.
                    if (line.strip() and 
                        not line.strip().startswith('[') and 
                        ('Current Radio Configuration:' in line or
                         'Frequency:' in line or
                         'TX Power:' in line or
                         'Bandwidth:' in line or
                         'Spreading Factor:' in line or
                         'Coding Rate:' in line or
                         'TX Delay:' in line or
                         'Persistence:' in line or
                         'Slot Time:' in line)):
                        config_lines.append(line.strip())
                
                return '\n'.join(config_lines)
            else:
                # Filter out debug messages
                lines = response_str.split('\n')
                filtered_lines = []
                for line in lines:
                    if (line.strip() and 
                        not line.strip().startswith('[LOOP]') and
                        not line.strip().startswith('[DEBUG]') and
                        not 'uptime:' in line and
                        not 'free heap:' in line and
                        not 'bytes' in line.strip()):
                        filtered_lines.append(line.strip())
                
                return '\n'.join(filtered_lines) if filtered_lines else "No configuration data received"
                
        except Exception as e:
            return f"Error parsing response: {e}"

    def monitor_debug_output(self, duration: int = 10):
        """Monitor and display debug output from the device.
        
        Args:
            duration: How long to monitor in seconds
        """
        if not self.serial or not self.serial.is_open:
            print("Error: Not connected to device")
            return
            
        print(f"Monitoring debug output for {duration} seconds (press Ctrl+C to stop early)...")
        print("-" * 50)
        
        start_time = time.time()
        try:
            while time.time() - start_time < duration:
                if self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    try:
                        output = data.decode('utf-8', errors='ignore')
                        print(output, end='')
                    except:
                        print(f"[Binary data: {data.hex()}]")
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\nMonitoring stopped by user")
        
        print("-" * 50)
        print("Debug monitoring complete")

    def escape_kiss_data(self, data: bytes) -> bytes:
        """Apply KISS byte stuffing to prevent frame marker conflicts.
        
        Args:
            data: Raw data bytes
            
        Returns:
            Escaped data bytes
        """
        # Replace FESC first to avoid double-escaping
        data = data.replace(bytes([KISSCommands.FESC]), 
                           bytes([KISSCommands.FESC, KISSCommands.TFESC]))
        data = data.replace(bytes([KISSCommands.FEND]), 
                           bytes([KISSCommands.FESC, KISSCommands.TFEND]))
        return data

    def send_kiss_command(self, cmd: int, data: bytes = b'', clear_buffer: bool = False) -> bool:
        """Send a KISS command to the device.
        
        Args:
            cmd: KISS command byte
            data: Optional command data
            clear_buffer: Whether to clear the serial buffer before sending
            
        Returns:
            True if command sent successfully, False otherwise
        """
        if not self.serial or not self.serial.is_open:
            print("Error: Not connected to device")
            return False

        try:
            # Clear buffer if requested (useful to avoid mixing with debug output)
            if clear_buffer:
                self._clear_serial_buffer()
            
            # Build frame data
            frame_data = bytes([cmd]) + data
            frame_data = self.escape_kiss_data(frame_data)
            
            # Add frame markers
            frame = bytes([KISSCommands.FEND]) + frame_data + bytes([KISSCommands.FEND])
            
            # Send frame
            self.serial.write(frame)
            self.serial.flush()
            time.sleep(0.1)  # Allow processing time
            
            return True
        except serial.SerialException as e:
            print(f"Error: Failed to send command: {e}")
            return False

    def set_frequency(self, frequency: float) -> bool:
        """Set the LoRa operating frequency.
        
        Args:
            frequency: Frequency in MHz (150.0 - 960.0)
            
        Returns:
            True if command sent successfully
        """
        if not (self.min_frequency <= frequency <= self.max_frequency):
            print(f"Error: Frequency must be between {self.min_frequency} and {self.max_frequency} MHz")
            return False

        # Pack frequency as IEEE 754 float, little-endian
        freq_data = struct.pack('<f', frequency)
        data = bytes([KISSCommands.SET_FREQUENCY]) + freq_data
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print(f"Set frequency to {frequency} MHz")
            return True
        return False

    def set_tx_power(self, power: int) -> bool:
        """Set the transmitter output power.
        
        Args:
            power: Power in dBm (-9 to +22)
            
        Returns:
            True if command sent successfully
        """
        if not (self.min_power <= power <= self.max_power):
            print(f"Error: TX power must be between {self.min_power} and {self.max_power} dBm")
            return False

        # Pack power as signed 8-bit integer
        power_data = struct.pack('b', power)
        data = bytes([KISSCommands.SET_TX_POWER]) + power_data
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print(f"Set TX power to {power} dBm")
            return True
        return False

    def set_bandwidth(self, bandwidth: float) -> bool:
        """Set the LoRa signal bandwidth.
        
        Args:
            bandwidth: Bandwidth in kHz (must be one of the valid options)
            
        Returns:
            True if command sent successfully
        """
        if bandwidth not in self.valid_bandwidths:
            print(f"Error: Bandwidth must be one of: {self.valid_bandwidths} kHz")
            return False

        # Pack bandwidth as IEEE 754 float, little-endian
        bw_data = struct.pack('<f', bandwidth)
        data = bytes([KISSCommands.SET_BANDWIDTH]) + bw_data
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print(f"Set bandwidth to {bandwidth} kHz")
            return True
        return False

    def set_spreading_factor(self, sf: int) -> bool:
        """Set the LoRa spreading factor.
        
        Args:
            sf: Spreading factor (7-12)
            
        Returns:
            True if command sent successfully
        """
        if not (self.min_sf <= sf <= self.max_sf):
            print(f"Error: Spreading factor must be between {self.min_sf} and {self.max_sf}")
            return False

        data = bytes([KISSCommands.SET_SPREADING_FACTOR, sf])
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print(f"Set spreading factor to {sf}")
            return True
        return False

    def set_coding_rate(self, cr: int) -> bool:
        """Set the LoRa coding rate.
        
        Args:
            cr: Coding rate (5-8, representing 4/5 to 4/8)
            
        Returns:
            True if command sent successfully
        """
        if not (self.min_cr <= cr <= self.max_cr):
            print(f"Error: Coding rate must be between {self.min_cr} and {self.max_cr}")
            return False

        data = bytes([KISSCommands.SET_CODING_RATE, cr])
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print(f"Set coding rate to {cr} (4/{cr})")
            return True
        return False

    def set_tx_delay(self, delay: int) -> bool:
        """Set the pre-transmission delay.
        
        Args:
            delay: Delay in units of 10ms (0-255)
            
        Returns:
            True if command sent successfully
        """
        if not (0 <= delay <= 255):
            print("Error: TX delay must be between 0 and 255 (0-2550ms)")
            return False

        if self.send_kiss_command(KISSCommands.TX_DELAY, bytes([delay])):
            print(f"Set TX delay to {delay} x 10ms ({delay * 10}ms)")
            return True
        return False

    def set_persistence(self, persistence: int) -> bool:
        """Set the p-persistent CSMA probability.
        
        Args:
            persistence: Persistence value (0-255)
            
        Returns:
            True if command sent successfully
        """
        if not (0 <= persistence <= 255):
            print("Error: Persistence must be between 0 and 255")
            return False

        probability = (persistence + 1) / 256 * 100
        if self.send_kiss_command(KISSCommands.PERSISTENCE, bytes([persistence])):
            print(f"Set persistence to {persistence} (~{probability:.1f}% transmission probability)")
            return True
        return False

    def set_slot_time(self, slot_time: int) -> bool:
        """Set the CSMA back-off interval.
        
        Args:
            slot_time: Slot time in units of 10ms (0-255)
            
        Returns:
            True if command sent successfully
        """
        if not (0 <= slot_time <= 255):
            print("Error: Slot time must be between 0 and 255 (0-2550ms)")
            return False

        if self.send_kiss_command(KISSCommands.SLOT_TIME, bytes([slot_time])):
            print(f"Set slot time to {slot_time} x 10ms ({slot_time * 10}ms)")
            return True
        return False

    def get_configuration(self) -> bool:
        """Request current configuration from the device.
        
        Returns:
            True if command sent successfully
        """
        # Clear buffer before sending command
        self._clear_serial_buffer()
        
        data = bytes([KISSCommands.GET_CONFIG])
        
        if self.send_kiss_command(KISSCommands.SET_HARDWARE, data):
            print("Requesting current configuration...")
            
            # Wait for and read response
            response = self._read_response_with_timeout(3.0)
            
            if response:
                config_text = self._parse_config_response(response)
                if config_text.strip():
                    print("\nCurrent Configuration:")
                    print(config_text)
                else:
                    print("No configuration data received. Device may not support this command.")
                    print(f"Raw response ({len(response)} bytes):", response[:200] + (b'...' if len(response) > 200 else b''))
            else:
                print("No response received from device.")
                print("Note: Configuration may be printed to device's debug console.")
            
            return True
        return False

    def interactive_mode(self):
        """Run interactive configuration mode."""
        print("\n=== LoRaTNCX Interactive Configuration ===")
        print("Enter commands or 'help' for options. Type 'quit' to exit.\n")
        
        while True:
            try:
                cmd = input("LoRaTNCX> ").strip().lower()
                
                if cmd == 'quit' or cmd == 'exit':
                    break
                elif cmd == 'help' or cmd == '?':
                    self.print_interactive_help()
                elif cmd == 'config' or cmd == 'get':
                    self.get_configuration()
                elif cmd.startswith('freq '):
                    try:
                        freq = float(cmd.split()[1])
                        self.set_frequency(freq)
                    except (ValueError, IndexError):
                        print("Usage: freq <frequency_mhz>")
                elif cmd.startswith('power '):
                    try:
                        power = int(cmd.split()[1])
                        self.set_tx_power(power)
                    except (ValueError, IndexError):
                        print("Usage: power <power_dbm>")
                elif cmd.startswith('bw '):
                    try:
                        bw = float(cmd.split()[1])
                        self.set_bandwidth(bw)
                    except (ValueError, IndexError):
                        print("Usage: bw <bandwidth_khz>")
                elif cmd.startswith('sf '):
                    try:
                        sf = int(cmd.split()[1])
                        self.set_spreading_factor(sf)
                    except (ValueError, IndexError):
                        print("Usage: sf <spreading_factor>")
                elif cmd.startswith('cr '):
                    try:
                        cr = int(cmd.split()[1])
                        self.set_coding_rate(cr)
                    except (ValueError, IndexError):
                        print("Usage: cr <coding_rate>")
                elif cmd.startswith('txdelay '):
                    try:
                        delay = int(cmd.split()[1])
                        self.set_tx_delay(delay)
                    except (ValueError, IndexError):
                        print("Usage: txdelay <delay_units>")
                elif cmd.startswith('persist '):
                    try:
                        persist = int(cmd.split()[1])
                        self.set_persistence(persist)
                    except (ValueError, IndexError):
                        print("Usage: persist <persistence_value>")
                elif cmd.startswith('slottime '):
                    try:
                        slot = int(cmd.split()[1])
                        self.set_slot_time(slot)
                    except (ValueError, IndexError):
                        print("Usage: slottime <slot_time_units>")
                elif cmd == 'monitor' or cmd == 'debug':
                    self.monitor_debug_output()
                elif cmd == '':
                    continue
                else:
                    print(f"Unknown command: {cmd}. Type 'help' for available commands.")
                    
            except KeyboardInterrupt:
                print("\nExiting...")
                break
            except Exception as e:
                print(f"Error: {e}")

    def print_interactive_help(self):
        """Print interactive mode help."""
        print("""
Available Commands:
  freq <mhz>          Set frequency (150.0 - 960.0 MHz)
  power <dbm>         Set TX power (-9 to +22 dBm)
  bw <khz>            Set bandwidth (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0)
  sf <factor>         Set spreading factor (7-12)
  cr <rate>           Set coding rate (5-8, representing 4/5 to 4/8)
  txdelay <units>     Set TX delay (0-255, units of 10ms)
  persist <value>     Set persistence (0-255)
  slottime <units>    Set slot time (0-255, units of 10ms)
  config              Get current configuration
  monitor             Monitor debug output from device (10 seconds)
  help                Show this help
  quit                Exit interactive mode

Examples:
  freq 433.175        Set to 433.175 MHz
  power 14            Set to 14 dBm
  sf 9                Set spreading factor to 9
  cr 7                Set coding rate to 4/7
  monitor             Show device debug messages
""")


def main():
    """Main entry point for the configuration tool."""
    parser = argparse.ArgumentParser(
        description="LoRaTNCX Configuration Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --port /dev/ttyACM0 --frequency 433.175
  %(prog)s --port COM3 --sf 12 --cr 8 --power 20
  %(prog)s --port /dev/ttyACM0 --get-config
  %(prog)s --port /dev/ttyACM0 --interactive

Regional Frequency Guidelines:
  ISM 433 MHz: 433.050 - 434.790 MHz (EMEA, Russia, APAC)
  ISM 868 MHz: 863.000 - 870.000 MHz (Europe)
  ISM 915 MHz: 902.000 - 928.000 MHz (Americas)
  Amateur bands: Check your license privileges
        """
    )
    
    # Connection parameters
    parser.add_argument('--port', '-p', required=True,
                       help='Serial port (e.g., /dev/ttyACM0, COM3)')
    parser.add_argument('--baudrate', '-b', type=int, default=115200,
                       help='Baud rate (default: 115200)')
    parser.add_argument('--timeout', '-t', type=float, default=2.0,
                       help='Serial timeout in seconds (default: 2.0)')
    
    # Radio parameters
    parser.add_argument('--frequency', '--freq', '-f', type=float,
                       help='Set frequency in MHz (150.0 - 960.0)')
    parser.add_argument('--power', type=int,
                       help='Set TX power in dBm (-9 to +22)')
    parser.add_argument('--bandwidth', '--bw', type=float,
                       help='Set bandwidth in kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0)')
    parser.add_argument('--spreading-factor', '--sf', type=int,
                       help='Set spreading factor (7-12)')
    parser.add_argument('--coding-rate', '--cr', type=int,
                       help='Set coding rate (5-8, representing 4/5 to 4/8)')
    
    # CSMA parameters
    parser.add_argument('--txdelay', type=int,
                       help='Set TX delay in 10ms units (0-255)')
    parser.add_argument('--persistence', '--persist', type=int,
                       help='Set persistence value (0-255)')
    parser.add_argument('--slottime', type=int,
                       help='Set slot time in 10ms units (0-255)')
    
    # Actions
    parser.add_argument('--get-config', action='store_true',
                       help='Get current configuration')
    parser.add_argument('--interactive', '-i', action='store_true',
                       help='Run in interactive mode')
    parser.add_argument('--debug', '-d', action='store_true',
                       help='Show debug output from device')
    parser.add_argument('--monitor', '-m', type=int, metavar='SECONDS',
                       help='Monitor debug output for specified seconds')

    args = parser.parse_args()

    # Create configuration tool instance
    config_tool = LoRaTNCXConfig(args.port, args.baudrate, args.timeout, args.debug)
    
    # Connect to device
    if not config_tool.connect():
        sys.exit(1)

    try:
        # Execute commands based on arguments
        commands_executed = False
        
        if args.frequency:
            config_tool.set_frequency(args.frequency)
            commands_executed = True
            
        if args.power is not None:
            config_tool.set_tx_power(args.power)
            commands_executed = True
            
        if args.bandwidth:
            config_tool.set_bandwidth(args.bandwidth)
            commands_executed = True
            
        if args.spreading_factor:
            config_tool.set_spreading_factor(args.spreading_factor)
            commands_executed = True
            
        if args.coding_rate:
            config_tool.set_coding_rate(args.coding_rate)
            commands_executed = True
            
        if args.txdelay is not None:
            config_tool.set_tx_delay(args.txdelay)
            commands_executed = True
            
        if args.persistence is not None:
            config_tool.set_persistence(args.persistence)
            commands_executed = True
            
        if args.slottime is not None:
            config_tool.set_slot_time(args.slottime)
            commands_executed = True
            
        if args.get_config:
            config_tool.get_configuration()
            commands_executed = True
            
        if args.interactive:
            config_tool.interactive_mode()
            commands_executed = True
            
        if args.monitor:
            config_tool.monitor_debug_output(args.monitor)
            commands_executed = True
            
        if not commands_executed:
            print("No configuration commands specified. Use --help for usage information.")
            print("Or use --interactive for interactive mode.")
            print("Use --monitor 10 to watch debug output, or --debug to see debug output during commands.")
            
    except KeyboardInterrupt:
        print("\nOperation cancelled by user")
    finally:
        config_tool.disconnect()


if __name__ == '__main__':
    main()