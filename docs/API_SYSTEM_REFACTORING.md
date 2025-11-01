# API System Refactoring

## Overview

The WebServerManager has been refactored to separate API handling from static file serving, creating a more modular and maintainable architecture.

## New Structure

### Base Classes
- **APIHandler**: Base class for all API handlers with common functionality
- **APIManager**: Central coordinator for all API handlers

### Specific API Handlers
- **SystemAPIHandler**: System status, hardware info, performance metrics
- **LoRaAPIHandler**: LoRa configuration, status, and statistics
- **WiFiAPIHandler**: WiFi management, scanning, connection handling

## Key Improvements

1. **Separation of Concerns**: API logic is now separate from web server management
2. **Modular Design**: Each API module can be developed and tested independently
3. **Consistent Response Format**: All API responses follow a standard JSON structure
4. **Better Error Handling**: Standardized error responses with proper HTTP status codes
5. **Enhanced CORS Support**: Proper CORS headers for all API endpoints
6. **Extensibility**: Easy to add new API handlers

## API Endpoints

### System API (`/api/system/`)
- `GET /status` - System status with uptime and memory info
- `GET /info` - Hardware and software information
- `GET /performance` - Performance metrics and reset information

### LoRa API (`/api/lora/`)
- `GET /status` - LoRa radio status
- `GET /config` - Current configuration
- `POST /config` - Set configuration parameters
- `GET /stats` - Packet statistics and signal info

### WiFi API (`/api/wifi/`)
- `GET /networks` - List available networks
- `GET /status` - Current WiFi connection status
- `POST /scan` - Trigger network scan
- `POST /add` - Add network to known list
- `POST /remove` - Remove network from known list
- `POST /connect` - Connect to specific network
- `POST /disconnect` - Disconnect from current network

### Debug API (`/api/debug/`)
- `GET /files` - List SPIFFS files

## Response Format

All API responses follow this structure:

### Success Response
```json
{
  "status": "success",
  "message": "Optional success message",
  "data": { ... },
  "timestamp": 12345
}
```

### Error Response
```json
{
  "status": "error",
  "message": "Error description",
  "timestamp": 12345
}
```

## Usage

The WebServerManager now uses the APIManager internally. Callbacks are set through the WebServerManager as before, but they're automatically routed to the appropriate API handlers.

```cpp
// Initialize web server (unchanged)
webServer.begin();

// Set callbacks (unchanged interface)
webServer.setCallbacks(
    getSystemStatus,
    getLoRaStatus,
    getWiFiNetworks,
    addWiFiNetwork,
    removeWiFiNetwork
);

// Access API managers for advanced configuration
APIManager* apiManager = webServer.getAPIManager();
SystemAPIHandler* systemAPI = apiManager->getSystemHandler();
```

## Benefits

1. **Easier Maintenance**: Each API module is self-contained
2. **Better Testing**: Individual API handlers can be unit tested
3. **Cleaner Code**: WebServerManager focuses only on static file serving
4. **Consistent APIs**: All endpoints follow the same patterns
5. **Future-Proof**: Easy to add new API modules without touching existing code
6. **Better Documentation**: API structure is self-documenting through the `/api` endpoint

## Migration

The existing interface remains unchanged - all existing callback setters work as before. The refactoring is internal and doesn't break existing code.