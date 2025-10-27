# WebSocket API Reference

**LoRaTNCX WebSocket Communication Protocol**  
**Version:** 1.0  
**Date:** October 27, 2025

## Connection Details

- **Protocol**: WebSocket (RFC 6455)
- **Port**: 80 (HTTP upgrade to WebSocket)
- **Path**: `/ws`
- **URL**: `ws://[device-ip]/ws`

## Message Format

All WebSocket messages use JSON format:

```json
{
  "type": "message_type",
  "data": { /* message-specific data */ },
  "timestamp": 1635123456789
}
```

## Outbound Messages (Device → Web Client)

### 1. System Status
**Type**: `system_status`  
**Frequency**: Every 5 seconds  
**Purpose**: Real-time system health monitoring

```json
{
  "type": "system_status",
  "data": {
    "uptime": 300,              // Seconds since boot
    "freeHeap": 247400,         // Available heap memory (bytes)
    "voltage": 3.95,            // Battery voltage (V)
    "isCharging": false,        // Charging status
    "wifiConnected": true,      // WiFi connection status
    "apMode": false,            // Access Point mode active
    "temperature": 25.3,        // Device temperature (°C)
    "cpuFreq": 240              // CPU frequency (MHz)
  },
  "timestamp": 1635123456789
}
```

### 2. Radio Status
**Type**: `radio_status`  
**Frequency**: On change or request  
**Purpose**: Radio transmission statistics

```json
{
  "type": "radio_status", 
  "data": {
    "frequency": 915000000,     // Frequency (Hz)
    "power": 17,                // TX power (dBm)
    "bandwidth": 125000,        // Bandwidth (Hz)
    "spreadingFactor": 9,       // LoRa SF
    "codingRate": 5,            // LoRa CR
    "preambleLength": 8,        // Preamble symbols
    "syncWord": 0x12,           // Sync word
    "txCount": 42,              // Transmitted packets
    "rxCount": 38,              // Received packets
    "lastRSSI": -85,            // Last RSSI (dBm)
    "lastSNR": 8.5,             // Last SNR (dB)
    "airtime": 1234,            // Total airtime (ms)
    "dutyCycle": 2.5            // Duty cycle (%)
  },
  "timestamp": 1635123456789
}
```

### 3. GNSS Status
**Type**: `gnss_status`  
**Frequency**: On position update  
**Purpose**: GPS location and satellite data

```json
{
  "type": "gnss_status",
  "data": {
    "latitude": 42.123456,      // Latitude (decimal degrees)
    "longitude": -71.654321,    // Longitude (decimal degrees)
    "altitude": 123.45,         // Altitude (meters MSL)
    "speed": 5.2,               // Speed (km/h)
    "course": 287.5,            // Course (degrees true)
    "satellites": 8,            // Satellites in use
    "hdop": 1.2,                // Horizontal DOP
    "fix": "3D",                // Fix type: "None", "2D", "3D"
    "fixQuality": 1,            // GPS fix quality (0-9)
    "age": 0.5,                 // Age of fix (seconds)
    "timestamp": "2025-10-27T14:30:00Z"
  },
  "timestamp": 1635123456789
}
```

### 4. APRS Messages
**Type**: `aprs_message`  
**Frequency**: On message reception  
**Purpose**: Received APRS packets

```json
{
  "type": "aprs_message",
  "data": {
    "from": "KC1AWV-9",         // Source callsign
    "to": "CQ",                 // Destination
    "message": "Hello World",   // Message text
    "path": "WIDE1-1,WIDE2-1",  // Digipeater path
    "latitude": 42.123456,      // Position (if present)
    "longitude": -71.654321,
    "symbol": "/-",             // APRS symbol
    "rssi": -85,                // Reception RSSI
    "snr": 8.5,                 // Reception SNR
    "raw": "KC1AWV-9>CQ:Hello World"  // Raw packet
  },
  "timestamp": 1635123456789
}
```

### 5. Configuration Response
**Type**: `config_response`  
**Frequency**: In response to config requests  
**Purpose**: Current device configuration

```json
{
  "type": "config_response",
  "data": {
    "radio": {
      "frequency": 915000000,
      "power": 17,
      "bandwidth": 125000,
      "spreadingFactor": 9,
      "codingRate": 5,
      "preambleLength": 8,
      "syncWord": 0x12
    },
    "aprs": {
      "callsign": "KC1AWV",
      "ssid": 9,
      "symbol": "/-",
      "beaconInterval": 300,
      "comment": "LoRaTNCX Station"
    },
    "system": {
      "displayTimeout": 30,
      "backlightBrightness": 128,
      "serialBaud": 115200,
      "kissMode": true
    },
    "network": {
      "wifiSSID": "MyNetwork",
      "wifiPassword": "[HIDDEN]",
      "apMode": false,
      "apSSID": "KISS-TNC",
      "tcpPort": 8001,
      "nmeaPort": 10110
    }
  },
  "timestamp": 1635123456789
}
```

### 6. Error Messages
**Type**: `error`  
**Frequency**: On error conditions  
**Purpose**: Error reporting and debugging

```json
{
  "type": "error",
  "data": {
    "code": "RADIO_TX_FAIL",     // Error code
    "message": "Transmission failed", // Human readable
    "details": "SPI timeout",    // Technical details
    "severity": "warning",       // "info", "warning", "error", "critical"
    "component": "radio",        // Affected component
    "action": "retry"            // Suggested action
  },
  "timestamp": 1635123456789
}
```

## Inbound Messages (Web Client → Device)

### 1. Configuration Updates
**Type**: `config_update`  
**Purpose**: Update device configuration

```json
{
  "type": "config_update",
  "data": {
    "section": "radio",          // "radio", "aprs", "system", "network"
    "parameters": {
      "frequency": 915000000,
      "power": 20,
      "bandwidth": 125000
    }
  }
}
```

### 2. APRS Message Send
**Type**: `aprs_send`  
**Purpose**: Send APRS message or beacon

```json
{
  "type": "aprs_send",
  "data": {
    "to": "CQ",                  // Destination callsign
    "message": "Hello from web", // Message text
    "type": "message",           // "message", "beacon", "position"
    "ack": true                  // Request acknowledgment
  }
}
```

### 3. System Commands
**Type**: `system_command`  
**Purpose**: Execute system-level commands

```json
{
  "type": "system_command",
  "data": {
    "command": "restart",        // "restart", "reset_config", "calibrate"
    "confirm": true              // Confirmation flag
  }
}
```

### 4. Data Requests
**Type**: `request`  
**Purpose**: Request specific data from device

```json
{
  "type": "request",
  "data": {
    "what": "config",            // "config", "status", "statistics"
    "section": "all"             // Specific section or "all"
  }
}
```

### 5. Radio Control
**Type**: `radio_control`  
**Purpose**: Direct radio operations

```json
{
  "type": "radio_control",
  "data": {
    "action": "transmit",        // "transmit", "receive", "standby"
    "data": "Test transmission", // Data to transmit (if applicable)
    "frequency": 915000000       // Override frequency (optional)
  }
}
```

## Response Codes and Status

### Success Responses
```json
{
  "type": "ack",
  "data": {
    "requestId": "12345",        // Original request ID
    "status": "success",
    "message": "Configuration updated"
  }
}
```

### Error Responses  
```json
{
  "type": "nak",
  "data": {
    "requestId": "12345",
    "status": "error", 
    "code": "INVALID_FREQUENCY",
    "message": "Frequency out of range"
  }
}
```

## Connection Management

### Connection Establishment
1. Client initiates WebSocket handshake to `/ws`
2. Server accepts connection and sends initial status
3. Server begins periodic status updates

### Keep-Alive
- Server sends `ping` frames every 30 seconds
- Client should respond with `pong` frames
- Connection timeout after 60 seconds without response

### Graceful Disconnection
- Client sends WebSocket close frame
- Server acknowledges and closes connection
- Server stops sending updates for that client

## Implementation Examples

### JavaScript Client Connection
```javascript
const ws = new WebSocket('ws://192.168.4.1/ws');

ws.onopen = function() {
    console.log('WebSocket connected');
    // Request initial configuration
    ws.send(JSON.stringify({
        type: 'request',
        data: { what: 'config', section: 'all' }
    }));
};

ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    handleMessage(message);
};

function sendAPRS(to, message) {
    ws.send(JSON.stringify({
        type: 'aprs_send',
        data: { to: to, message: message, type: 'message', ack: true }
    }));
}
```

### ESP32 Server Implementation
```cpp
void WebSocketServer::onMessage(AsyncWebSocketClient* client, 
                               AwsEventType type, void* arg, 
                               uint8_t* data, size_t len) {
    if (type == WS_EVT_DATA) {
        JsonDocument doc;
        deserializeJson(doc, (char*)data);
        
        String msgType = doc["type"];
        if (msgType == "config_update") {
            handleConfigUpdate(client, doc["data"]);
        } else if (msgType == "aprs_send") {
            handleAPRSSend(client, doc["data"]);
        }
        // ... handle other message types
    }
}
```

## Security Considerations

- **No Authentication**: Current implementation has no authentication
- **Network Security**: Use only on trusted networks
- **Data Validation**: All incoming data is validated before processing
- **Rate Limiting**: Commands are rate-limited to prevent abuse
- **Future**: HTTPS/WSS and authentication should be added for production use

## Debugging and Troubleshooting

### Common Issues
1. **Connection Refused**: Check WiFi connection and IP address
2. **Data Not Updating**: Verify WebSocket connection status
3. **Commands Not Working**: Check message format and device status
4. **High Latency**: Monitor network conditions and device load

### Debug Tools
- Browser developer console for WebSocket traffic
- Serial monitor for ESP32 debug output
- Network analyzer for packet inspection