#!/usr/bin/env python3
"""
LoRa TNC Terminal Interface
A user-friendly terminal interface for interacting with the LoRaTNCX device
"""

import curses
import serial
import threading
import time
import sys
import argparse
from datetime import datetime
from collections import deque

class TNCTerminal:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.running = False
        
        # Buffers for different windows
        self.tnc_output = deque(maxlen=1000)
        self.packet_log = deque(maxlen=500)
        self.input_buffer = ""
        self.input_history = deque(maxlen=100)
        self.history_index = 0
        
        # Window objects
        self.stdscr = None
        self.tnc_win = None
        self.packet_win = None
        self.input_win = None
        self.status_win = None
        
        # Tracking state
        self.connected = False
        self.in_converse_mode = False
        
        # Scrolling state
        self.tnc_scroll_offset = 0
        self.packet_scroll_offset = 0
        
    def connect_serial(self):
        """Connect to the TNC via serial port"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            self.connected = True
            return True
        except Exception as e:
            self.add_tnc_output(f"ERROR: Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect_serial(self):
        """Disconnect from the TNC"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
        self.connected = False
    
    def setup_windows(self, stdscr):
        """Setup the curses windows"""
        self.stdscr = stdscr
        curses.curs_set(1)  # Show cursor
        
        # Get terminal size
        height, width = stdscr.getmaxyx()
        
        # Create windows
        # TNC Output window (top half)
        tnc_height = height // 2 - 1
        self.tnc_win = curses.newwin(tnc_height, width, 0, 0)
        self.tnc_win.border()
        self.tnc_win.addstr(0, 2, " TNC Output ")
        
        # Packet Log window (middle section)
        packet_height = height // 3
        packet_start = tnc_height
        self.packet_win = curses.newwin(packet_height, width, packet_start, 0)
        self.packet_win.border()
        self.packet_win.addstr(0, 2, " Packet Log ")
        
        # Input window (bottom)
        input_height = height - tnc_height - packet_height - 1
        input_start = tnc_height + packet_height
        self.input_win = curses.newwin(input_height, width, input_start, 0)
        self.input_win.border()
        self.input_win.addstr(0, 2, " Input ")
        
        # Status line (very bottom)
        self.status_win = curses.newwin(1, width, height - 1, 0)
        
        # Enable scrolling and keypad
        self.tnc_win.scrollok(True)
        self.packet_win.scrollok(True)
        self.input_win.keypad(True)
        
        # Initialize colors if available
        if curses.has_colors():
            curses.start_color()
            curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)   # Connected
            curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)     # Disconnected
            curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)  # Warnings
            curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)    # Packets
            curses.init_pair(5, curses.COLOR_MAGENTA, curses.COLOR_BLACK) # Commands
    
    def add_tnc_output(self, text):
        """Add text to TNC output buffer"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.tnc_output.append(f"[{timestamp}] {text}")
        # Auto-scroll to bottom if we're close to the bottom
        if self.tnc_scroll_offset < 5:
            self.tnc_scroll_offset = 0
    
    def add_packet_log(self, packet_type, data):
        """Add packet to packet log buffer"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.packet_log.append(f"[{timestamp}] {packet_type}: {data}")
    
    def wrap_text(self, text, width):
        """Wrap text to fit within specified width"""
        if len(text) <= width:
            return [text]
        
        wrapped_lines = []
        words = text.split(' ')
        current_line = ""
        
        for word in words:
            # If word itself is longer than width, split it
            if len(word) > width:
                if current_line:
                    wrapped_lines.append(current_line)
                    current_line = ""
                # Split long word across multiple lines
                while len(word) > width:
                    wrapped_lines.append(word[:width])
                    word = word[width:]
                if word:
                    current_line = word
            else:
                # Check if adding this word would exceed width
                test_line = current_line + (" " if current_line else "") + word
                if len(test_line) <= width:
                    current_line = test_line
                else:
                    # Start new line with this word
                    if current_line:
                        wrapped_lines.append(current_line)
                    current_line = word
        
        if current_line:
            wrapped_lines.append(current_line)
        
        return wrapped_lines if wrapped_lines else [""]

    def update_tnc_window(self):
        """Update the TNC output window"""
        if not self.tnc_win:
            return
        
        try:
            height, width = self.tnc_win.getmaxyx()
            self.tnc_win.clear()
            self.tnc_win.border()
            
            # Prepare wrapped lines for display
            display_lines = []
            for line in self.tnc_output:
                # Wrap long lines
                wrapped = self.wrap_text(line, width - 3)
                display_lines.extend(wrapped)
            
            # Display output with scroll support
            available_lines = height - 2
            total_lines = len(display_lines)
            
            # Calculate start line based on scroll offset
            if self.tnc_scroll_offset == 0:
                # Normal mode - show most recent (scroll to bottom)
                start_line = max(0, total_lines - available_lines)
            else:
                # Scrolling mode - offset from bottom
                start_line = max(0, total_lines - available_lines - self.tnc_scroll_offset)
            
            # Show scroll indicator in title if scrolled
            title = " TNC Output "
            if self.tnc_scroll_offset > 0:
                title = f" TNC Output (↑{self.tnc_scroll_offset}) "
            
            try:
                self.tnc_win.addstr(0, 2, title[:width-4])  # Ensure title fits
            except curses.error:
                pass
            
            for i, line in enumerate(display_lines[start_line:start_line + available_lines]):
                if i >= available_lines:
                    break
                try:
                    # Ensure line fits in window
                    display_line = line[:width-3] if len(line) > width-3 else line
                    self.tnc_win.addstr(i + 1, 1, display_line)
                except (curses.error, UnicodeEncodeError):
                    # Skip problematic lines
                    pass
            
            self.tnc_win.refresh()
        except Exception as e:
            # Log error but don't crash
            pass
    
    def update_packet_window(self):
        """Update the packet log window"""
        if not self.packet_win:
            return
        
        try:
            height, width = self.packet_win.getmaxyx()
            self.packet_win.clear()
            self.packet_win.border()
            
            try:
                self.packet_win.addstr(0, 2, " Packet Log ")
            except curses.error:
                pass
            
            # Prepare wrapped lines for display
            display_lines = []
            line_colors = []
            
            for line in self.packet_log:
                # Determine color for this line
                if "RX:" in line:
                    color = 4  # Cyan
                elif "TX:" in line:
                    color = 1  # Green
                else:
                    color = 0  # Default
                
                # Wrap long lines
                wrapped = self.wrap_text(line, width - 3)
                display_lines.extend(wrapped)
                # Apply same color to all wrapped portions
                line_colors.extend([color] * len(wrapped))
            
            # Display recent packets (scroll to bottom)
            available_lines = height - 2
            start_line = max(0, len(display_lines) - available_lines)
            
            for i, (line, color) in enumerate(zip(display_lines[start_line:], line_colors[start_line:])):
                if i >= available_lines:
                    break
                try:
                    # Ensure line fits in window
                    display_line = line[:width-3] if len(line) > width-3 else line
                    # Apply color coding
                    if curses.has_colors() and color > 0:
                        self.packet_win.addstr(i + 1, 1, display_line, curses.color_pair(color))
                    else:
                        self.packet_win.addstr(i + 1, 1, display_line)
                except (curses.error, UnicodeEncodeError):
                    pass
            
            self.packet_win.refresh()
        except Exception as e:
            # Log error but don't crash
            pass
    
    def update_input_window(self):
        """Update the input window"""
        if not self.input_win:
            return
            
        height, width = self.input_win.getmaxyx()
        self.input_win.clear()
        self.input_win.border()
        
        # Show current mode in window title
        mode_str = "CONVERSE" if self.in_converse_mode else "COMMAND"
        self.input_win.addstr(0, 2, f" Input [{mode_str}] ")
        
        # Show input buffer
        if len(self.input_buffer) > width - 4:
            display_buffer = "..." + self.input_buffer[-(width-7):]
        else:
            display_buffer = self.input_buffer
            
        try:
            self.input_win.addstr(1, 1, f"> {display_buffer}")
            # Position cursor at end of input
            cursor_pos = min(len(display_buffer) + 3, width - 2)
            self.input_win.move(1, cursor_pos)
        except curses.error:
            pass
        
        self.input_win.refresh()
    
    def update_status_window(self):
        """Update the status line"""
        if not self.status_win:
            return
            
        _, width = self.status_win.getmaxyx()
        self.status_win.clear()
        
        # Create status message
        conn_status = "CONNECTED" if self.connected else "DISCONNECTED"
        mode_status = "CONVERSE" if self.in_converse_mode else "COMMAND"
        scroll_status = f" ↑{self.tnc_scroll_offset}" if self.tnc_scroll_offset > 0 else ""
        status_msg = f" Port: {self.port} | {conn_status} | {mode_status}{scroll_status} | PgUp/PgDn=Scroll F1=Help F10=Quit "
        
        # Truncate if too long
        if len(status_msg) > width:
            status_msg = status_msg[:width-3] + "..."
        
        try:
            if curses.has_colors():
                color = curses.color_pair(1) if self.connected else curses.color_pair(2)
                self.status_win.addstr(0, 0, status_msg, color)
            else:
                self.status_win.addstr(0, 0, status_msg)
        except curses.error:
            pass
        
        self.status_win.refresh()
    
    def process_tnc_response(self, data):
        """Process incoming data from TNC"""
        try:
            # Handle both \n and \r\n line endings
            text = data.decode('utf-8', errors='ignore').replace('\r\n', '\n').replace('\r', '\n')
            lines = text.split('\n')
            
            for line in lines:
                # Keep leading/trailing spaces for formatting (like help text)
                line = line.rstrip()
                if not line and len(lines) == 1:  # Skip only single empty lines
                    continue
                    
                # Check for mode changes - be more flexible with detection
                line_upper = line.upper()
                if any(phrase in line_upper for phrase in ['CONNECTED TO', '*** CONNECTED', 'CONNECTION ESTABLISHED']):
                    self.in_converse_mode = True
                    self.add_tnc_output(f"MODE: Switched to CONVERSE mode")
                elif any(phrase in line_upper for phrase in ['*** DISCONNECTED', 'CONNECTION CLOSED', 'DISCONNECTED FROM']):
                    self.in_converse_mode = False
                    self.add_tnc_output(f"MODE: Switched to COMMAND mode")
                
                # Check if this looks like a packet - be more specific
                if any(keyword in line_upper for keyword in ['TX:', 'RX:', 'BEACON SENT', 'FRAME RECEIVED', 'CONNECTING TO', 'DISCONNECTING FROM']):
                    self.add_packet_log("PACKET", line)
                elif any(keyword in line_upper for keyword in ['CONNECTED TO', 'CONNECTION ESTABLISHED', 'CONNECTION CLOSED', 'DISCONNECTED FROM']):
                    self.add_packet_log("CONNECTION", line)
                    self.add_tnc_output(line)  # Also show in main output
                else:
                    self.add_tnc_output(line)
                    
        except Exception as e:
            # Log any processing errors
            self.add_tnc_output(f"ERROR processing TNC response: {e}")
            self.add_tnc_output(f"Raw data: {repr(data[:100])}")  # Show first 100 bytes
    
    def serial_reader_thread(self):
        """Thread for reading from serial port"""
        while self.running and self.connected:
            try:
                if self.serial_conn and self.serial_conn.is_open and self.serial_conn.in_waiting:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    if data:
                        self.process_tnc_response(data)
            except serial.SerialException as e:
                self.add_tnc_output(f"Serial connection error: {e}")
                self.connected = False
                break
            except Exception as e:
                self.add_tnc_output(f"Unexpected serial error: {e}")
                # Don't break on unexpected errors, just log them
                pass
            time.sleep(0.01)
    
    def send_to_tnc(self, data):
        """Send data to TNC"""
        if not self.connected or not self.serial_conn:
            self.add_tnc_output("ERROR: Not connected to TNC")
            return False
        
        try:
            # Add command to TNC output for reference
            if self.in_converse_mode:
                self.add_tnc_output(f"CHAT> {data}")
            else:
                self.add_tnc_output(f"CMD> {data}")
            
            # Send to TNC
            self.serial_conn.write((data + '\r\n').encode('utf-8'))
            return True
        except Exception as e:
            self.add_tnc_output(f"ERROR: Failed to send data: {e}")
            return False
    
    def show_help(self):
        """Show help information"""
        help_text = [
            "LoRa TNC Terminal Help",
            "======================",
            "",
            "Navigation Keys:",
            "  F1         - Show this help",
            "  F10        - Quit program",
            "  Ctrl+C     - Quit program",
            "  Up/Down    - Command history",
            "  Page Up    - Scroll TNC output backward",
            "  Page Down  - Scroll TNC output forward",
            "  Home       - Scroll to top of TNC output",
            "  End        - Scroll to bottom (live view)",
            "  Enter      - Send command/message",
            "",
            "TNC Commands (Command Mode):",
            "  MYCALL <call>  - Set your callsign",
            "  FREQ <freq>    - Set frequency (MHz)",
            "  POWER <dbm>    - Set TX power (-9 to 22 dBm)",
            "  PRESET <name>  - Apply preset configuration",
            "  CONNECT <call> - Connect to station",
            "  BEACON         - Send beacon",
            "  HELP           - Show TNC help",
            "",
            "Available Presets:",
            "  BALANCED, HIGH_SPEED, LONG_RANGE, LOW_POWER",
            "  AMATEUR_70CM, AMATEUR_33CM, AMATEUR_23CM",
            "",
            "Converse Mode:",
            "  Type messages normally",
            "  /DISC or QUIT  - Disconnect",
            "  /CMD           - Return to command mode",
            "",
            "Press any key to continue..."
        ]
        
        # Clear TNC window and show help
        height, width = self.tnc_win.getmaxyx()
        self.tnc_win.clear()
        self.tnc_win.border()
        self.tnc_win.addstr(0, 2, " Help ")
        
        for i, line in enumerate(help_text):
            if i >= height - 2:
                break
            try:
                self.tnc_win.addstr(i + 1, 1, line[:width-3])
            except curses.error:
                pass
        
        self.tnc_win.refresh()
        self.tnc_win.getch()  # Wait for keypress
    
    def run(self):
        """Main program loop"""
        def main_loop(stdscr):
            self.setup_windows(stdscr)
            
            # Connect to TNC
            if not self.connect_serial():
                self.add_tnc_output("Failed to connect to TNC. Press any key to exit.")
                self.update_tnc_window()
                stdscr.getch()
                return
            
            self.running = True
            
            # Start serial reader thread
            reader_thread = threading.Thread(target=self.serial_reader_thread)
            reader_thread.daemon = True
            reader_thread.start()
            
            self.add_tnc_output(f"Connected to TNC on {self.port}")
            self.add_tnc_output("Type 'HELP' for TNC commands, F1 for terminal help, F10 to quit")
            
            # Send initial command to get TNC prompt
            self.send_to_tnc("")
            
            try:
                while self.running:
                    try:
                        # Update all windows
                        self.update_tnc_window()
                        self.update_packet_window()
                        self.update_input_window()
                        self.update_status_window()
                        
                        # Get user input
                        try:
                            key = self.input_win.getch()
                        except KeyboardInterrupt:
                            break
                    except Exception as e:
                        # Log display errors but continue
                        self.add_tnc_output(f"Display error: {e}")
                        time.sleep(0.1)
                        continue
                    
                    if key == curses.KEY_F1:
                        self.show_help()
                    elif key == curses.KEY_F10 or key == 3:  # F10 or Ctrl+C
                        break
                    elif key == curses.KEY_UP:
                        # Command history - previous
                        if self.input_history and self.history_index > 0:
                            self.history_index -= 1
                            self.input_buffer = self.input_history[self.history_index]
                    elif key == curses.KEY_DOWN:
                        # Command history - next
                        if self.input_history and self.history_index < len(self.input_history) - 1:
                            self.history_index += 1
                            self.input_buffer = self.input_history[self.history_index]
                        else:
                            self.input_buffer = ""
                            self.history_index = len(self.input_history)
                    elif key == curses.KEY_PPAGE:  # Page Up - scroll TNC output up
                        self.tnc_scroll_offset = min(self.tnc_scroll_offset + 10, len(self.tnc_output))
                    elif key == curses.KEY_NPAGE:  # Page Down - scroll TNC output down
                        self.tnc_scroll_offset = max(0, self.tnc_scroll_offset - 10)
                    elif key == curses.KEY_HOME:  # Home - scroll to top
                        self.tnc_scroll_offset = len(self.tnc_output)
                    elif key == curses.KEY_END:   # End - scroll to bottom
                        self.tnc_scroll_offset = 0
                    elif key == 10 or key == 13:  # Enter
                        if self.input_buffer.strip():
                            # Add to history
                            self.input_history.append(self.input_buffer)
                            self.history_index = len(self.input_history)
                            
                            # Send to TNC
                            self.send_to_tnc(self.input_buffer)
                            self.input_buffer = ""
                    elif key == 127 or key == curses.KEY_BACKSPACE:  # Backspace
                        if self.input_buffer:
                            self.input_buffer = self.input_buffer[:-1]
                    elif key >= 32 and key < 127:  # Printable characters
                        self.input_buffer += chr(key)
                    
                    time.sleep(0.01)
                    
            except Exception as e:
                self.add_tnc_output(f"Error in main loop: {e}")
            finally:
                self.running = False
                self.disconnect_serial()
        
        # Run the curses application
        curses.wrapper(main_loop)

def main():
    parser = argparse.ArgumentParser(description='LoRa TNC Terminal Interface')
    parser.add_argument('port', help='Serial port (e.g., /dev/ttyACM0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, 
                       help='Baud rate (default: 115200)')
    
    args = parser.parse_args()
    
    print(f"Starting TNC Terminal for {args.port}...")
    print("Press Ctrl+C to exit setup if needed")
    
    try:
        terminal = TNCTerminal(args.port, args.baudrate)
        terminal.run()
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())