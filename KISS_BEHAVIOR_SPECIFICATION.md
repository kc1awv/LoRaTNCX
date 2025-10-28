# KISS Behavior Specification - TAPR TNC-2 Compatibility

## Overview

The LoRaTNCX KISS implementation has been corrected to exactly match the authentic TAPR TNC-2 behavior as specified in the original documentation. This ensures compatibility with existing software that expects authentic TNC-2 behavior.

## Corrected KISS Command Behavior

### KISS ON Command

**Authentic TNC-2 Behavior:**
- Sets an internal flag indicating KISS mode should be activated on restart
- Does NOT immediately enter KISS mode
- Responds with confirmation but remains in command mode
- The TNC continues accepting commands normally

**Implementation:**
```cpp
void TNCCommandParser::handleKiss() {
    kissOnFlag = true;  // Set flag for restart
    sendResponse("KISS mode will be enabled on restart");
    // Mode remains TNC_MODE_COMMAND
}
```

### RESTART Command

**Authentic TNC-2 Behavior:**
- Checks if KISS ON flag is set
- If flag is set: Enters KISS mode and flashes status LED 3 times
- If flag is NOT set: Remains in command mode
- This is the only way to activate KISS mode after KISS ON

**Implementation:**
```cpp
void TNCCommandParser::handleRestart() {
    if (kissOnFlag) {
        sendResponse("Restarting TNC into KISS mode...");
        setMode(TNC_MODE_KISS);  // LED flashing handled by TNCManager
    } else {
        sendResponse("Restarting TNC into command mode...");
        // Standard restart behavior
    }
}
```

### KISSM Command (Immediate KISS)

**Authentic TNC-2 Behavior:**
- Immediately enters KISS mode without setting flag
- Does NOT require restart
- Bypasses the normal KISS ON + RESTART sequence
- Used for immediate KISS activation

**Implementation:**
```cpp
void TNCCommandParser::handleKissImmediate() {
    sendResponse("Entering KISS mode immediately...");
    setMode(TNC_MODE_KISS);
}
```

### LED Flashing Behavior

**Authentic TNC-2 Behavior:**
- Status LED flashes 3 times when entering KISS mode via RESTART
- Only occurs for KISS ON + RESTART sequence
- Does NOT occur for KISSM immediate entry
- Visual indication that KISS mode is now active

**Implementation:**
```cpp
void TNCManager::flashKissLEDs() {
    // Flash status LED 3 times (Heltec V4 has single LED on GPIO 35)
    for (int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, LED_ON);
        delay(200);
        digitalWrite(STATUS_LED_PIN, LED_OFF);
        delay(200);
    }
}
```

## Command Sequence Examples

### Standard KISS Activation
```
cmd: KISS ON
TNC: KISS mode will be enabled on restart

cmd: RESTART  
TNC: Restarting TNC into KISS mode...
TNC: Status LED will flash three times
[Status LED flashes 3 times]
[TNC enters KISS mode - no more text responses]
```

### Immediate KISS Activation
```
cmd: KISSM
TNC: Entering KISS mode immediately...
[TNC enters KISS mode - no more text responses, no status LED flashing]
```

### Restart Without KISS Flag
```
cmd: RESTART
TNC: Restarting TNC into command mode...
cmd:
```

## Technical Implementation Details

### Flag Management
- `kissOnFlag` boolean tracks KISS ON state
- Flag persists until restart is executed
- Flag is cleared when KISS mode is actually entered
- Flag can be queried via `getKissFlag()` method

### Mode Coordination
- TNCCommandParser manages the flag and commands
- TNCManager handles actual mode switching and LED control
- Callbacks coordinate between parser and manager
- Proper separation of concerns maintained

### KISS Exit Behavior
- Exiting KISS mode (via KISS frame or timeout) clears the flag
- Returns to command mode with standard prompt
- No LED flashing on exit

## Compatibility Notes

This implementation now exactly matches the behavior documented in the TAPR TNC-2 manual and exhibited by authentic TNC-2 hardware. Software written for TNC-2 compatibility should work without modification.

Key differences from generic KISS implementations:
1. KISS ON does not immediately enter KISS mode
2. RESTART is required to actually activate KISS mode
3. LED flashing provides visual confirmation
4. KISSM provides immediate entry for special cases

## Testing Verification

The corrected behavior can be verified by:
1. Sending `KISS ON` - should respond but stay in command mode
2. Sending other commands - should work normally
3. Sending `RESTART` - should enter KISS mode with LED flash
4. Testing `KISSM` - should enter immediately without LED flash
5. Testing `RESTART` without prior `KISS ON` - should stay in command mode