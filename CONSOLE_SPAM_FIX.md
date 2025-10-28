# Console Spam Fix - RX Timeout Messages

## Issue
The LoRaTNCX console was being flooded with "RX timeout" debug messages every time the LoRa radio checked for incoming packets and found none (which is normal when there's no radio activity).

## Root Cause
In `src/LoRaRadio.cpp`, line 196 contained:
```cpp
} else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Timeout - not necessarily an error
    Serial.println("RX timeout");  // <-- This was spamming the console
} else {
```

## Solution
Commented out the debug message since RX timeouts are completely normal when no radio traffic is present:

```cpp
} else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Timeout - not necessarily an error (normal when no radio activity)
    // Serial.println("RX timeout");  // Commented out to reduce console spam
} else {
```

## Result
✅ **Clean Console**: No more spam messages  
✅ **Professional Output**: Only meaningful messages displayed  
✅ **Functionality Preserved**: Radio still works normally, just without debug noise  
✅ **User Experience**: Much better console readability  

## Boot Sequence Now Shows
```
TNC Manager initialized successfully

=== LoRaTNCX v2.0 - TAPR TNC-2 Compatible ===
Hardware: Heltec WiFi LoRa 32 V4 (ESP32-S3)
LoRa Chip: SX1262, RadioLib: v7.4.0
Type HELP for command list

cmd:
```

Clean, professional, and ready for user interaction!