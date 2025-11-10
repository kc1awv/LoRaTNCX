# 16. Development and Customization

## Overview

This section provides guidance for developers who want to modify, extend, or contribute to the LoRaTNCX firmware. Topics include source code architecture, adding new features, build customization, and contribution guidelines.

## Source Code Overview

### Project Architecture

LoRaTNCX follows a modular architecture with clear separation of concerns:

```
LoRaTNCX/
├── src/                    # Main source code
│   ├── main.cpp           # Application entry point and initialization
│   ├── board_config.cpp   # Board-specific pin configurations
│   ├── config_manager.cpp # Configuration persistence and management
│   ├── display.cpp        # OLED display and user interface
│   ├── gnss.cpp           # GNSS module handling
│   ├── kiss.cpp           # KISS protocol implementation
│   ├── nmea_server.cpp   # NMEA-over-TCP server
│   ├── radio.cpp          # LoRa radio interface
│   ├── tcp_kiss.cpp       # TCP KISS server
│   ├── web_server.cpp    # RESTful web API and interface
│   └── wifi_manager.cpp   # WiFi connectivity management
├── include/               # Header files
│   ├── board_config.h     # Board pin definitions
│   ├── config.h           # Protocol constants and defaults
│   ├── config_manager.h   # Configuration management interface
│   ├── debug.h            # Debug utilities
│   ├── display.h          # Display interface
│   ├── gnss.h             # GNSS module interface
│   ├── kiss.h             # KISS protocol definitions
│   ├── nmea_server.h     # NMEA server interface
│   ├── radio.h            # Radio interface
│   ├── tcp_kiss.h         # TCP KISS server interface
│   ├── web_server.h      # Web server interface
│   └── wifi_manager.h     # WiFi manager interface
├── boards/                # Board-specific configurations
├── variants/              # Arduino variant definitions
├── data/                  # Web interface files (SPIFFS)
├── test/                  # Test utilities
├── tools/                 # Development tools
└── docs/                  # Documentation
```

### Core Components

**Main Application (`main.cpp`):**
- System initialization and startup sequence
- Service instantiation and dependency injection
- Main event loop and task coordination
- Error handling and recovery

**KISS Protocol (`kiss.cpp`, `kiss.h`):**
- Frame encoding/decoding with byte stuffing
- Command processing (DATA, SETHARDWARE, GETHARDWARE)
- Serial communication handling
- Protocol state management

**Radio Interface (`radio.cpp`, `radio.h`):**
- LoRa radio configuration and control
- Packet transmission and reception
- RadioLib integration
- Interrupt-driven receive handling

**Configuration Management (`config_manager.cpp`, `config_manager.h`):**
- Persistent storage using ESP32 NVS
- Runtime configuration management
- Parameter validation and bounds checking
- Factory reset functionality

**WiFi Management (`wifi_manager.cpp`, `wifi_manager.h`):**
- AP/STA/AP+STA mode support
- Network configuration and connection
- DHCP server for AP mode
- Connection monitoring and recovery

**Web Server (`web_server.cpp`, `web_server.h`):**
- RESTful API endpoints
- Static file serving (SPIFFS)
- WebSocket support for real-time updates
- CORS handling for web interface

**TCP KISS Server (`tcp_kiss.cpp`, `tcp_kiss.h`):**
- Multi-client TCP server (up to 4 simultaneous connections)
- KISS protocol over TCP
- Client connection management
- Thread-safe packet distribution

**GNSS Module (`gnss.cpp`, `gnss.h`):**
- GPS/GLONASS/BeiDou/QZSS support
- TinyGPS++ integration
- NMEA sentence parsing
- Location data caching

**Display System (`display.cpp`, `display.h`):**
- U8g2 OLED library integration
- Real-time status display
- Button input handling
- Power management (deep sleep)

### Build System

**PlatformIO Configuration:**
```ini
[env]
platform = espressif32
framework = arduino
board_build.variants_dir = variants
monitor_speed = 115200
lib_deps = 
    RadioLib@^7.4.0
    olikraus/U8g2@^2.35.30
    mathieucarbou/ESPAsyncWebServer@^3.6.0
    bblanchon/ArduinoJson@^7.0.0
    mikalhart/TinyGPSPlus@^1.1.0
build_flags = 
    -DCONFIG_TASK_WDT_TIMEOUT_S=30
    -DCONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU0=0
    -DCONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU1=0
```

**Board-Specific Environments:**
```ini
[env:heltec_wifi_lora_32_V3]
board = heltec_wifi_lora_32_V3
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V3

[env:heltec_wifi_lora_32_V4]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
```

### Key Dependencies

**RadioLib (^7.4.0):**
- LoRa radio control and configuration
- SX1262/SX127x support
- Packet handling and modulation

**U8g2 (^2.35.30):**
- OLED display driver
- Font rendering and graphics
- Hardware abstraction for multiple displays

**ESPAsyncWebServer (^3.6.0):**
- Asynchronous HTTP server
- WebSocket support
- Static file serving

**ArduinoJson (^7.0.0):**
- JSON parsing and generation
- REST API data handling
- Configuration serialization

**TinyGPSPlus (^1.1.0):**
- NMEA sentence parsing
- GNSS data extraction
- Location calculation utilities

## Development Environment Setup

### Prerequisites

**Required Software:**
- **PlatformIO IDE** (VS Code extension) or PlatformIO Core
- **Python 3.7+** (for configuration tools and testing)
- **Git** (for version control and contributions)

**Hardware Requirements:**
- Heltec WiFi LoRa 32 V3 or V4 board
- USB cable for programming
- Optional: GNSS module for V4 boards

### Installation

**1. Clone the Repository:**
```bash
git clone https://github.com/kc1awv/LoRaTNCX.git
cd LoRaTNCX
```

**2. Install PlatformIO:**
```bash
# Using pip (recommended)
pip install platformio

# Or install VS Code extension
# Search for "PlatformIO IDE" in VS Code extensions
```

**3. Install Dependencies:**
```bash
# PlatformIO will automatically download libraries
# First build will install all required dependencies
platformio run --environment heltec_wifi_lora_32_V4
```

**4. Configure Board Selection:**
```bash
# For V3 boards
export PLATFORMIO_ENV=heltec_wifi_lora_32_V3

# For V4 boards  
export PLATFORMIO_ENV=heltec_wifi_lora_32_V4
```

### Build and Upload

**Basic Build:**
```bash
# Build for V4 (default)
platformio run

# Build for specific board
platformio run --environment heltec_wifi_lora_32_V4
platformio run --environment heltec_wifi_lora_32_V3
```

**Upload Firmware:**
```bash
# Upload to connected board
platformio run --target upload --environment heltec_wifi_lora_32_V4

# Upload filesystem (web interface)
platformio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

**Monitor Serial Output:**
```bash
# Open serial monitor
platformio device monitor --environment heltec_wifi_lora_32_V4
```

### VS Code Tasks

The project includes pre-configured VS Code tasks for common operations:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build (V4)",
            "type": "shell",
            "command": "platformio",
            "args": ["run", "--environment", "heltec_wifi_lora_32_V4"],
            "group": "build"
        },
        {
            "label": "Upload (V4)",
            "type": "shell", 
            "command": "platformio",
            "args": ["run", "--target", "upload", "--environment", "heltec_wifi_lora_32_V4"],
            "group": "build"
        },
        {
            "label": "Upload Filesystem (V4)",
            "type": "shell",
            "command": "platformio",
            "args": ["run", "--target", "uploadfs", "--environment", "heltec_wifi_lora_32_V4"],
            "group": "build"
        }
    ]
}
```

## Adding New Features

### Feature Development Process

**1. Plan the Feature:**
- Define requirements and scope
- Identify affected components
- Consider backward compatibility
- Plan testing approach

**2. Implement Core Logic:**
- Add new classes/functions in appropriate modules
- Update header files with new interfaces
- Follow existing code patterns and naming conventions

**3. Integrate with Main Application:**
- Add service instantiation in `main.cpp`
- Wire dependencies between components
- Update initialization sequence if needed

**4. Add Configuration Support:**
- Define new parameters in `config.h`
- Add validation ranges and defaults
- Update configuration persistence

**5. Update User Interfaces:**
- Add REST API endpoints in `web_server.cpp`
- Update web interface files in `data/`
- Add KISS command support if needed

### Example: Adding a New Radio Parameter

**1. Define the Parameter:**
```cpp
// In config.h
#define HW_SET_NEW_PARAMETER 0x0A  // Next available subcommand

// Add to validation ranges
#define RADIO_NEW_PARAM_MIN 0
#define RADIO_NEW_PARAM_MAX 100

// Add to defaults
#define LORA_NEW_PARAM 50
```

**2. Implement in Radio Class:**
```cpp
// In radio.h
class LoRaRadio {
public:
    bool setNewParameter(uint8_t value);
    uint8_t getNewParameter() const;
    
private:
    uint8_t newParameter = LORA_NEW_PARAM;
};

// In radio.cpp
bool LoRaRadio::setNewParameter(uint8_t value) {
    if (value < RADIO_NEW_PARAM_MIN || value > RADIO_NEW_PARAM_MAX) {
        return false;
    }
    newParameter = value;
    // Apply to radio hardware
    return radio.setNewParameter(value);
}

uint8_t LoRaRadio::getNewParameter() const {
    return newParameter;
}
```

**3. Add KISS Command Support:**
```cpp
// In kiss.cpp
void KISSProtocol::processHardwareCommand(const uint8_t* data, size_t length) {
    // ... existing commands ...
    
    case HW_SET_NEW_PARAMETER:
        if (length >= 1) {
            uint8_t value = data[0];
            if (loraRadio.setNewParameter(value)) {
                sendHardwareResponse(HW_SET_NEW_PARAMETER, &value, 1);
            } else {
                sendErrorResponse("Invalid new parameter value");
            }
        }
        break;
}
```

**4. Add Configuration Persistence:**
```cpp
// In config_manager.h
struct LoRaConfig {
    // ... existing fields ...
    uint8_t newParameter;
};

// In config_manager.cpp
void ConfigManager::loadDefaults() {
    // ... existing defaults ...
    config.newParameter = LORA_NEW_PARAM;
}

void ConfigManager::applyConfig() {
    // ... existing application ...
    loraRadio.setNewParameter(config.newParameter);
}
```

**5. Update Web Interface:**
```cpp
// In web_server.cpp
void TNCWebServer::handleGetConfig(AsyncWebServerRequest* request) {
    // ... existing config ...
    doc["newParameter"] = loraRadio.getNewParameter();
}

void TNCWebServer::handleSetConfig(AsyncWebServerRequest* request) {
    // ... existing validation ...
    if (request->hasParam("newParameter")) {
        uint8_t value = request->getParam("newParameter")->value().toInt();
        if (loraRadio.setNewParameter(value)) {
            configManager.saveConfig();
        }
    }
}
```

### Adding Hardware Support

**Board-Specific Pin Definitions:**
```cpp
// In board_config.h
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
    #define NEW_PIN 42  // GPIO pin for new feature
#else
    #define NEW_PIN 38  // Different pin for V3
#endif
```

**Conditional Compilation:**
```cpp
// In main.cpp
#ifdef FEATURE_NEW_HARDWARE
    initializeNewHardware();
#endif
```

**Build Flag Configuration:**
```ini
# In platformio.ini
[env:heltec_wifi_lora_32_V4_custom]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -DFEATURE_NEW_HARDWARE  # Enable new feature
```

### Testing New Features

**Unit Testing:**
```python
# In test/ directory
def test_new_parameter():
    # Test parameter validation
    assert radio.setNewParameter(50) == True
    assert radio.getNewParameter() == 50
    
    # Test bounds checking
    assert radio.setNewParameter(150) == False  # Out of range
    assert radio.setNewParameter(0) == True    # Minimum
    assert radio.setNewParameter(100) == True  # Maximum
```

**Integration Testing:**
```bash
# Test with configuration tool
python3 tools/loratncx_config.py /dev/ttyACM0 --new-parameter 75

# Verify via KISS protocol
python3 test/kiss_test.py --set-new-param 75 --verify
```

**Field Testing:**
- Test in various environments
- Verify power consumption impact
- Check for interference with existing features
- Validate user interface integration

## Modifying Configurations

### Build-Time Configuration

**Custom Build Flags:**
```ini
# platformio.ini - Custom environment
[env:custom_build]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -DLOG_LEVEL=4                    # Maximum logging
    -DDEAF_PERIOD_MS=5000           # Longer deaf period
    -DCUSTOM_FREQUENCY_DEFAULT=868.0 # Different default frequency
    -DMAX_TCP_CLIENTS=8             # More TCP clients
```

**Feature Flags:**
```cpp
// Conditional compilation based on build flags
#ifdef CUSTOM_FEATURE_ENABLED
    // Custom feature code
#endif

#ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_LEVEL_INFO
#endif
```

### Runtime Configuration

**Adding New Parameters:**
```cpp
// In config.h - Extend LoRaConfig struct
struct LoRaConfig {
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t power;
    uint16_t syncWord;
    bool gnssEnabled;
    // New parameters
    uint8_t newParameter;
    bool customFeatureEnabled;
    uint32_t customTimeout;
};
```

**Parameter Validation:**
```cpp
// In config_manager.cpp
bool ConfigManager::validateConfig(const LoRaConfig& cfg) {
    return (cfg.frequency >= RADIO_FREQ_MIN && cfg.frequency <= RADIO_FREQ_MAX) &&
           (cfg.spreadingFactor >= RADIO_SF_MIN && cfg.spreadingFactor <= RADIO_SF_MAX) &&
           (cfg.newParameter >= RADIO_NEW_PARAM_MIN && cfg.newParameter <= RADIO_NEW_PARAM_MAX) &&
           // ... other validations
           true;
}
```

### Board Variants

**Creating New Board Support:**
```cpp
// In board_config.h
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V5  // Hypothetical V5
    #define LORA_SCK 9
    #define LORA_MISO 11
    #define LORA_MOSI 10
    #define LORA_CS 8
    #define LORA_RST 12
    #define LORA_DIO1 14
    #define LORA_BUSY 13
    
    #define OLED_SDA 17
    #define OLED_SCL 18
    #define OLED_RST 21
    
    #define BUTTON_PIN 0
    
    #define GNSS_TX 33
    #define GNSS_RX 34
    
    #define BATTERY_PIN 1
#endif
```

**PlatformIO Board Configuration:**
```ini
# In platformio.ini
[env:heltec_wifi_lora_32_V5]
board = heltec_wifi_lora_32_V5  # Custom board definition
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V5
```

### Custom Default Parameters

**Region-Specific Defaults:**
```cpp
// In config.h
#ifdef REGION_EU
    #define LORA_FREQUENCY 868.0    // EU ISM band
    #define LORA_POWER 14           // EU power limit
#elif defined(REGION_US)
    #define LORA_FREQUENCY 915.0    // US ISM band
    #define LORA_POWER 20           // US power limit
#endif
```

**Application-Specific Profiles:**
```cpp
// In main.cpp
void loadApplicationProfile() {
#ifdef APPLICATION_APRS
    // APRS-specific settings
    loraRadio.setFrequency(433.775);
    loraRadio.setBandwidth(125.0);
    loraRadio.setSpreadingFactor(12);
    
#elif defined(APPLICATION_LORAWAN)
    // LoRaWAN-specific settings
    loraRadio.setFrequency(868.1);
    loraRadio.setBandwidth(125.0);
    loraRadio.setSpreadingFactor(7);
    
#endif
}
```

## Contributing to the Project

### Development Workflow

**1. Fork and Clone:**
```bash
git clone https://github.com/yourusername/LoRaTNCX.git
cd LoRaTNCX
git checkout -b feature/your-feature-name
```

**2. Development Process:**
```bash
# Create feature branch
git checkout -b feature/new-radio-parameter

# Make changes
# ... edit files ...

# Test changes
platformio run --environment heltec_wifi_lora_32_V4
platformio run --target upload --environment heltec_wifi_lora_32_V4
python3 test/kiss_test.py

# Commit changes
git add .
git commit -m "Add new radio parameter support

- Add HW_SET_NEW_PARAMETER command
- Implement parameter validation
- Update web interface
- Add unit tests"
```

**3. Pull Request Process:**
```bash
# Push to your fork
git push origin feature/new-radio-parameter

# Create pull request on GitHub
# - Describe the changes
# - Reference any related issues
# - Include testing instructions
# - Update documentation if needed
```

### Code Standards

**C++ Coding Style:**
```cpp
// Use consistent naming
class MyClass {
public:
    void doSomething();           // camelCase for methods
private:
    uint8_t myVariable;           // camelCase for variables
    static const uint8_t MY_CONSTANT = 42;  // UPPER_SNAKE_CASE for constants
};

// Use meaningful names
void setRadioFrequency(float frequencyMHz);  // Clear parameter names
bool isValidFrequency(float freq);           // Boolean methods start with 'is'

// Error handling
if (!radio.configure()) {
    LOG_ERRORLN("Failed to configure radio");
    return false;
}
```

**Documentation Standards:**
```cpp
/**
 * Configure the LoRa radio with specified parameters
 * @param frequency Center frequency in MHz (150.0 - 960.0)
 * @param bandwidth Bandwidth in kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500)
 * @param spreadingFactor Spreading factor (7-12)
 * @param codingRate Coding rate (5-8 for 4/5 through 4/8)
 * @param power Transmit power in dBm (-9 to 22)
 * @return true if configuration successful, false otherwise
 */
bool LoRaRadio::configure(float frequency, float bandwidth, 
                         uint8_t spreadingFactor, uint8_t codingRate, 
                         uint8_t power);
```

### Testing Requirements

**Unit Tests:**
- Test all new functions and methods
- Test parameter validation (bounds checking)
- Test error conditions and edge cases
- Mock hardware interfaces where possible

**Integration Tests:**
- Test with actual hardware
- Verify KISS protocol compliance
- Test web interface functionality
- Validate configuration persistence

**Performance Tests:**
- Measure power consumption impact
- Test memory usage
- Verify timing requirements
- Check for memory leaks

**Compatibility Tests:**
- Test on both V3 and V4 boards
- Verify backward compatibility
- Test with existing KISS applications
- Validate web interface on different browsers

### Documentation Updates

**Code Documentation:**
- Update header file comments
- Document new KISS commands
- Explain configuration parameters
- Provide usage examples

**User Documentation:**
- Update relevant manual sections
- Add configuration examples
- Document new features
- Update troubleshooting guides

**API Documentation:**
- Update REST API reference
- Document new endpoints
- Provide example requests/responses
- Update OpenAPI specifications

### Issue Reporting

**Bug Reports:**
- Use the issue template
- Include firmware version and board type
- Provide serial logs and error messages
- Describe steps to reproduce
- Include configuration details

**Feature Requests:**
- Clearly describe the desired functionality
- Explain the use case and benefits
- Consider implementation complexity
- Suggest design approaches if possible

### Release Process

**Version Numbering:**
- Follow semantic versioning (MAJOR.MINOR.PATCH)
- Increment MAJOR for breaking changes
- Increment MINOR for new features
- Increment PATCH for bug fixes

**Release Checklist:**
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Changelog updated
- [ ] Compatibility verified
- [ ] Performance validated
- [ ] Security review completed

## Custom Firmware Builds

### Advanced PlatformIO Configuration

**Custom Build Environments:**
```ini
# platformio.ini - Advanced configuration
[env:debug_build]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -DLOG_LEVEL=4                    # Maximum debug logging
    -DCORE_DEBUG_LEVEL=5            # ESP32 core debugging
    -DDEBUG_ESP_PORT=Serial         # Enable serial debugging
    -DDEBUG_ESP_CORE                # Core debug output
    -DDEBUG_ESP_WIFI                # WiFi debug output
    -DDEBUG_ESP_HTTP_CLIENT         # HTTP client debugging
    -DDEBUG_ESP_HTTP_SERVER         # HTTP server debugging

[env:minimal_build]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -DDISABLE_WEB_INTERFACE         # Remove web server
    -DDISABLE_TCP_KISS             # Remove TCP server
    -DDISABLE_DISPLAY              # Remove OLED support
    -DDISABLE_GNSS                 # Remove GNSS support
    # Reduces binary size for specific applications

[env:performance_build]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -O3                             # Maximum optimization
    -flto                           # Link-time optimization
    -DCONFIG_FREERTOS_HZ=1000      # Higher FreeRTOS tick rate
    -DCONFIG_ESP32_DEFAULT_CPU_FREQ_240  # Overclock to 240MHz
```

**Custom Partition Tables:**
```ini
# For larger applications or additional features
board_build.partitions = custom_partitions.csv

# custom_partitions.csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1E0000,
app1,     app,  ota_1,   0x1F0000,0x1E0000,
spiffs,   data, spiffs,  0x3D0000,0x30000,
```

### Library Management

**Adding Custom Libraries:**
```ini
# In platformio.ini
lib_deps = 
    ${env.lib_deps}
    https://github.com/username/CustomLibrary.git#v1.0.0
    file:///path/to/local/library

# Or in lib/ directory
# Copy library files to lib/CustomLibrary/
# PlatformIO will automatically find and compile
```

**Library Optimization:**
```ini
# Reduce binary size by excluding unused features
build_flags = 
    ${env.build_flags}
    -DARDUINOJSON_ENABLE_COMMENTS=0    # Disable JSON comments
    -DARDUINOJSON_ENABLE_NAN=0         # Disable NaN handling
    -DARDUINOJSON_ENABLE_INFINITY=0    # Disable infinity handling
```

### Firmware Customization Scripts

**Automated Build Script:**
```bash
#!/bin/bash
# build_custom.sh - Custom firmware build script

set -e

# Configuration
BOARD=${1:-heltec_wifi_lora_32_V4}
CUSTOM_FLAGS=${2:-}

echo "Building custom firmware for $BOARD"

# Clean previous build
platformio run --environment $BOARD --target clean

# Build with custom flags
platformio run --environment $BOARD \
    --build_flags "$CUSTOM_FLAGS"

# Upload if requested
if [ "$3" = "upload" ]; then
    platformio run --environment $BOARD --target upload
fi

echo "Build complete"
```

**Usage:**
```bash
# Standard build
./build_custom.sh heltec_wifi_lora_32_V4

# Debug build
./build_custom.sh heltec_wifi_lora_32_V4 "-DLOG_LEVEL=4 -DDEBUG_ESP_CORE"

# Build and upload
./build_custom.sh heltec_wifi_lora_32_V4 "" upload
```

### OTA Updates

**Over-The-Air Updates:**
```cpp
// In main.cpp - Add OTA support
#include <ArduinoOTA.h>

void setupOTA() {
    ArduinoOTA.setHostname("LoRaTNCX");
    ArduinoOTA.setPassword("your-ota-password");
    
    ArduinoOTA.onStart([]() {
        LOG_INFOLN("OTA update started");
    });
    
    ArduinoOTA.onEnd([]() {
        LOG_INFOLN("OTA update finished");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        LOG_INFO("OTA progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        LOG_ERROR("OTA error[%u]: ", error);
        // Handle error
    });
    
    ArduinoOTA.begin();
}

void loop() {
    // ... existing loop code ...
    ArduinoOTA.handle();
}
```

**OTA Build Configuration:**
```ini
# In platformio.ini
[env:ota_build]
board = heltec_wifi_lora_32_V4
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V4
    -DENABLE_OTA_UPDATES
upload_protocol = espota
upload_port = 192.168.4.1  # IP address when in AP mode
upload_flags = 
    --auth=your-ota-password
```

### Performance Profiling

**Memory Usage Analysis:**
```bash
# Check binary size
platformio run --environment heltec_wifi_lora_32_V4 --target size

# Monitor heap usage at runtime
void printMemoryInfo() {
    LOG_INFO("Free heap: %d bytes\n", ESP.getFreeHeap());
    LOG_INFO("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
    LOG_INFO("Max alloc heap: %d bytes\n", ESP.getMaxAllocHeap());
}
```

**CPU Usage Monitoring:**
```cpp
// Add to main loop for profiling
static unsigned long lastProfileTime = 0;
if (millis() - lastProfileTime > 5000) {  // Every 5 seconds
    LOG_INFO("Task high water marks:\n");
    // Print FreeRTOS task stack usage
    lastProfileTime = millis();
}
```

**Power Consumption Profiling:**
```cpp
// Measure current consumption
void measurePowerConsumption() {
    static unsigned long lastMeasure = 0;
    if (millis() - lastMeasure > 1000) {
        float voltage = getBatteryVoltage();
        // Estimate current based on voltage drop and known resistance
        // Log power consumption data
        lastMeasure = millis();
    }
}
```

---

**Next:** [Section 17: Appendices](17-appendices.md) | **Previous:** [Section 15: Advanced Topics](15-advanced-topics.md) | **Main Index:** [User Manual](../user_manual.md)