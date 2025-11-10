# 4. Software Architecture

This section explains the internal structure of LoRaTNCX firmware, including the KISS protocol implementation, interrupt handling, and system organization.

## 4.1. KISS Protocol Implementation

### Overview
LoRaTNCX implements the standard KISS (Keep It Simple, Stupid) protocol with LoRa-specific extensions. The KISS philosophy emphasizes silent operation - the TNC never sends unsolicited output, only responding to properly formatted KISS frames.

### Core Principles
- **Silent Operation**: No startup banners, debug output, or human-readable messages after startup
- **Frame-Based Communication**: All communication uses binary KISS frames
- **Computer-to-Computer**: Designed exclusively for programmatic control
- **Minimal State**: No complex command interpreters or mode switching

### KISS Frame Format
```
Frame Structure:
┌─────────┬─────────┬─────────┬─────────────────┐
│ FEND    │ Command │ Data... │ FEND            │
│ (0xC0)  │ (0x00+) │         │ (0xC0)          │
└─────────┴─────────┴─────────┴─────────────────┘

FEND = Frame End (0xC0)
Command = Frame type (0x00 = Data, 0x06 = SETHARDWARE, etc.)
```

### Standard KISS Commands
- **0x00**: DATA frame - Send/receive packet data
- **0xC0**: FEND - Frame delimiter
- **0xDB**: FESC - Frame escape character
- **0xDC**: TFEND - Transposed frame end
- **0xDD**: TFESC - Transposed frame escape

### LoRa Extensions (SETHARDWARE)
Command **0x06** with subcommands for LoRa configuration:

| Subcommand | Parameter     | Description                        |
| ---------- | ------------- | ---------------------------------- |
| 0x01       | 4-byte float  | Set frequency (MHz)                |
| 0x02       | 4-byte float  | Set bandwidth (kHz)                |
| 0x03       | 1-byte        | Set spreading factor (7-12)        |
| 0x04       | 1-byte        | Set coding rate (5-8 = 4/5 to 4/8) |
| 0x05       | 1-byte signed | Set TX power (2-20 dBm)            |
| 0x06       | -             | Get current configuration          |
| 0x07       | -             | Save to NVS flash                  |
| 0x08       | 2-byte        | Set sync word                      |
| 0xFF       | -             | Factory reset                      |

### LoRa Extensions (GETHARDWARE)
Command **0x07** for querying hardware status:

| Subcommand | Returns      | Description                        |
| ---------- | ------------ | ---------------------------------- |
| 0x01       | Radio config | Frequency, BW, SF, CR, power, sync |
| 0x02       | 4-byte float | Battery voltage                    |
| 0x03       | String       | Board type and name                |
| 0xFF       | All data     | Complete status dump               |

## 4.2. SETHARDWARE and GETHARDWARE Commands

### SETHARDWARE Implementation
The SETHARDWARE command (0x06) allows runtime configuration of LoRa parameters:

```cpp
// Example: Set frequency to 915.0 MHz
uint8_t frame[] = {0xC0, 0x06, 0x01, 0x00, 0x00, 0x3C, 0x70, 0xC0};
//                          ^     ^     ^     ^^^^^^^^^^^^     ^
//                         FEND  CMD  SUB   915.0 (float)   FEND
```

### GETHARDWARE Implementation
The GETHARDWARE command (0x07) provides read access to current settings:

```cpp
// Query battery voltage
uint8_t query[] = {0xC0, 0x07, 0x02, 0xC0};

// Response format:
// FEND, CMD, SUB, DATA..., FEND
// Example: Battery = 3.85V
// 0xC0, 0x07, 0x02, 0x40, 0x75, 0xC2, 0x8F, 0xC0
```

### Parameter Validation
- **Frequency**: 433.0 - 928.0 MHz (ISM bands)
- **Bandwidth**: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500 kHz
- **Spreading Factor**: 7-12 (higher = longer range, slower data rate)
- **Coding Rate**: 5-8 (4/5 through 4/8, higher = better error correction)
- **TX Power**: 2-20 dBm (V3 limited by internal PA, V4 can use full range)
- **Sync Word**: 2-byte value (0x1424 default for public networks)

## 4.3. Interrupt-Driven Reception

### Radio Event System
LoRaTNCX uses interrupt-driven packet reception for efficient operation:

```cpp
// Radio event callbacks
void OnTxDone(void) {
    // Transmission completed
    radioState = RADIO_STATE_RX;
    Radio.Rx(0); // Return to receive mode
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    // Packet received successfully
    processReceivedPacket(payload, size, rssi, snr);
}

void OnTxTimeout(void) {
    // Transmission timeout
    handleTxTimeout();
}

void OnRxTimeout(void) {
    // Receive timeout
    Radio.Rx(0); // Continue listening
}

void OnRxError(void) {
    // Receive error (CRC, etc.)
    Radio.Rx(0); // Continue listening
}
```

### Interrupt Sources
- **DIO0/DIO1**: Radio state change interrupts
- **BUSY**: Radio busy status
- **Timer**: Deaf period timing
- **Serial**: KISS frame reception

### Packet Processing Flow
1. **Radio Interrupt**: Packet received, triggers OnRxDone
2. **CRC Validation**: Automatic hardware CRC checking
3. **Frame Formatting**: Add KISS framing (FEND + DATA + FEND)
4. **Queue Management**: Buffer packet for serial transmission
5. **Serial Output**: Send framed packet to host application

### Transmit Flow
1. **KISS Frame Reception**: Host sends data frame
2. **Deframing**: Remove KISS framing, extract payload
3. **Radio Configuration**: Set TX parameters
4. **Transmission**: Send packet via LoRa radio
5. **Return to RX**: Automatically switch back to receive mode

## 4.4. Configuration Management

### NVS Storage
Configuration persists across power cycles using ESP32 Non-Volatile Storage:

```cpp
// Configuration structure
struct LoRaConfig {
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    int8_t txPower;
    uint16_t syncWord;
    // ... other settings
};

// Save configuration
void saveConfig() {
    preferences.begin("lora", false);
    preferences.putFloat("freq", config.frequency);
    preferences.putFloat("bw", config.bandwidth);
    // ... save other parameters
    preferences.end();
}

// Load configuration
void loadConfig() {
    preferences.begin("lora", true);
    config.frequency = preferences.getFloat("freq", 915.0);
    config.bandwidth = preferences.getFloat("bw", 125.0);
    // ... load other parameters
    preferences.end();
}
```

### Configuration Sources
1. **Factory Defaults**: Compiled-in default values
2. **NVS Storage**: Persistent user configuration
3. **Runtime Commands**: SETHARDWARE commands
4. **Web Interface**: JSON API updates

### Validation and Safety
- Parameter range checking
- Safe defaults for invalid values
- Configuration backup before changes
- Factory reset capability

## 4.5. Web Server and REST API

### Web Server Architecture
The firmware includes a complete web interface using ESPAsyncWebServer:

```cpp
// Web server setup
AsyncWebServer server(80);

// Serve static files from SPIFFS
server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

// API endpoints
server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = getSystemStatusJson();
    request->send(200, "application/json", json);
});

server.on("/api/lora/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Handle LoRa configuration update
    handleLoRaConfigUpdate(request);
});

// Start server
server.begin();
```

### REST API Endpoints

#### Status and Monitoring
- `GET /api/status` - System status (WiFi, battery, uptime)
- `GET /api/lora/config` - Current LoRa configuration
- `GET /api/wifi/config` - Current WiFi configuration
- `GET /api/wifi/scan` - Available WiFi networks

#### Configuration Updates
- `POST /api/lora/config` - Update LoRa parameters
- `POST /api/lora/save` - Save LoRa config to flash
- `POST /api/wifi/config` - Update WiFi settings
- `POST /api/reboot` - Reboot the device

### Web Interface Features
- **Responsive Design**: Works on desktop, tablet, and mobile
- **Real-time Updates**: AJAX polling for status information
- **Form Validation**: Client-side parameter validation
- **Error Handling**: User-friendly error messages
- **Security**: Basic authentication for configuration changes

### SPIFFS File System
Web interface files are stored in SPIFFS:
- `index.html` - Main interface
- `style.css` - Styling
- `app.js` - Client-side JavaScript
- Various assets (icons, fonts)

### Power Considerations
- Web server can be disabled to save power
- WiFi modes: Off, AP only, STA only, AP+STA
- Automatic power management for battery operation

---

[← Previous: Hardware Overview](03-hardware-overview.md) | [Back to Main Manual](../user_manual.md) | [Next: Installation and Setup →](05-installation-setup.md)