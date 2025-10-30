# LoRa TNC KISS Protocol Implementation - Complete

## Overview
This TNC now features complete KISS protocol implementation with full Xastir compatibility for amateur radio packet operations.

## KISS Protocol Features Implemented

### 1. KISS Mode Entry
- **ESC@k Sequence**: `0x1B 0x40 0x6B 0x0D` - Direct KISS mode entry (Xastir compatible)
- **Auto-detection**: Automatic KISS mode when receiving `0xC0` (KISS frame start)

### 2. KISS Commands Supported
- `0x00` - Data Frame (transmit packet)
- `0x01` - TX Delay (set transmit delay in 10ms units)
- `0x02` - Persistence (set persistence value 0-255)
- `0x03` - Slot Time (set slot time in 10ms units)
- `0x04` - TX Tail (set TX tail in 10ms units)  
- `0x05` - Full Duplex (0=half-duplex, 1=full-duplex)
- `0x06` - Set Hardware (reserved/not implemented)
- `0xFF` - Return to Command Mode (CMD_RETURN)

### 3. KISS Mode Exit
- **ESC Character**: `0x1B` - TAPR TNC2 standard exit (universal TNC escape sequence)
- **CMD_RETURN**: `0xFF` - KISS protocol standard exit

#### ESC Character (0x1B) Usage in TNCs
The ESC character is a fundamental control character in TNC operations:

**Common TNC ESC Functions:**
- **Exit Converse Mode**: Returns from transparent keyboard-to-keyboard mode to command mode
- **Exit KISS Mode**: Returns from KISS protocol mode to command mode  
- **Command Prefix**: Can signal the start of complex command sequences
- **In-band Signaling**: Special control character within the data stream
- **Mode Breaking**: Interrupts current operation to return to command state

**Implementation Notes:**
- ESC provides universal "break" functionality across TNC modes
- Works as in-band signaling without requiring out-of-band control
- Compatible with legacy TNC behavior and modern packet software
- Essential for emergency return to command mode when other methods fail

### 4. Frame Processing
- Proper KISS frame assembly with `0xC0` start/end markers
- Byte stuffing/unstuffing (FESC/TFESC/TFEND handling)
- Automatic frame validation and parsing

## Amateur Radio Preset Configurations

### Available Presets (PRESET command)
1. **HIGH_SPEED** - Maximum data rate for local networks
2. **BALANCED** - Good balance of speed and range
3. **LONG_RANGE** - Maximum range for distant stations
4. **LOW_POWER** - Battery conservation mode
5. **AMATEUR_70CM** - Optimized for 70cm amateur band (420-450 MHz)
6. **AMATEUR_33CM** - Optimized for 33cm amateur band (902-928 MHz)
7. **AMATEUR_23CM** - Optimized for 23cm amateur band (1240-1300 MHz)
8. **FAST_BALANCED** - Fast data with good range
9. **ROBUST_BALANCED** - Robust communication with balanced settings
10. **MAX_RANGE** - Maximum possible range configuration

## Xastir Compatibility Testing

### Tested Operations
✅ **Initialization**: ESC@k sequence properly enters KISS mode  
✅ **Configuration**: All KISS parameter commands work correctly  
✅ **Packet Transmission**: APRS packets transmit successfully  
✅ **ESC Exit**: ESC character returns to command mode  
✅ **CMD_RETURN Exit**: 0xFF command returns to command mode  
✅ **Mode Switching**: Seamless transitions between KISS and command modes  

### Test Results
```
=== XASTIR COMPATIBILITY TEST COMPLETE ===
✓ All major Xastir operations tested successfully!
✓ TNC is fully compatible with Xastir packet radio software
```

## TNC Operating Modes and ESC Behavior

### Mode Overview
This TNC implements standard TNC operating modes with proper ESC handling:

1. **Command Mode** (Default)
   - Interactive command processing
   - ESC has no special function (processed as regular input)
   - All TNC configuration commands available

2. **KISS Mode** 
   - Binary packet protocol for computer control
   - ESC (0x1B) exits to Command Mode (TAPR TNC2 standard)
   - Transparent packet data transmission
   - Used by packet radio software (Xastir, APRSIS32, etc.)

3. **Converse Mode** (Future Enhancement)
   - Direct keyboard-to-keyboard communication
   - ESC would exit to Command Mode
   - Transparent data forwarding to connected station

### ESC Character Behavior by Mode

| Mode     | ESC (0x1B) Action    | Purpose                      |
| -------- | -------------------- | ---------------------------- |
| Command  | No special action    | Normal character input       |
| KISS     | Exit to Command Mode | Emergency return to control  |
| Converse | Exit to Command Mode | Break transparent connection |

## Technical Implementation

### State Machine
- Proper ESC@k sequence detection without false command processing
- Timeout handling for incomplete sequences
- Clean state transitions with buffer management
- Universal ESC handling across all modes

### KISS Protocol Stack
- **KISSProtocol.cpp**: Core KISS frame processing
- **TNCManager.cpp**: Mode switching and initialization handling
- **TNCCommandsSimple.cpp**: Enhanced command system with amateur radio presets

### Serial Communication
- 9600 baud default (configurable)
- Proper buffer management
- Binary frame handling for KISS protocol

## Usage with Amateur Radio Software

### Xastir Configuration
1. Set port to TNC serial device (e.g., /dev/ttyACM0)
2. Set baud rate to 9600
3. Select "KISS TNC" mode
4. Xastir will automatically send ESC@k to initialize

### Other Compatible Software
- APRSIS32
- YAAC (Yet Another APRS Client)
- Dire Wolf (as KISS interface)
- WinLink Express
- Any software supporting KISS protocol

## Configuration Examples

### Quick Setup
```
PRESET AMATEUR_70CM    # Configure for 70cm band
KISS                   # Enter KISS mode manually
```

### Advanced Configuration
```
PRESET BALANCED        # Start with balanced settings
FREQ 433.500          # Set frequency
POWER 14              # Set power level
KISS                   # Enter KISS mode
```

## Command Line Tools

### Terminal Interface
```bash
python3 tools/tnc_terminal.py
```
Professional terminal interface with:
- Text wrapping for long outputs
- Scrolling with PgUp/PgDn
- Three-pane layout (TNC output, packet log, input)
- Command history

### Testing Tools
```bash
python3 tools/test_xastir_full_workflow.py  # Complete Xastir test
python3 tools/test_esc_at_k_entry.py       # ESC@k sequence test
python3 tools/test_kiss.py                 # KISS protocol test
```

## Protocol Compliance

### TAPR TNC2 Compatibility
- ESC character exit from KISS mode
- Standard KISS command set
- Proper frame formatting

### Amateur Radio Standards
- Frequency allocations per band
- Power level compliance
- Modulation parameters optimized for amateur use

## Troubleshooting

### Common Issues and Solutions

#### TNC Stuck in KISS Mode
**Problem**: TNC not responding to commands, stuck in KISS mode  
**Solution**: Send ESC character (0x1B) to return to command mode
```bash
echo -ne '\x1B' > /dev/ttyACM0
```

#### Software Can't Initialize KISS Mode
**Problem**: Packet software fails to initialize TNC  
**Solutions**:
1. Verify ESC@k sequence support: `echo -ne '\x1B@k\r' > /dev/ttyACM0`
2. Try manual KISS mode entry: `KISS` command
3. Check baud rate (default: 9600)

#### ESC Character Not Working
**Problem**: ESC doesn't exit KISS mode  
**Diagnosis**:
- Verify character is 0x1B (decimal 27)
- Check terminal program isn't interpreting ESC locally
- Try CMD_RETURN instead: `echo -ne '\xC0\xFF\xC0' > /dev/ttyACM0`

#### Mode Confusion
**Problem**: Uncertain which mode TNC is in  
**Solution**: 
1. Send ESC (0x1B) to ensure command mode
2. Send `ID` command to verify response
3. Look for command prompt indicators

### Emergency Recovery
If TNC becomes unresponsive:
1. **Hardware Reset**: Power cycle the device
2. **ESC Sequence**: Send multiple ESC characters
3. **KISS Exit**: Send CMD_RETURN frame (0xC0 0xFF 0xC0)
4. **Terminal Reset**: Close/reopen serial connection

## Future Enhancements

### Potential Additions
- **Converse Mode**: Direct keyboard-to-keyboard communication
- **KISS parameter persistence**: Settings saved across reboots
- **Multiple KISS ports**: Support for multiple simultaneous connections
- **Advanced error correction**: Enhanced packet reliability
- **Signal strength reporting**: RSSI feedback to applications
- **Automatic frequency coordination**: Dynamic frequency management

### Enhanced ESC Handling
- **ESC Sequence Commands**: Complex commands starting with ESC
- **Transparent Mode**: Binary data passing with ESC break sequences
- **Quote Character Support**: Control-V style literal character entry

---

*This TNC implementation provides professional-grade KISS protocol support with proper ESC character handling, suitable for amateur radio packet operations, emergency communications, and experimental digital modes.*