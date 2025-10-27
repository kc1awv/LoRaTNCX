# Backup Configuration Button Fixes

**Date:** October 27, 2025  
**Author:** GitHub Copilot  
**Issue:** Backup Configuration button not working and causing device crashes  
**Status:** ✅ RESOLVED

## Problem Summary

The "Backup Configuration" button in the web interface had multiple critical issues:

1. **No download functionality** - Button appeared to do nothing when clicked
2. **Device crashes** - Clicking the button caused a stack overflow crash with infinite recursion
3. **Code duplication** - Duplicate JavaScript functions with inconsistent message formats
4. **Server response mismatch** - Server sent wrong response type that client couldn't handle

## Root Cause Analysis

### 1. JavaScript Issues
- **Duplicate functions**: Two `backupConfiguration()` functions existed with different WebSocket message formats
- **Message format inconsistency**: First function used `{type: 'command', action: 'backup_config'}`, second used `{type: 'request', data: 'backup_config'}`
- **Server compatibility**: Only the second format was compatible with server-side message handling

### 2. Server Response Issues
- **Wrong response type**: Server sent `"config_backup"` but client expected `"backup_config"`
- **Placeholder data**: `handleBackupConfig()` sent dummy data instead of actual configuration

### 3. Critical Stack Overflow Bug
- **Circular JSON reference**: Both `createConfigurationBackup()` and `createConfigurationData()` used the same `JsonDocument`
- **Infinite recursion**: When assigning config data to payload, ArduinoJson detected circular reference and entered infinite recursion
- **Stack overflow**: Recursion depth exceeded ESP32 stack limits, causing device crash and reboot

## Stack Trace Analysis

The crash occurred in ArduinoJson's `JsonVariantCopier` with 100+ recursive calls:
```
ArduinoJson::V742PB22::detail::JsonVariantCopier::visit<ArduinoJson::V742PB22::JsonObjectConst>
-> ArduinoJson::V742PB22::JsonObject::set(ArduinoJson::V742PB22::JsonObjectConst)
-> [INFINITE RECURSION]
```

This pattern repeated until stack overflow occurred at frame #100.

## Solutions Implemented

### 1. JavaScript Cleanup (`data/app.js`)

**Removed duplicate function:**
```javascript
// REMOVED - First duplicate function around line 623
backupConfiguration() {
    this.sendWebSocketMessage({
        type: 'command',
        action: 'backup_config'  // Wrong format
    });
}
```

**Kept working function:**
```javascript
// KEPT - Correct function that matches server expectations
backupConfiguration() {
    this.sendWebSocketMessage({
        type: 'request',
        data: 'backup_config'    // Correct format
    });
}
```

### 2. Server Response Fix (`src/WebSocketServer.cpp`)

**Fixed response type in `handleBackupConfig()`:**
```cpp
// BEFORE
sendResponse(client, "config_backup", response);  // Wrong type

// AFTER  
sendResponse(client, "backup_config", payload);   // Correct type
```

**Used proper data generation:**
```cpp
// BEFORE
JsonObject response = doc.to<JsonObject>();
response["config_data"] = "backup_data_placeholder";  // Placeholder

// AFTER
JsonObject payload = createConfigurationBackup(doc);  // Real data
```

### 3. Critical Stack Overflow Fix (`src/WebSocketServer.cpp`)

**Root cause - Circular reference:**
```cpp
// BEFORE - CAUSED INFINITE RECURSION
JsonObject WebSocketServer::createConfigurationBackup(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();           // Uses doc
    JsonObject config = createConfigurationData(doc);    // Uses same doc!
    payload["configuration"] = config;                   // Circular reference!
    return payload;
}
```

**Fixed with separate documents:**
```cpp
// AFTER - SAFE IMPLEMENTATION
JsonObject WebSocketServer::createConfigurationBackup(JsonDocument& doc) {
    JsonObject payload = doc.to<JsonObject>();           // Uses doc
    JsonDocument configDoc;                              // Separate document
    JsonObject config = createConfigurationData(configDoc); // Uses configDoc
    payload["configuration"] = config;                   // Safe assignment
    return payload;
}
```

## Technical Details

### ArduinoJson Circular Reference Issue
When two JsonObjects from the same JsonDocument reference each other, ArduinoJson's copy mechanism detects this as a circular reference. The library attempts to resolve this by deep-copying the structure, but when the reference is truly circular, it results in infinite recursion.

### Memory Layout Problem
```
JsonDocument doc
├── payload (JsonObject)
│   ├── timestamp
│   ├── firmware_version  
│   └── configuration ──┐
└── config (JsonObject) ←┘  // Points back to same document!
```

### Solution: Separate Documents
```
JsonDocument doc              JsonDocument configDoc
├── payload (JsonObject)      ├── config (JsonObject)
│   ├── timestamp            │   ├── radio
│   ├── firmware_version     │   ├── aprs
│   └── configuration ───────┼──→│   ├── wifi
                              │   └── system
```

## Files Modified

1. **`data/app.js`**
   - Removed duplicate `backupConfiguration()` function (line ~623)
   - Kept working implementation that uses correct message format

2. **`src/WebSocketServer.cpp`**
   - Fixed `handleBackupConfig()` to use correct response type and real data
   - Fixed `createConfigurationBackup()` to use separate JsonDocument to prevent circular reference

## Testing Results

✅ **Backup button functionality**: Button now triggers download correctly  
✅ **Device stability**: No more crashes when clicking backup button  
✅ **File download**: Browser automatically downloads JSON configuration file  
✅ **Data integrity**: Configuration file contains all expected settings  
✅ **Format consistency**: Generated files follow expected JSON structure  

## Expected Behavior

1. User clicks "Backup Configuration" button
2. JavaScript sends: `{"type": "request", "data": "backup_config"}`
3. Server processes request with `handleBackupConfig()`
4. Server generates backup using `createConfigurationBackup()` with separate JsonDocument
5. Server responds: `{"type": "backup_config", "payload": {...}}`
6. Client processes response with `handleConfigurationBackup()`
7. JavaScript creates downloadable file: `loratncx_config_YYYY-MM-DD.json`
8. Browser automatically downloads the file
9. Success message displayed: "Configuration downloaded successfully"

## Configuration File Format

The generated backup file contains:
```json
{
  "timestamp": 1234567890,
  "firmware_version": "1.0.0",
  "device_id": "80:F1:B2:A7:C0:A8",
  "backup_format_version": "1.0",
  "configuration": {
    "radio": {
      "frequency": 915000000,
      "power": 20,
      "bandwidth": 125000,
      "spreading_factor": 12,
      "coding_rate": 5
    },
    "aprs": {
      "callsign": "NOCALL",
      "ssid": 0,
      "beacon_interval": 300,
      "beacon_text": "LoRaTNCX",
      "auto_beacon": false
    },
    "wifi": {
      "ssid": "LoRaTNCX",
      "ap_mode": true
    },
    "system": {
      "oled_enabled": true,
      "gnss_enabled": true,
      "timezone": "UTC"
    }
  }
}
```

## Lessons Learned

1. **Always use separate JsonDocuments** when creating nested JSON structures to avoid circular references
2. **ArduinoJson circular reference detection** can cause stack overflow in recursive copy operations
3. **Message format consistency** is critical between client and server WebSocket communication
4. **Duplicate code** can lead to inconsistent behavior and debugging difficulties
5. **Stack overflow debugging** requires careful analysis of recursive patterns in embedded systems

## Prevention Measures

1. **Code review**: Always check for duplicate functions during development
2. **Message format documentation**: Maintain clear documentation of WebSocket message formats
3. **JsonDocument separation**: Use separate documents for nested JSON structures
4. **Testing**: Test all interactive features thoroughly, especially those involving complex data structures
5. **Error handling**: Implement proper error handling for WebSocket communication failures

## Related Issues

- Initial web interface implementation
- WebSocket message handling inconsistencies  
- ArduinoJson memory management in embedded systems
- ESP32 stack size limitations

---

**Note**: This fix resolves a critical stability issue that could cause device crashes. The backup functionality is now safe and reliable for production use.