# LoRaTNCX REST API Documentation

## 📡 API Overview

### Base Configuration
- **Protocol**: HTTP REST API
- **Default Port**: 80
- **Content-Type**: `application/json`
- **CORS**: Enabled (all origins accepted)
- **Authentication**: None (currently open API)

---

## 🎯 API Endpoints

### 1. System Information & Status

#### `GET /api/status`
**Purpose**: Real-time device status monitoring

**Response Schema**:
```json
{
  "wifi": {
    "connected": boolean,
    "ap_active": boolean,
    "sta_ip": "string (IP address)",
    "ap_ip": "string (IP address)",
    "rssi": number
  },
  "lora": {
    "enabled": boolean
  },
  "gnss": {
    "enabled": boolean
  },
  "battery": {
    "voltage": float
  },
  "system": {
    "uptime": number (seconds),
    "free_heap": number (bytes),
    "min_free_heap": number (bytes),
    "heap_size": number (bytes)
  }
}
```

**Use Cases**: Dashboard monitoring, health checks, connectivity status

---

#### `GET /api/system`
**Purpose**: Hardware and system information

**Response Schema**:
```json
{
  "board": {
    "name": "string",
    "type": number (3 = V3, 4 = V4)
  },
  "chip": {
    "model": "string",
    "revision": number,
    "cores": number,
    "frequency": number (MHz)
  },
  "memory": {
    "flash_size": number (bytes),
    "free_heap": number (bytes),
    "heap_size": number (bytes)
  },
  "storage": {
    "spiffs_used": number (bytes),
    "spiffs_total": number (bytes)
  }
}
```

**Use Cases**: Device identification, hardware capability detection, diagnostics

---

### 2. LoRa Radio Configuration

#### `GET /api/lora/config`
**Purpose**: Retrieve current LoRa radio configuration

**Response Schema**:
```json
{
  "frequency": float (MHz, 150.0-960.0),
  "bandwidth": float (kHz, 125/250/500),
  "spreading": number (7-12),
  "codingRate": number (5-8),
  "power": number (dBm, -9 to 22/28),
  "syncWord": number (0x0000-0xFFFF)
}
```

**Notes**:
- `power` max is 22 dBm for V3, 28 dBm for V4 boards
- Default: 915.0 MHz, 125 kHz BW, SF12, CR 4/7, 20 dBm, sync 0x1424

---

#### `POST /api/lora/config`
**Purpose**: Apply LoRa radio configuration (runtime only)

**Request Body Schema**:
```json
{
  "frequency": float (optional),
  "bandwidth": float (optional),
  "spreading": number (optional),
  "codingRate": number (optional),
  "power": number (optional),
  "syncWord": number (optional)
}
```

**Response**:
```json
{
  "success": true,
  "message": "Configuration applied"
}
```

**Validation**:
- `frequency`: 150.0 - 960.0 MHz
- `spreading`: 7 - 12
- `codingRate`: 5 - 8 (represents 4/5 to 4/8)
- `power`: -9 to 22 dBm (V3) or 28 dBm (V4)
- `syncWord`: 0x0000 - 0xFFFF

**Behavior**: Configuration applied immediately, radio reconfigured automatically

---

#### `POST /api/lora/save`
**Purpose**: Save current LoRa configuration to NVS (persistent)

**Response**:
```json
{
  "success": true,
  "message": "Configuration saved to NVS"
}
```

**Use Case**: Persist runtime configuration changes to survive reboots

---

#### `POST /api/lora/reset`
**Purpose**: Reset LoRa configuration to defaults

**Response**:
```json
{
  "success": true,
  "message": "Configuration reset to defaults"
}
```

**Default Values**: 915 MHz, 125 kHz, SF12, CR7, 20 dBm, sync 0x1424

---

### 3. WiFi Management

#### `GET /api/wifi/config`
**Purpose**: Retrieve current WiFi configuration

**Response Schema**:
```json
{
  "ssid": "string",
  "ap_ssid": "string",
  "mode": number (0=off, 1=AP, 2=STA, 3=AP+STA),
  "dhcp": boolean,
  "ip": "string (dotted quad)",
  "gateway": "string (dotted quad)",
  "subnet": "string (dotted quad)",
  "dns": "string (dotted quad)",
  "tcp_kiss_enabled": boolean,
  "tcp_kiss_port": number
}
```

**Notes**: Passwords are never returned in responses for security

---

#### `POST /api/wifi/config`
**Purpose**: Configure WiFi settings (delayed application)

**Request Body Schema**:
```json
{
  "ssid": "string" (optional),
  "password": "string" (optional),
  "ap_ssid": "string" (optional),
  "ap_password": "string" (optional),
  "mode": number (optional, 0-3),
  "dhcp": boolean (optional),
  "tcp_kiss_enabled": boolean (optional),
  "tcp_kiss_port": number (optional)
}
```

**Response**:
```json
{
  "success": true,
  "message": "WiFi configuration will be applied"
}
```

**Behavior**: 
- Changes are delayed by 1 second to allow response delivery
- Configuration is automatically saved to NVS
- Device may restart if WiFi mode changes significantly

---

#### `POST /api/wifi/save`
**Purpose**: Explicitly save WiFi configuration to NVS

**Response**:
```json
{
  "success": true,
  "message": "WiFi configuration saved"
}
```

---

#### `GET /api/wifi/scan`
**Purpose**: Start WiFi network scan

**Response**:
```json
{
  "status": "started"
}
```

**Notes**: Scan runs asynchronously, check status with `/api/wifi/scan/status`

---

#### `GET /api/wifi/scan/status`
**Purpose**: Get WiFi scan results

**Response Schema** (scanning):
```json
{
  "status": "scanning",
  "progress": number (0-100)
}
```

**Response Schema** (completed):
```json
{
  "status": "completed",
  "networks": [
    {
      "ssid": "string",
      "rssi": number (dBm),
      "encrypted": boolean
    }
  ]
}
```

**Response Schema** (idle):
```json
{
  "status": "idle"
}
```

---

### 4. GNSS Configuration & Status

#### `GET /api/gnss/config`
**Purpose**: Retrieve GNSS configuration

**Response Schema**:
```json
{
  "enabled": boolean,
  "serialPassthrough": boolean,
  "pinRX": number,
  "pinTX": number,
  "pinCtrl": number,
  "pinWake": number,
  "pinPPS": number,
  "pinRST": number,
  "baudRate": number (4800/9600/19200/38400/57600/115200),
  "tcpPort": number,
  "hasBuiltInPort": boolean
}
```

**Notes**: Pin configuration only relevant if `hasBuiltInPort` is false

---

#### `POST /api/gnss/config`
**Purpose**: Configure GNSS module

**Request Body Schema**:
```json
{
  "enabled": boolean (optional),
  "serialPassthrough": boolean (optional),
  "tcpPort": number (optional),
  "baudRate": number (optional),
  "pinRX": number (optional),
  "pinTX": number (optional),
  "pinCtrl": number (optional),
  "pinWake": number (optional),
  "pinPPS": number (optional),
  "pinRST": number (optional)
}
```

**Response**:
```json
{
  "success": true,
  "message": "GNSS configuration saved",
  "rebootRequired": boolean
}
```

**Notes**: 
- Changing baud rate or TCP port requires device reboot
- Configuration is saved to NVS automatically
- GNSS module is powered on/off based on `enabled` flag

---

#### `GET /api/gnss/status`
**Purpose**: Get real-time GNSS status and position

**Response Schema** (running with fix):
```json
{
  "running": true,
  "hasFix": true,
  "latitude": float,
  "longitude": float,
  "altitude": float,
  "speed": float,
  "course": float,
  "satellites": number,
  "hdop": float,
  "time": "string (HH:MM:SS)",
  "date": "string (YYYY-MM-DD)",
  "charsProcessed": number,
  "passedChecksums": number,
  "failedChecksums": number,
  "nmeaServer": {
    "running": boolean,
    "clients": number,
    "port": number
  }
}
```

**Response Schema** (not running):
```json
{
  "running": false,
  "hasFix": false
}
```

---

### 5. Device Control

#### `POST /api/reboot`
**Purpose**: Restart the device

**Response**:
```json
{
  "success": true,
  "message": "Device restarting"
}
```

**Behavior**: Device restarts after 1 second delay

---

## 🔧 Implementation Notes

### CORS Support
All API responses include CORS headers:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type`

### Error Handling
- **400 Bad Request**: Invalid JSON or malformed request
- **404 Not Found**: Unknown endpoint
- **500 Internal Server Error**: Configuration save failure

### Performance Optimizations
- Static assets served with 1-hour cache (`max-age=3600`)
- GZ compression support for CSS/JS files
- Asynchronous request handling (non-blocking)

### Security Considerations
- ⚠️ **No authentication** - Secure your network!
- Passwords never returned in GET responses
- CORS allows all origins (suitable for local network only)

---

## 📱 Client Application Development Tips

### 1. Board Type Detection
Always fetch `/api/system` first to determine board type, as this affects TX power limits (22 dBm vs 28 dBm).

### 2. Configuration Workflow
```
1. GET /api/lora/config (or wifi, gnss)
2. Modify configuration locally
3. POST /api/lora/config (apply changes)
4. POST /api/lora/save (persist to NVS)
```

### 3. WiFi Scanning Pattern
```
1. GET /api/wifi/scan (start scan)
2. Poll GET /api/wifi/scan/status every 1-2 seconds
3. When status="completed", display networks
```

### 4. Real-time Monitoring
Poll these endpoints for live updates:
- `/api/status` - every 5 seconds
- `/api/gnss/status` - every 2-5 seconds (if GNSS enabled)

### 5. Error Recovery
- Always check `success` field in POST responses
- Handle network timeouts gracefully
- Retry failed requests with exponential backoff

### 6. Validation
Validate all user inputs client-side before sending to API:
- Frequency: 150.0 - 960.0 MHz
- Spreading Factor: 7 - 12
- Coding Rate: 5 - 8
- Power: -9 to 22/28 dBm (check board type!)
- GNSS Baud: Only standard rates (4800, 9600, 19200, 38400, 57600, 115200)

---

## 🎯 Example API Flows

### Configure LoRa for Long Range
```bash
# 1. Get current config
GET /api/lora/config

# 2. Apply long-range settings
POST /api/lora/config
{
  "frequency": 915.0,
  "bandwidth": 125.0,
  "spreading": 12,
  "codingRate": 8,
  "power": 22
}

# 3. Save to NVS
POST /api/lora/save
```

### Connect to WiFi Network
```bash
# 1. Scan for networks
GET /api/wifi/scan

# 2. Check scan results
GET /api/wifi/scan/status

# 3. Configure station mode
POST /api/wifi/config
{
  "mode": 2,
  "ssid": "MyNetwork",
  "password": "MyPassword"
}
```

### Monitor GNSS Position
```bash
# 1. Enable GNSS
POST /api/gnss/config
{
  "enabled": true
}

# 2. Poll for position updates
GET /api/gnss/status
# Repeat every 2-5 seconds
```

---

## 📚 Additional Resources

- [KISS Protocol Documentation](docs/KISS_PHILOSOPHY.md)
- [Hardware Configuration](docs/SETHARDWARE_COMMAND.md)
- [Testing Guide](docs/TESTING.md)
- [User Manual](manual/user_manual.md)

---

**Note**: This API provides complete remote configuration and monitoring capabilities for your LoRaTNCX device. The design is RESTful, JSON-based, and suitable for cross-platform application development.
