"""
6PACK Protocol Implementation Example

This demonstrates how 6PACK protocol could be implemented as an alternative
to KISS for more advanced TNC control with built-in error checking and
flow control.

6PACK provides:
- Multiple virtual channels
- Built-in CRC error checking
- Sequence numbers for flow control
- Priority levels
- Automatic retransmission
"""

import struct
import time
from enum import IntEnum
from dataclasses import dataclass
from typing import Optional, Callable, List
import threading
import queue

class SixPackCommands(IntEnum):
    """6PACK command definitions"""
    DATA = 0x00           # Data frame
    ACK = 0x01            # Acknowledgment
    NACK = 0x02           # Negative acknowledgment
    STATUS = 0x03         # Status request/response
    CONFIG = 0x04         # Configuration command
    RESET = 0x05          # Reset channel
    TEST = 0x06           # Test frame
    BEACON = 0x07         # Beacon frame

class Priority(IntEnum):
    """Priority levels for frames"""
    LOW = 0
    NORMAL = 1
    HIGH = 2
    URGENT = 3

@dataclass
class SixPackFrame:
    """6PACK frame structure"""
    sync: int = 0x00          # Sync byte (always 0x00)
    command: int = 0          # Command type
    channel: int = 0          # Virtual channel (0-15)
    priority: int = 0         # Priority level (0-3)
    sequence: int = 0         # Sequence number (0-255)
    data: bytes = b''         # Payload data
    crc: int = 0              # CRC-16 checksum
    
    @classmethod
    def create_data_frame(cls, channel: int, data: bytes, 
                         priority: Priority = Priority.NORMAL, 
                         sequence: int = 0) -> 'SixPackFrame':
        """Create a data frame"""
        frame = cls(
            sync=0x00,
            command=SixPackCommands.DATA,
            channel=channel,
            priority=priority,
            sequence=sequence,
            data=data
        )
        frame.crc = frame.calculate_crc()
        return frame
    
    @classmethod
    def create_ack_frame(cls, channel: int, sequence: int) -> 'SixPackFrame':
        """Create an ACK frame"""
        frame = cls(
            sync=0x00,
            command=SixPackCommands.ACK,
            channel=channel,
            sequence=sequence
        )
        frame.crc = frame.calculate_crc()
        return frame
    
    @classmethod
    def create_status_request(cls, channel: int = 0) -> 'SixPackFrame':
        """Create a status request frame"""
        frame = cls(
            sync=0x00,
            command=SixPackCommands.STATUS,
            channel=channel
        )
        frame.crc = frame.calculate_crc()
        return frame
    
    def calculate_crc(self) -> int:
        """Calculate CRC-16 for the frame (excluding CRC field)"""
        # Simple CRC-16-CCITT implementation
        data = self.pack_header() + self.data
        crc = 0xFFFF
        
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc <<= 1
                crc &= 0xFFFF
        
        return crc
    
    def pack_header(self) -> bytes:
        """Pack the frame header (without CRC)"""
        cmd_byte = (self.channel << 4) | (self.priority << 2) | (self.command & 0x03)
        return struct.pack('BBB', self.sync, cmd_byte, self.sequence)
    
    def pack(self) -> bytes:
        """Pack the complete frame for transmission"""
        self.crc = self.calculate_crc()  # Recalculate CRC
        header = self.pack_header()
        return header + self.data + struct.pack('>H', self.crc)
    
    @classmethod
    def unpack(cls, data: bytes) -> Optional['SixPackFrame']:
        """Unpack a frame from received data"""
        if len(data) < 5:  # Minimum frame size
            return None
        
        try:
            # Parse header
            sync, cmd_byte, sequence = struct.unpack('BBB', data[:3])
            
            if sync != 0x00:
                return None
            
            channel = (cmd_byte >> 4) & 0x0F
            priority = (cmd_byte >> 2) & 0x03
            command = cmd_byte & 0x03
            
            # Extract CRC
            crc = struct.unpack('>H', data[-2:])[0]
            
            # Extract payload
            payload = data[3:-2]
            
            frame = cls(
                sync=sync,
                command=command,
                channel=channel,
                priority=priority,
                sequence=sequence,
                data=payload,
                crc=crc
            )
            
            # Verify CRC
            calculated_crc = frame.calculate_crc()
            if calculated_crc != crc:
                return None  # CRC mismatch
            
            return frame
            
        except struct.error:
            return None

class SixPackTNC:
    """6PACK TNC implementation with flow control and error recovery"""
    
    def __init__(self, serial_port, max_channels: int = 16):
        self.serial_port = serial_port
        self.max_channels = max_channels
        
        # Channel state tracking
        self.tx_sequence = [0] * max_channels
        self.rx_sequence = [0] * max_channels
        self.pending_acks = {}  # seq_num -> (frame, timestamp, retries)
        
        # Callbacks
        self.on_data_received: Optional[Callable[[int, bytes], None]] = None
        self.on_status_response: Optional[Callable[[int, dict], None]] = None
        
        # Threading
        self.running = False
        self.rx_thread = None
        self.tx_queue = queue.PriorityQueue()
        self.ack_timeout = 5.0  # seconds
        self.max_retries = 3
        
        # Statistics
        self.stats = {
            'frames_tx': 0,
            'frames_rx': 0,
            'frames_ack': 0,
            'frames_nack': 0,
            'crc_errors': 0,
            'timeouts': 0,
            'retries': 0
        }
    
    def start(self):
        """Start the TNC processing"""
        self.running = True
        self.rx_thread = threading.Thread(target=self._rx_thread)
        self.rx_thread.daemon = True
        self.rx_thread.start()
    
    def stop(self):
        """Stop the TNC processing"""
        self.running = False
        if self.rx_thread:
            self.rx_thread.join(timeout=2)
    
    def send_data(self, channel: int, data: bytes, 
                  priority: Priority = Priority.NORMAL) -> bool:
        """Send data on a specific channel"""
        if channel >= self.max_channels:
            return False
        
        # Create frame with next sequence number
        seq = self.tx_sequence[channel]
        frame = SixPackFrame.create_data_frame(channel, data, priority, seq)
        
        # Queue for transmission
        priority_value = (3 - priority) * 1000 + seq  # Higher priority = lower value
        self.tx_queue.put((priority_value, frame))
        
        # Update sequence number
        self.tx_sequence[channel] = (seq + 1) % 256
        
        # Add to pending ACKs
        self.pending_acks[f"{channel}:{seq}"] = (frame, time.time(), 0)
        
        return True
    
    def send_status_request(self, channel: int = 0) -> bool:
        """Request status from TNC"""
        frame = SixPackFrame.create_status_request(channel)
        self.tx_queue.put((0, frame))  # High priority
        return True
    
    def _rx_thread(self):
        """Background thread for receiving frames"""
        buffer = bytearray()
        
        while self.running:
            try:
                if self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer.extend(data)
                    
                    # Process complete frames
                    while len(buffer) >= 5:  # Minimum frame size
                        # Look for sync byte
                        sync_pos = buffer.find(0x00)
                        if sync_pos == -1:
                            buffer.clear()
                            break
                        
                        if sync_pos > 0:
                            buffer = buffer[sync_pos:]
                        
                        # Try to parse frame
                        frame = self._extract_frame(buffer)
                        if frame:
                            self._process_received_frame(frame)
                        else:
                            # Not enough data or invalid frame
                            break
                
                # Process retransmissions
                self._check_timeouts()
                
                time.sleep(0.01)
                
            except Exception as e:
                if self.running:
                    print(f"RX thread error: {e}")
    
    def _extract_frame(self, buffer: bytearray) -> Optional[SixPackFrame]:
        """Try to extract a complete frame from buffer"""
        if len(buffer) < 5:
            return None
        
        # Frame format: SYNC CMD SEQ DATA... CRC16
        # We need to find the frame length somehow
        # For simplicity, assume frames are delimited by sync bytes
        
        next_sync = buffer.find(0x00, 1)
        if next_sync == -1:
            if len(buffer) > 256:  # Maximum reasonable frame size
                buffer.clear()
            return None
        
        frame_data = bytes(buffer[:next_sync])
        frame = SixPackFrame.unpack(frame_data)
        
        if frame:
            del buffer[:next_sync]  # Remove processed data
            return frame
        else:
            # Invalid frame, skip first byte
            del buffer[0]
            return None
    
    def _process_received_frame(self, frame: SixPackFrame):
        """Process a received frame"""
        self.stats['frames_rx'] += 1
        
        if frame.command == SixPackCommands.DATA:
            # Send ACK
            ack_frame = SixPackFrame.create_ack_frame(frame.channel, frame.sequence)
            self._send_frame_immediate(ack_frame)
            self.stats['frames_ack'] += 1
            
            # Check sequence number
            expected_seq = self.rx_sequence[frame.channel]
            if frame.sequence == expected_seq:
                # In-order frame
                self.rx_sequence[frame.channel] = (expected_seq + 1) % 256
                if self.on_data_received:
                    self.on_data_received(frame.channel, frame.data)
            else:
                # Out-of-order frame (duplicate or lost frames)
                print(f"Out-of-order frame on channel {frame.channel}: "
                      f"got {frame.sequence}, expected {expected_seq}")
        
        elif frame.command == SixPackCommands.ACK:
            # Remove from pending ACKs
            key = f"{frame.channel}:{frame.sequence}"
            if key in self.pending_acks:
                del self.pending_acks[key]
        
        elif frame.command == SixPackCommands.NACK:
            # Retransmit frame
            key = f"{frame.channel}:{frame.sequence}"
            if key in self.pending_acks:
                original_frame, _, retries = self.pending_acks[key]
                if retries < self.max_retries:
                    self.pending_acks[key] = (original_frame, time.time(), retries + 1)
                    self._send_frame_immediate(original_frame)
                    self.stats['retries'] += 1
                else:
                    del self.pending_acks[key]
                    print(f"Max retries exceeded for frame {frame.sequence} on channel {frame.channel}")
            self.stats['frames_nack'] += 1
        
        elif frame.command == SixPackCommands.STATUS:
            # Status response
            if self.on_status_response:
                status_data = self._parse_status_data(frame.data)
                self.on_status_response(frame.channel, status_data)
    
    def _send_frame_immediate(self, frame: SixPackFrame):
        """Send a frame immediately (bypassing queue)"""
        data = frame.pack()
        self.serial_port.write(data)
        self.serial_port.flush()
        self.stats['frames_tx'] += 1
    
    def _check_timeouts(self):
        """Check for ACK timeouts and retransmit"""
        current_time = time.time()
        timed_out = []
        
        for key, (frame, timestamp, retries) in self.pending_acks.items():
            if current_time - timestamp > self.ack_timeout:
                if retries < self.max_retries:
                    # Retransmit
                    self._send_frame_immediate(frame)
                    self.pending_acks[key] = (frame, current_time, retries + 1)
                    self.stats['retries'] += 1
                else:
                    # Give up
                    timed_out.append(key)
                    self.stats['timeouts'] += 1
        
        # Remove timed-out frames
        for key in timed_out:
            del self.pending_acks[key]
            print(f"Frame timeout: {key}")
    
    def _parse_status_data(self, data: bytes) -> dict:
        """Parse status response data"""
        # Example status format
        status = {
            'frequency': 915.0,
            'tx_power': 20,
            'packets_tx': 0,
            'packets_rx': 0,
            'crc_errors': 0
        }
        
        if len(data) >= 16:
            try:
                # Example: freq(4) power(1) tx_count(4) rx_count(4) errors(2) padding(1)
                freq, power, tx_count, rx_count, errors = struct.unpack('>fBIIH', data[:15])
                status.update({
                    'frequency': freq,
                    'tx_power': power,
                    'packets_tx': tx_count,
                    'packets_rx': rx_count,
                    'crc_errors': errors
                })
            except struct.error:
                pass
        
        return status
    
    def get_statistics(self) -> dict:
        """Get current statistics"""
        return self.stats.copy()

# Example usage
def example_usage():
    """Example of how to use the 6PACK TNC"""
    import serial
    
    # Connect to TNC
    ser = serial.Serial('/dev/ttyUSB0', 115200)
    tnc = SixPackTNC(ser)
    
    # Set up callbacks
    def on_data(channel: int, data: bytes):
        print(f"Received on channel {channel}: {data.decode('utf-8', errors='ignore')}")
    
    def on_status(channel: int, status: dict):
        print(f"Status for channel {channel}: {status}")
    
    tnc.on_data_received = on_data
    tnc.on_status_response = on_status
    
    # Start TNC
    tnc.start()
    
    try:
        # Send test message
        tnc.send_data(0, b"Hello, 6PACK World!", Priority.NORMAL)
        
        # Request status
        tnc.send_status_request(0)
        
        # Keep running
        while True:
            time.sleep(1)
            stats = tnc.get_statistics()
            print(f"TX: {stats['frames_tx']}, RX: {stats['frames_rx']}, "
                  f"Retries: {stats['retries']}, Timeouts: {stats['timeouts']}")
    
    except KeyboardInterrupt:
        print("Stopping...")
    
    finally:
        tnc.stop()
        ser.close()

if __name__ == '__main__':
    example_usage()