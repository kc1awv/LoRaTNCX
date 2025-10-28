# LoRaTNCX Chat App Receive Debugging Guide

## Current Issue
The chat app can send messages (TNC shows successful transmission), but received messages aren't appearing in the chat interface.

## Debugging Tools Created

### 1. Debug Chat App (`debug_chat.py`)
Enhanced version with extensive logging that shows:
- All raw bytes received from serial port
- KISS frame parsing details
- Message queue processing
- Statistics counters

### 2. Serial Monitor (`serial_monitor.py`)
Simple tool that just shows raw serial data:
```bash
python chat_app/serial_monitor.py [/dev/ttyUSB0]
```

## Systematic Debugging Steps

### Step 1: Verify TNC is Transmitting
When you send a message, you should see in the TNC's serial output:
```
[RADIO] Attempting TX: XX bytes
[RADIO] Data: [hex dump]
[RADIO] TX SUCCESS: XX bytes transmitted
```

### Step 2: Verify TNC is Receiving
When another station transmits, you should see:
```
[RADIO] RX: XX bytes, RSSI: -XX dBm, SNR: X.X dB
```

### Step 3: Check if KISS Frames are Being Sent
Run the serial monitor while having another station transmit:
```bash
python chat_app/serial_monitor.py /dev/ttyUSB0
```

You should see KISS frames starting/ending with `c0` bytes when packets are received.

### Step 4: Run Debug Chat App
```bash
python chat_app/debug_chat.py
```

This will show detailed parsing of every byte and frame received.

## Expected KISS Frame Format for Received Data

When the TNC receives a packet from the radio, it should send a KISS frame like:
```
C0 00 [packet_data] C0
```

Where:
- `C0` = FEND (Frame End marker)
- `00` = DATA_FRAME command for port 0
- `[packet_data]` = The actual received packet (may be byte-stuffed)
- `C0` = FEND (Frame End marker)

## Common Issues and Solutions

### Issue 1: No Serial Data at All
**Symptoms**: Serial monitor shows no data when packets should be received
**Causes**: 
- Wrong serial port
- TNC not actually receiving packets
- Radio configuration mismatch between stations

**Solutions**:
- Verify serial port with `ls /dev/tty*`
- Check TNC display for radio activity
- Verify frequency/settings match other station

### Issue 2: Serial Data but No KISS Frames
**Symptoms**: Serial monitor shows data but no `C0` bytes
**Causes**:
- TNC receiving data but not radio packets
- Firmware issue with KISS frame generation

**Solutions**:
- Check if data looks like debug output vs KISS frames
- Verify firmware is properly loaded

### Issue 3: KISS Frames Present but Chat App Not Parsing
**Symptoms**: Serial monitor shows KISS frames, but debug chat shows parsing errors
**Causes**:
- Bug in Python KISS parsing
- Byte stuffing issues
- Frame format problems

**Solutions**:
- Check debug chat app output for specific parsing errors
- Compare expected vs actual frame format

### Issue 4: Frames Parsed but Messages Not Displayed
**Symptoms**: Debug chat shows successful parsing but no chat messages appear
**Causes**:
- Message format doesn't match expected `CALL>TO:MSG` format
- Threading/queue issues

**Solutions**:
- Check debug output for raw packet content
- Verify message format matches protocol

## Test Scenarios

### Test 1: Loopback Test
Send a message from the same chat app and see if you receive your own transmission (if radio supports this).

### Test 2: Two Station Test
Use two LoRaTNCX devices:
1. Station A sends test message using chat app
2. Station B should see it in their chat app
3. Check both stations' debug output

### Test 3: Manual KISS Frame Test
Create a simple script that sends a known KISS frame to the TNC and monitors for the transmission.

## Quick Fixes to Try

### Fix 1: Frame Buffer Issue
The current frame parsing might have a bug. Try this in `debug_chat.py`:

```python
# In receive_thread(), after "if byte == KISSFrame.FEND:"
if in_frame and len(frame_buffer) > 0:
    print(f"[DEBUG] Processing complete frame: {frame_buffer.hex()}")
    # ... rest of processing
```

### Fix 2: Threading Timing
Add more debugging to the queue processing:

```python
def process_received_messages(self):
    queue_size = self.receive_queue.qsize()
    if queue_size > 0:
        print(f"[DEBUG] Processing {queue_size} messages from queue")
    # ... rest of method
```

### Fix 3: Message Format Tolerance
Make the message parser more forgiving:

```python
@classmethod
def from_packet(cls, packet_data):
    try:
        packet_str = packet_data.decode('utf-8', errors='ignore').strip()
        print(f"[DEBUG] Raw packet string: '{packet_str}'")
        
        # Try different parsing approaches
        if ':' in packet_str:
            if '>' in packet_str:
                # Standard format: CALL>TO:MSG
                header_part = packet_str.split(':', 1)[0]
                msg_part = packet_str.split(':', 1)[1]
                if '>' in header_part:
                    from_call = header_part.split('>', 1)[0]
                    to_call = header_part.split('>', 1)[1]
                    return cls(from_call, to_call, msg_part)
            else:
                # Fallback: treat as simple message
                return cls("UNKNOWN", "ALL", packet_str)
        else:
            # No colon, treat entire packet as message
            return cls("UNKNOWN", "ALL", packet_str)
            
    except Exception as e:
        print(f"[DEBUG] Parse error: {e}")
    return None
```

## Next Steps

1. Run `debug_chat.py` and send a test message from another station
2. Check the debug output to see exactly where the process is failing
3. Compare with expected KISS frame format
4. If frames are being received but not parsed correctly, the issue is in the Python code
5. If no frames are being received, the issue is in the TNC firmware or radio configuration

The debug output will tell us exactly what's happening and where the issue lies.