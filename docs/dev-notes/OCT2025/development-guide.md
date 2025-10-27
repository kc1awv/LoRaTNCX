# Web Interface Development Guide

**Quick Start Guide for LoRaTNCX Web Interface Development**  
**Date:** October 27, 2025

## Overview

This guide helps developers understand, modify, and extend the LoRaTNCX web interface. The system consists of a responsive Bootstrap frontend communicating via WebSocket with an ESP32-based backend.

## Architecture

```
Frontend (Browser)          Backend (ESP32)
┌─────────────────┐         ┌──────────────────┐
│ Bootstrap UI    │         │ AsyncWebServer   │
│ Chart.js        │ ◄────►  │ WebSocket Server │
│ WebSocket Client│  JSON   │ LoRaTNCX Core    │
└─────────────────┘         └──────────────────┘
```

## Development Environment Setup

### Prerequisites
- PlatformIO (ESP32 development)
- Python 3.7+ (build tools)
- Modern web browser (Chrome/Firefox/Safari)
- Text editor (VS Code recommended)

### Project Structure
```
LoRaTNCX/
├── web/src/               # Frontend source files
│   ├── index.html         # Main HTML interface
│   ├── app.js             # WebSocket client & UI logic
│   └── style.css          # Custom styling
├── tools/                 # Build automation
│   ├── build_web_pre.py   # PlatformIO integration
│   └── build_web.py       # Standalone builder
├── include/               # ESP32 headers
│   └── WebSocketServer.h  # WebSocket server interface
├── src/                   # ESP32 source
│   ├── WebSocketServer.cpp # WebSocket implementation
│   └── main.cpp           # Main application
└── data/                  # Built web assets (auto-generated)
```

## Frontend Development

### HTML Structure (index.html)

The interface uses Bootstrap tabs for organization:

```html
<!-- Navigation Tabs -->
<ul class="nav nav-tabs" id="mainTabs">
    <li class="nav-item">
        <a class="nav-link active" data-bs-toggle="tab" href="#dashboard">Dashboard</a>
    </li>
    <li class="nav-item">
        <a class="nav-link" data-bs-toggle="tab" href="#radio">Radio</a>
    </li>
    <!-- More tabs... -->
</ul>

<!-- Tab Content -->
<div class="tab-content" id="mainTabsContent">
    <div class="tab-pane fade show active" id="dashboard">
        <!-- Dashboard content with charts and status -->
    </div>
    <!-- More tab panes... -->
</div>
```

### JavaScript Architecture (app.js)

The JavaScript is organized into modules:

```javascript
// WebSocket connection management
class WebSocketManager {
    constructor(url) {
        this.url = url;
        this.ws = null;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
    }
    
    connect() {
        this.ws = new WebSocket(this.url);
        this.ws.onopen = this.onOpen.bind(this);
        this.ws.onmessage = this.onMessage.bind(this);
        this.ws.onclose = this.onClose.bind(this);
        this.ws.onerror = this.onError.bind(this);
    }
    
    // Message handlers...
}

// UI component managers
class DashboardManager { /* ... */ }
class RadioManager { /* ... */ }
class APRSManager { /* ... */ }
```

### CSS Styling (style.css)

Custom styles supplement Bootstrap:

```css
/* Color scheme */
:root {
    --primary-color: #2c5aa0;
    --secondary-color: #34a853;
    --warning-color: #fbbc04;
    --danger-color: #ea4335;
}

/* Status indicators */
.status-indicator {
    display: inline-block;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    margin-right: 5px;
}

.status-connected { background-color: var(--secondary-color); }
.status-disconnected { background-color: var(--danger-color); }
```

## Backend Development

### WebSocket Server (WebSocketServer.cpp)

The server handles WebSocket connections and message routing:

```cpp
class WebSocketServer {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    
public:
    void begin(AsyncWebServer* webServer);
    void loop();
    void broadcastSystemStatus();
    
private:
    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                AwsEventType type, void* arg, uint8_t* data, size_t len);
    void handleMessage(AsyncWebSocketClient* client, JsonDocument& doc);
    void handleConfigUpdate(AsyncWebSocketClient* client, JsonObject& data);
    void handleAPRSSend(AsyncWebSocketClient* client, JsonObject& data);
};
```

### Message Handlers

Each subsystem has dedicated message handlers:

```cpp
void WebSocketServer::handleConfigUpdate(AsyncWebSocketClient* client, JsonObject& data) {
    String section = data["section"];
    
    if (section == "radio") {
        if (data["parameters"].containsKey("frequency")) {
            uint32_t freq = data["parameters"]["frequency"];
            // Update radio frequency
            radioManager.setFrequency(freq);
        }
        // Handle other radio parameters...
    }
    
    // Send acknowledgment
    sendResponse(client, "config_update", "success");
}
```

## Building and Deployment

### Automated Build Process

The build system automatically:
1. Downloads external dependencies (Bootstrap, Chart.js)
2. Processes source files
3. Compresses assets with gzip
4. Generates SPIFFS filesystem image

```bash
# Build web interface
python3 tools/build_web_pre.py

# Compile and upload firmware
platformio run -e heltec_wifi_lora_32_V4 -t upload

# Upload web interface to SPIFFS
platformio run -e heltec_wifi_lora_32_V4 -t uploadfs
```

### Manual Development Workflow

For faster development iterations:

```bash
# 1. Modify web interface files (web/src/)
# 2. Build web interface
python3 tools/build_web.py

# 3. Upload only SPIFFS (faster than full firmware)
platformio run -t uploadfs

# 4. Test in browser at http://192.168.4.1/
```

## Adding New Features

### 1. Adding a New Tab

**Step 1**: Add tab to HTML
```html
<li class="nav-item">
    <a class="nav-link" data-bs-toggle="tab" href="#newtab">New Feature</a>
</li>
```

**Step 2**: Add tab content
```html
<div class="tab-pane fade" id="newtab">
    <div class="row">
        <div class="col-md-6">
            <div class="card">
                <div class="card-header">New Feature</div>
                <div class="card-body">
                    <!-- Your content here -->
                </div>
            </div>
        </div>
    </div>
</div>
```

**Step 3**: Add JavaScript handler
```javascript
class NewFeatureManager {
    constructor(wsManager) {
        this.wsManager = wsManager;
        this.init();
    }
    
    init() {
        // Initialize UI components
        this.setupEventHandlers();
    }
    
    handleMessage(message) {
        if (message.type === 'new_feature_data') {
            this.updateDisplay(message.data);
        }
    }
}
```

### 2. Adding New WebSocket Messages

**Frontend**: Send new message type
```javascript
function sendNewCommand(data) {
    wsManager.send({
        type: 'new_command',
        data: data
    });
}
```

**Backend**: Add message handler
```cpp
void WebSocketServer::handleMessage(AsyncWebSocketClient* client, JsonDocument& doc) {
    String msgType = doc["type"];
    
    if (msgType == "new_command") {
        handleNewCommand(client, doc["data"]);
    }
    // ... existing handlers
}

void WebSocketServer::handleNewCommand(AsyncWebSocketClient* client, JsonObject& data) {
    // Process new command
    // Send response if needed
    sendResponse(client, "new_command", "completed");
}
```

### 3. Adding Real-time Charts

Using Chart.js for new data visualization:

```javascript
// Create chart
const ctx = document.getElementById('newChart').getContext('2d');
const newChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'New Data',
            data: [],
            borderColor: 'rgb(75, 192, 192)',
            tension: 0.1
        }]
    },
    options: {
        responsive: true,
        scales: {
            y: {
                beginAtZero: true
            }
        }
    }
});

// Update chart with new data
function updateNewChart(value) {
    const now = new Date().toLocaleTimeString();
    
    newChart.data.labels.push(now);
    newChart.data.datasets[0].data.push(value);
    
    // Keep last 20 data points
    if (newChart.data.labels.length > 20) {
        newChart.data.labels.shift();
        newChart.data.datasets[0].data.shift();
    }
    
    newChart.update();
}
```

## Testing and Debugging

### Frontend Debugging

Use browser developer tools:
```javascript
// Enable debug logging
const DEBUG = true;

function debugLog(message, data) {
    if (DEBUG) {
        console.log(`[DEBUG] ${message}`, data);
    }
}

// Monitor WebSocket traffic
wsManager.ws.addEventListener('message', (event) => {
    debugLog('WebSocket received:', JSON.parse(event.data));
});
```

### Backend Debugging

Use serial monitoring:
```cpp
#define DEBUG_WEBSOCKET 1

void WebSocketServer::handleMessage(AsyncWebSocketClient* client, JsonDocument& doc) {
    #if DEBUG_WEBSOCKET
    Serial.printf("[WS] Received message type: %s\n", doc["type"].as<String>().c_str());
    #endif
    
    // Process message...
}
```

### Network Testing

Test WebSocket connection independently:
```javascript
// Simple WebSocket test
const testWs = new WebSocket('ws://192.168.4.1/ws');
testWs.onopen = () => console.log('Test connection opened');
testWs.onmessage = (event) => console.log('Test received:', event.data);
testWs.send(JSON.stringify({type: 'request', data: {what: 'status'}}));
```

## Performance Optimization

### Frontend Optimization
- Minimize DOM updates
- Use document fragments for bulk updates
- Debounce user input events
- Lazy load charts and heavy components

### Backend Optimization
- Limit WebSocket message frequency
- Use JSON streaming for large data
- Implement message queuing for busy periods
- Monitor memory usage and cleanup

## Common Issues and Solutions

### Issue: WebSocket Connection Fails
**Symptoms**: Cannot connect to device web interface
**Solutions**:
1. Check WiFi connection to device AP
2. Verify device IP address (usually 192.168.4.1)
3. Check firewall settings
4. Monitor serial output for network errors

### Issue: Interface Not Updating
**Symptoms**: Static data, no real-time updates
**Solutions**:
1. Check WebSocket connection status in browser console
2. Verify message handlers are properly registered
3. Monitor backend message broadcasting
4. Check for JavaScript errors in console

### Issue: Build Errors
**Symptoms**: Web interface build fails
**Solutions**:
1. Check Python dependencies: `pip install requests`
2. Verify internet connection (for CDN downloads)
3. Clear build cache and rebuild
4. Check file permissions in project directory

### Issue: Memory Issues on ESP32
**Symptoms**: Device crashes, WebSocket disconnects
**Solutions**:
1. Monitor heap usage via serial output
2. Reduce WebSocket message frequency
3. Optimize JSON message sizes
4. Check for memory leaks in custom code

## Best Practices

### Code Organization
- Keep related functionality in separate files/classes
- Use consistent naming conventions
- Document all public APIs
- Handle errors gracefully

### User Experience
- Provide loading indicators for async operations
- Show connection status clearly
- Validate user input before sending to device
- Use progressive enhancement for optional features

### Security
- Validate all incoming WebSocket messages
- Sanitize user input before display
- Use HTTPS/WSS in production environments
- Implement rate limiting for commands

## Contributing

When contributing new features:

1. **Test thoroughly** on actual hardware
2. **Update documentation** for any API changes
3. **Follow existing code style** and patterns
4. **Consider backwards compatibility** with existing configurations
5. **Add error handling** for edge cases

## Resources

- **Bootstrap Documentation**: https://getbootstrap.com/docs/5.3/
- **Chart.js Documentation**: https://www.chartjs.org/docs/latest/
- **WebSocket API**: https://developer.mozilla.org/en-US/docs/Web/API/WebSocket
- **AsyncWebServer Library**: https://github.com/me-no-dev/ESPAsyncWebServer
- **PlatformIO Documentation**: https://docs.platformio.org/

---

This development guide should help you understand and extend the LoRaTNCX web interface. For specific questions or issues, refer to the detailed API documentation and source code comments.