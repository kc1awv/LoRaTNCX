#include <Arduino.h>
#include <SPIFFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"
#include "board_config.h"
#include "config_manager.h"
#include "kiss.h"
#include "radio.h"
#include "display.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "tcp_kiss.h"
#include "gnss.h"
#include "nmea_server.h"

// Battery monitoring globals (from board_config.cpp)
extern bool battery_ready;
extern BatteryChargeState battery_charge_state;
extern float battery_voltage;
extern float battery_percent;

// Thread-safe button event queue (declared in display.cpp)
extern QueueHandle_t buttonEventQueue;

// Logging configuration
#define LOG_LEVEL_NONE    0  // No logging
#define LOG_LEVEL_ERROR   1  // Error messages only
#define LOG_LEVEL_WARN    2  // Warnings and errors
#define LOG_LEVEL_INFO    3  // Info, warnings, and errors
#define LOG_LEVEL_DEBUG   4  // Debug, info, warnings, and errors

// Set logging level (can be changed at compile time or runtime)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Logging macros
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(x) Serial.print(x)
#define LOG_DEBUGLN(x) Serial.println(x)
#else
#define LOG_DEBUG(x)
#define LOG_DEBUGLN(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(x) Serial.print(x)
#define LOG_INFOLN(x) Serial.println(x)
#else
#define LOG_INFO(x)
#define LOG_INFOLN(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(x) Serial.print(x)
#define LOG_WARNLN(x) Serial.println(x)
#else
#define LOG_WARN(x)
#define LOG_WARNLN(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(x) Serial.print(x)
#define LOG_ERRORLN(x) Serial.println(x)
#else
#define LOG_ERROR(x)
#define LOG_ERRORLN(x)
#endif

// Global service instances (encapsulated access)
static KISSProtocol& getKISSProtocol() {
    static KISSProtocol instance;
    return instance;
}

// Forward declarations for thread-safe button handling
extern QueueHandle_t buttonEventQueue;
void initializeButtonQueue();

static LoRaRadio& getLoRaRadio() {
    static LoRaRadio instance;
    return instance;
}

static ConfigManager& getConfigManager() {
    static ConfigManager instance;
    return instance;
}

static WiFiManager& getWiFiManager() {
    static WiFiManager instance;
    return instance;
}

static GNSSModule& getGNSSModule() {
    static GNSSModule instance;
    return instance;
}

static NMEAServer& getNMEAServer() {
    static NMEAServer instance;
    return instance;
}

// Global references for backward compatibility and cross-dependencies
KISSProtocol& kiss = getKISSProtocol();
LoRaRadio& loraRadio = getLoRaRadio();
ConfigManager& configManager = getConfigManager();
WiFiManager& wifiManager = getWiFiManager();
GNSSModule& gnssModule = getGNSSModule();
NMEAServer& nmeaServer = getNMEAServer();

// Web server needs references to other services
static TNCWebServer& getWebServer() {
    static TNCWebServer instance(&getWiFiManager(), &getLoRaRadio(), &getConfigManager());
    return instance;
}

static TCPKISSServer& getTCPKISSServer() {
    static TCPKISSServer instance;
    return instance;
}

// Global references for web server and TCP KISS server
TNCWebServer& webServer = getWebServer();
TCPKISSServer& tcpKissServer = getTCPKISSServer();

// Buffers
uint8_t rxBuffer[LORA_BUFFER_SIZE];

// Error codes for initialization
enum InitError {
    INIT_SUCCESS = 0,
    INIT_BOARD_UNKNOWN = 1,
    INIT_RADIO_FAILED = 2,
    INIT_SPIFFS_FAILED = 3,
    INIT_CONFIG_FAILED = 4,
    INIT_WIFI_FAILED = 5,
    INIT_GNSS_FAILED = 6
};

// Helper functions for setup initialization
InitError initializeSerial() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SERIAL_INIT_DELAY);
    LOG_INFOLN("\n=== LoRaTNCX Starting ===");
    return INIT_SUCCESS;
}

InitError initializeFileSystem() {
    LOG_INFOLN("Initializing SPIFFS...");
    if (!SPIFFS.begin(true)) {
        LOG_ERRORLN("SPIFFS mount failed - continuing without web server");
        return INIT_SPIFFS_FAILED;
    } else {
        LOG_INFOLN("SPIFFS mounted successfully");
        return INIT_SUCCESS;
    }
}

InitError initializeHardware() {
    // Initialize board-specific pins
    LOG_INFOLN("Initializing board pins...");
    initializeBoardPins();
    
    // Board type check - halt if unknown
    if (BOARD_TYPE == BOARD_UNKNOWN) {
        LOG_ERRORLN("FATAL: Unknown board type - cannot continue");
        return INIT_BOARD_UNKNOWN;
    }
    LOG_INFOLN("Board initialized");
    
    // Initialize display
    LOG_INFOLN("Initializing display...");
    displayManager.begin();
    LOG_INFOLN("Display initialized");
    
    // Initialize button event queue for thread-safe communication
    initializeButtonQueue();
    
    // Setup user button interrupt
    pinMode(PIN_USER_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_USER_BUTTON), buttonInterruptHandler, FALLING);
    
    // Setup PA control (V4 only)
    setupPAControl();
    
    return INIT_SUCCESS;
}

InitError initializeConfiguration() {
    // Initialize configuration manager
    LOG_INFOLN("Initializing config manager...");
    if (!configManager.begin()) {
        LOG_ERRORLN("Config manager initialization failed - using defaults");
        return INIT_CONFIG_FAILED;
    }
    return INIT_SUCCESS;
}

InitError initializeRadio() {
    // Initialize LoRa radio first - halt if failed
    LOG_INFOLN("Initializing radio...");
    int radioState = loraRadio.beginWithState();
    if (radioState != RADIOLIB_ERR_NONE) {
        LOG_ERROR("FATAL: Radio init failed with code: ");
        LOG_ERRORLN(radioState);
        LOG_ERRORLN("Cannot continue without radio functionality");
        return INIT_RADIO_FAILED;
    }
    LOG_INFOLN("Radio initialized!");
    
    // Now try to load and apply saved configuration
    LoRaConfig savedConfig;
    if (configManager.loadConfig(savedConfig)) {
        LOG_INFOLN("Applying saved config...");
        loraRadio.applyConfig(savedConfig);
        loraRadio.reconfigure();  // Reconfigure radio with saved settings
    } else {
        LOG_INFOLN("Using default config");
    }
    
    // Update display with initial radio config
    LoRaConfig currentConfig;
    loraRadio.getCurrentConfig(currentConfig);
    displayManager.setRadioConfig(
        currentConfig.frequency,
        currentConfig.bandwidth,
        currentConfig.spreading,
        currentConfig.codingRate,
        currentConfig.power,
        currentConfig.syncWord
    );
    
    return INIT_SUCCESS;
}

InitError initializeNetworking() {
    // Initialize WiFi manager
    LOG_INFOLN("Initializing WiFi manager...");
    if (wifiManager.begin()) {
        LOG_INFOLN("WiFi manager initialized");
        
        // Switch to WiFi startup screen
        displayManager.setScreen(SCREEN_WIFI_STARTUP);
        displayManager.setWiFiStartupMessage("Starting WiFi...");
        displayManager.update();
        
        // Start WiFi with saved/default configuration
        if (wifiManager.start()) {
            LOG_INFOLN("WiFi started");
            
            // Wait for WiFi to be ready (either AP started or STA connected)
            // with a timeout of 30 seconds
            unsigned long wifiStartTime = millis();
            const unsigned long WIFI_TIMEOUT = WIFI_TIMEOUT_MS;  // 30 seconds
            
            while (!wifiManager.isReady() && (millis() - wifiStartTime < WIFI_TIMEOUT)) {
                wifiManager.update();
                displayManager.setWiFiStartupMessage(wifiManager.getStatusMessage());
                displayManager.update();
                delay(WIFI_INIT_DELAY_MS);
            }
            
            if (wifiManager.isReady()) {
                LOG_INFOLN("WiFi ready!");
                
                // Show WiFi info on display
                if (wifiManager.isAPActive()) {
                    LOG_INFO("AP IP: ");
                    LOG_INFOLN(wifiManager.getAPIPAddress());
                    displayManager.setWiFiStartupMessage("AP: " + wifiManager.getAPIPAddress());
                }
                if (wifiManager.isConnected()) {
                    LOG_INFO("STA IP: ");
                    LOG_INFOLN(wifiManager.getIPAddress());
                    displayManager.setWiFiStartupMessage("Connected: " + wifiManager.getIPAddress());
                }
                displayManager.update();
                delay(WIFI_STATUS_DELAY_MS);  // Show WiFi status for 2 seconds
                
                // Start web server
                LOG_INFOLN("Starting web server...");
                
                // Log memory status for debugging TCP issues
                LOG_INFO("Free heap before web server: ");
                LOG_INFOLN(ESP.getFreeHeap());
                
                webServer.setGNSS(&gnssModule, &nmeaServer);  // Set GNSS references
                if (webServer.begin()) {
                    LOG_INFO("Web server started on port " + String(WEB_SERVER_PORT));
                    LOG_INFOLN("Access via: http://loratncx.local");
                } else {
                    LOG_ERRORLN("Failed to start web server");
                }
                
                // Start TCP KISS server if enabled
                WiFiConfig wifiConfig;
                wifiManager.getCurrentConfig(wifiConfig);
                if (wifiConfig.tcp_kiss_enabled) {
                    LOG_INFO("Starting TCP KISS server on port ");
                    LOG_INFOLN(wifiConfig.tcp_kiss_port);
                    if (tcpKissServer.begin(wifiConfig.tcp_kiss_port)) {
                        LOG_INFOLN("TCP KISS server started");
                    } else {
                        LOG_ERRORLN("Failed to start TCP KISS server");
                    }
                }
            } else {
                LOG_WARNLN("WiFi timeout - continuing anyway");
                displayManager.setWiFiStartupMessage("WiFi Timeout");
                displayManager.update();
                delay(WIFI_STATUS_DELAY_MS);
            }
        } else {
            LOG_WARNLN("WiFi start failed or disabled");
            displayManager.setWiFiStartupMessage("WiFi Disabled");
            displayManager.update();
            delay(WIFI_STATUS_DELAY_MS);
        }
    } else {
        LOG_ERRORLN("WiFi manager init failed - continuing without WiFi");
        return INIT_WIFI_FAILED;
    }
    
    return INIT_SUCCESS;
}

InitError initializeGNSS() {
    // Initialize GNSS if enabled
    GNSSConfig gnssConfig;
    if (configManager.loadGNSSConfig(gnssConfig)) {
        LOG_DEBUGLN("Loaded GNSS config from NVS");
    } else {
        LOG_DEBUGLN("No saved GNSS config, using defaults");
        configManager.resetGNSSToDefaults(gnssConfig);
    }
    
    if (gnssConfig.enabled && gnssConfig.pinRX >= 0 && gnssConfig.pinTX >= 0) {
        LOG_INFOLN("Initializing GNSS module...");
        if (gnssModule.begin(gnssConfig.pinRX, gnssConfig.pinTX, 
                            gnssConfig.pinCtrl, gnssConfig.pinWake,
                            gnssConfig.pinPPS, gnssConfig.pinRST,
                            gnssConfig.baudRate)) {
            LOG_INFOLN("GNSS module initialized");
            
            // Start NMEA TCP server
            LOG_INFO("Starting NMEA server on port ");
            LOG_INFOLN(gnssConfig.tcpPort);
            if (nmeaServer.begin(gnssConfig.tcpPort)) {
                LOG_INFOLN("NMEA server started");
            } else {
                LOG_ERRORLN("Failed to start NMEA server");
                return INIT_GNSS_FAILED;
            }
        } else {
            LOG_WARNLN("Failed to initialize GNSS module - continuing without GNSS");
            return INIT_GNSS_FAILED;
        }
    } else {
        LOG_INFOLN("GNSS disabled or not configured");
    }
    
    return INIT_SUCCESS;
}

void setup() {
    InitError error;
    
    // Initialize serial first for error reporting
    error = initializeSerial();
    if (error != INIT_SUCCESS) {
        // Serial failed - we're blind, but continue anyway
    }
    
    // Initialize file system (non-critical)
    error = initializeFileSystem();
    if (error != INIT_SUCCESS) {
        LOG_WARNLN("Warning: File system initialization failed - web server disabled");
    }
    
    // Initialize hardware (critical)
    error = initializeHardware();
    if (error != INIT_SUCCESS) {
        LOG_ERROR("Critical error during hardware initialization: ");
        LOG_ERRORLN(error);
        LOG_ERRORLN("System cannot continue safely");
        displayManager.setScreen(SCREEN_STATUS);
        displayManager.update();
        // For critical errors, we still halt but with better error indication
        while (1) {
            delay(1000);
        }
    }
    
    // Initialize configuration (non-critical, but affects other systems)
    error = initializeConfiguration();
    if (error != INIT_SUCCESS) {
        LOG_WARNLN("Warning: Configuration system failed - using defaults");
    }
    
    // Initialize radio (critical for TNC functionality)
    error = initializeRadio();
    if (error != INIT_SUCCESS) {
        LOG_ERROR("Critical error during radio initialization: ");
        LOG_ERRORLN(error);
        LOG_ERRORLN("TNC functionality disabled - system will still start for configuration");
        // For radio failure, we continue but with limited functionality
        // Display error on screen
        displayManager.setScreen(SCREEN_STATUS);
        displayManager.update();
    }
    
    // Initialize networking (non-critical)
    error = initializeNetworking();
    if (error != INIT_SUCCESS) {
        LOG_WARNLN("Warning: Networking initialization failed - WiFi features disabled");
    }
    
    // Initialize GNSS (non-critical)
    error = initializeGNSS();
    if (error != INIT_SUCCESS) {
        LOG_WARNLN("Warning: GNSS initialization failed - location features disabled");
    }
    
    LOG_INFOLN("LoRaTNCX ready - entering KISS mode");
    // TNC is now ready and enters KISS mode (silent operation)
}

// Helper functions for building hardware query response data
size_t buildRadioConfigData(uint8_t* buffer, uint8_t command) {
    buffer[0] = command;
    
    // Frequency (4 bytes, little-endian float)
    float freq = loraRadio.getFrequency();
    memcpy(&buffer[1], &freq, sizeof(float));
    
    // Bandwidth (4 bytes, little-endian float)
    float bw = loraRadio.getBandwidth();
    memcpy(&buffer[5], &bw, sizeof(float));
    
    // Spreading factor (1 byte)
    buffer[9] = loraRadio.getSpreadingFactor();
    
    // Coding rate (1 byte)
    buffer[10] = loraRadio.getCodingRate();
    
    // Output power (1 byte, signed)
    int8_t pwr = loraRadio.getOutputPower();
    memcpy(&buffer[11], &pwr, sizeof(int8_t));
    
    // Sync word (2 bytes, little-endian)
    uint16_t sw = loraRadio.getSyncWord();
    memcpy(&buffer[12], &sw, sizeof(uint16_t));
    
    return 14;  // Total size
}

size_t buildBatteryData(uint8_t* buffer) {
    buffer[0] = HW_QUERY_BATTERY;
    
    // Read current battery voltage (also updates sampling)
    float battVoltage = readBatteryVoltage();
    memcpy(&buffer[1], &battVoltage, sizeof(float));
    
    // Include averaged values if ready
    memcpy(&buffer[5], &battery_voltage, sizeof(float));
    memcpy(&buffer[9], &battery_percent, sizeof(float));
    buffer[13] = (uint8_t)battery_charge_state;
    buffer[14] = battery_ready ? 1 : 0;
    
    return 15;  // Total size: cmd(1) + voltage(4) + avg_voltage(4) + percent(4) + state(1) + ready(1)
}

size_t buildBoardData(uint8_t* buffer) {
    buffer[0] = HW_QUERY_BOARD;
    buffer[1] = (uint8_t)BOARD_TYPE;
    
    const char* boardName = BOARD_NAME;
    size_t nameLen = strlen(boardName);
    memcpy(&buffer[2], boardName, nameLen);
    
    return 2 + nameLen;  // Total size
}

size_t buildGNSSData(uint8_t* buffer) {
    buffer[0] = HW_QUERY_GNSS;
    
    bool gnssEnabled = false;
    bool hasFix = false;
    uint8_t satellites = 0;
    float lat = 0.0, lon = 0.0, alt = 0.0;
    
    if (gnssModule.isRunning()) {
        gnssEnabled = true;
        hasFix = gnssModule.hasValidFix();
        satellites = gnssModule.getSatellites();
        lat = gnssModule.getLatitude();
        lon = gnssModule.getLongitude();
        alt = gnssModule.getAltitude();
    }
    
    buffer[1] = gnssEnabled ? 1 : 0;
    buffer[2] = hasFix ? 1 : 0;
    buffer[3] = satellites;
    memcpy(&buffer[4], &lat, sizeof(float));
    memcpy(&buffer[8], &lon, sizeof(float));
    memcpy(&buffer[12], &alt, sizeof(float));
    
    return 16;  // Total size
}

void handleHardwareQuery(uint8_t* frame, size_t frameLen) {
    // Hardware Query Response System:
    // Processes GETHARDWARE commands from KISS protocol clients.
    // Provides real-time status information about radio, battery, board, and GNSS.
    // Each query returns structured data that clients can parse and display.
    //
    // Frame format: [CMD_GETHARDWARE][subcommand]
    // Responses are sent as separate KISS frames with structured data.
    
    // Input validation - prevent crashes from malformed frames
    if (frame == nullptr || frameLen < 2 || frameLen > LORA_BUFFER_SIZE) {
        return;  // Invalid frame or insufficient data
    }
    
    uint8_t subCmd = frame[1];  // Extract query type
    
    // Process different types of hardware queries
    switch (subCmd) {
        case HW_QUERY_CONFIG:
            // Return current LoRa radio configuration
            // Includes frequency, bandwidth, spreading factor, etc.
            {
                uint8_t config[14];
                size_t len = buildRadioConfigData(config, HW_QUERY_CONFIG);
                kiss.sendCommand(CMD_GETHARDWARE, config, len);
            }
            break;
            
        case HW_QUERY_BATTERY:
            // Return battery voltage for power management
            // Critical for portable/mobile applications
            {
                uint8_t battData[5];
                size_t len = buildBatteryData(battData);
                kiss.sendCommand(CMD_GETHARDWARE, battData, len);
            }
            break;
            
        case HW_QUERY_BOARD:
            // Return board identification and capabilities
            // Helps clients understand hardware features available
            {
                uint8_t boardData[32];  // Max 32 bytes for board info
                size_t len = buildBoardData(boardData);
                kiss.sendCommand(CMD_GETHARDWARE, boardData, len);
            }
            break;
            
        case HW_QUERY_GNSS:
            // Return GNSS receiver status and position data
            // Includes fix status, coordinates, satellite count
            {
                uint8_t gnssData[18];
                size_t len = buildGNSSData(gnssData);
                kiss.sendCommand(CMD_GETHARDWARE, gnssData, len);
            }
            break;
            
        case HW_QUERY_ALL:
            // Return all available information in sequence
            // Multiple KISS frames sent - client must handle sequence
            // Useful for initial status synchronization
            {
                // Radio configuration first
                uint8_t config[14];
                size_t len = buildRadioConfigData(config, HW_QUERY_CONFIG);
                kiss.sendCommand(CMD_GETHARDWARE, config, len);
                
                // Battery status
                uint8_t battData[5];
                len = buildBatteryData(battData);
                kiss.sendCommand(CMD_GETHARDWARE, battData, len);
                
                // Board information
                uint8_t boardData[32];
                len = buildBoardData(boardData);
                kiss.sendCommand(CMD_GETHARDWARE, boardData, len);
                
                // GNSS status and position
                uint8_t gnssData[18];
                len = buildGNSSData(gnssData);
                kiss.sendCommand(CMD_GETHARDWARE, gnssData, len);
            }
            break;
            
        default:
            // Invalid query type - ignore silently
            return;
    }
}

static void handleFrequencyConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 6) {  // Command + subcommand + 4 bytes for float
        float freq;
        memcpy(&freq, &frame[2], sizeof(float));
        if (freq >= RADIO_FREQ_MIN && freq <= RADIO_FREQ_MAX) {  // SX1262 range
            loraRadio.setFrequency(freq);
            needsReconfig = true;
        }
    }
}

static void handleBandwidthConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 3) {
        uint8_t bwIdx = frame[2];
        // Validate bandwidth index (0=125kHz, 1=250kHz, 2=500kHz)
        if (bwIdx <= 2) {
            float bw = 125.0;
            if (bwIdx == 1) bw = 250.0;
            else if (bwIdx == 2) bw = 500.0;
            loraRadio.setBandwidth(bw);
            needsReconfig = true;
        }
    }
}

static void handleSpreadingFactorConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 3) {
        uint8_t sf = frame[2];
        if (sf >= RADIO_SF_MIN && sf <= RADIO_SF_MAX) {
            loraRadio.setSpreadingFactor(sf);
            needsReconfig = true;
        }
    }
}

static void handleCodingRateConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 3) {
        uint8_t cr = frame[2];
        if (cr >= RADIO_CR_MIN && cr <= RADIO_CR_MAX) {
            loraRadio.setCodingRate(cr);
            needsReconfig = true;
        }
    }
}

static void handlePowerConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 3) {
        int8_t power = (int8_t)frame[2];
        if (power >= RADIO_POWER_MIN && power <= RADIO_POWER_MAX) {  // SX1262 range
            loraRadio.setOutputPower(power);
            needsReconfig = true;
        }
    }
}

static void handleSyncWordConfig(uint8_t* frame, size_t frameLen, bool& needsReconfig) {
    if (frameLen >= 4) {  // Command + subcommand + 2 bytes for sync word
        uint16_t sw;
        memcpy(&sw, &frame[2], sizeof(uint16_t));
        // Validate sync word range (allow full 16-bit range for SX126x)
        if (sw >= RADIO_SYNCWORD_MIN && sw <= RADIO_SYNCWORD_MAX) {
            loraRadio.setSyncWord(sw);
            needsReconfig = true;
        }
    }
}

static void handleGNSSConfig(uint8_t* frame, size_t frameLen) {
    if (frameLen >= 3) {
        uint8_t enableByte = frame[2];
        // Validate enable value (must be 0 or 1)
        if (enableByte <= 1) {
            bool enable = (enableByte != 0);
            GNSSConfig gnssConfig;
            if (configManager.loadGNSSConfig(gnssConfig)) {
                gnssConfig.enabled = enable;
                configManager.saveGNSSConfig(gnssConfig);
                
                if (enable) {
                    gnssModule.powerOn();
                } else {
                    gnssModule.powerOff();
                }
                
                // Display will be updated in main loop
            }
        }
    }
}

static void handleGetConfig() {
    // Send current config back via KISS
    uint8_t config[14];
    size_t len = buildRadioConfigData(config, HW_GET_CONFIG);
    kiss.sendCommand(CMD_SETHARDWARE, config, len);
}

static void handleSaveConfig() {
    // Save current configuration to NVS
    LoRaConfig currentConfig;
    loraRadio.getCurrentConfig(currentConfig);
    configManager.saveConfig(currentConfig);
}

static void handleResetConfig(bool& needsReconfig) {
    loraRadio.setFrequency(LORA_FREQUENCY);
    loraRadio.setBandwidth(LORA_BANDWIDTH);
    loraRadio.setSpreadingFactor(LORA_SPREADING);
    loraRadio.setCodingRate(LORA_CODINGRATE);
    loraRadio.setOutputPower(LORA_POWER);
    loraRadio.setSyncWord(LORA_SYNCWORD);
    needsReconfig = true;
    // Also clear saved config from NVS
    configManager.clearConfig();
}

void handleHardwareConfig(uint8_t* frame, size_t frameLen) {
    // Hardware Configuration Command Processor:
    // Processes SETHARDWARE commands from KISS protocol clients.
    // These commands allow remote configuration of LoRa radio parameters.
    //
    // Frame format: [CMD_SETHARDWARE][subcommand][parameters...]
    // Subcommands control different radio parameters and system features.
    // Some commands require radio reconfiguration (frequency, modulation parameters).
    
    // Input validation - prevent buffer overflows and null pointer issues
    if (frame == nullptr || frameLen < 2 || frameLen > LORA_BUFFER_SIZE) {
        return;  // Invalid frame or insufficient data
    }
    
    uint8_t subCmd = frame[1];  // Extract subcommand from frame
    bool needsReconfig = false; // Track if radio needs reconfiguration after parameter changes
    
    // Dispatch to appropriate handler based on subcommand
    // Each handler validates parameters and updates radio configuration
    switch (subCmd) {
        case HW_SET_FREQUENCY:
            // Set carrier frequency (433/868/915 MHz bands)
            handleFrequencyConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_BANDWIDTH:
            // Set signal bandwidth (125/250/500 kHz)
            // Affects data rate and receiver sensitivity
            handleBandwidthConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_SPREADING:
            // Set spreading factor (6-12)
            // Higher SF = longer range, lower data rate
            handleSpreadingFactorConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_CODINGRATE:
            // Set forward error correction coding rate (5-8)
            // Higher CR = better error correction, lower data rate
            handleCodingRateConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_POWER:
            // Set transmit power (-9 to +22 dBm)
            // Must stay within regulatory limits for the band
            handlePowerConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_SYNCWORD:
            // Set synchronization word for network identification
            // Allows multiple networks to coexist on same frequency
            handleSyncWordConfig(frame, frameLen, needsReconfig);
            break;
            
        case HW_SET_GNSS_ENABLE:
            // Enable/disable GNSS receiver and NMEA serial passthrough
            // GNSS provides position data for APRS applications
            handleGNSSConfig(frame, frameLen);
            break;
            
        case HW_GET_CONFIG:
            // Request current radio configuration parameters
            // Client can query current settings
            handleGetConfig();
            break;
            
        case HW_SAVE_CONFIG:
            // Save current configuration to non-volatile storage
            // Settings persist across power cycles
            handleSaveConfig();
            break;
            
        case HW_RESET_CONFIG:
            // Reset to factory default configuration
            // Clears any saved custom settings
            handleResetConfig(needsReconfig);
            break;
            
        default:
            // Invalid subcommand - ignore silently for compatibility
            return;
    }
    
    // Apply configuration changes to radio hardware if needed
    // Reconfiguration is required for frequency and modulation parameters
    if (needsReconfig) {
        loraRadio.reconfigure();
    }
}

// Helper functions for periodic updates
static void updateBatteryVoltage() {
    static uint32_t lastBatteryUpdate = 0;
    if (!displayManager.isBootScreenActive() && millis() - lastBatteryUpdate >= 10000) {
        float battVoltage = readBatteryVoltage();
        displayManager.setBatteryVoltage(battVoltage);
        lastBatteryUpdate = millis();
    }
}

static void updateWiFiStatus() {
    static uint32_t lastWiFiUpdate = 0;
    if (!displayManager.isBootScreenActive() && millis() - lastWiFiUpdate >= 5000) {
        bool apActive = wifiManager.isAPActive();
        bool staConnected = wifiManager.isConnected();
        String apIP = wifiManager.getAPIPAddress();
        String staIP = wifiManager.getIPAddress();
        int rssi = wifiManager.getRSSI();
        displayManager.setWiFiStatus(apActive, staConnected, apIP, staIP, rssi);
        lastWiFiUpdate = millis();
    }
}

static void updateGNSSStatus() {
    static uint32_t lastGNSSUpdate = 0;
    if (!displayManager.isBootScreenActive() && millis() - lastGNSSUpdate >= 2000) {
        if (gnssModule.isRunning()) {
            bool hasFix = gnssModule.hasValidFix();
            double lat = hasFix ? gnssModule.getLatitude() : 0.0;
            double lon = hasFix ? gnssModule.getLongitude() : 0.0;
            uint8_t sats = gnssModule.getSatellites();
            uint8_t clients = nmeaServer.getClientCount();
            displayManager.setGNSSStatus(true, hasFix, lat, lon, sats, clients);
        } else {
            displayManager.setGNSSStatus(false, false, 0.0, 0.0, 0, 0);
        }
        lastGNSSUpdate = millis();
    }
}

static void updateGNSSData() {
    static bool gnssSerialPassthroughEnabled = false;
    static uint32_t lastPassthroughConfigCheck = 0;
    static uint32_t lastGNSSDebug = 0;

    // Check passthrough config every 5 seconds instead of every sentence
    if (millis() - lastPassthroughConfigCheck >= 5000) {
        GNSSConfig gnssConfig;
        if (configManager.loadGNSSConfig(gnssConfig)) {
            gnssSerialPassthroughEnabled = gnssConfig.serialPassthrough;
        }
        lastPassthroughConfigCheck = millis();
    }

    if (gnssModule.isRunning()) {
        gnssModule.update();

        // Check if we have an NMEA sentence ready
        if (gnssModule.hasNMEASentence()) {
            const char* sentence = gnssModule.getNMEASentence();
            if (sentence) {
                // Forward to TCP clients if available
                if (nmeaServer.hasClients()) {
                    nmeaServer.sendNMEA(sentence);
                }

                // Forward to USB serial if passthrough is enabled
                // NMEA sentences need CR+LF line ending
                if (gnssSerialPassthroughEnabled) {
                    Serial.print(sentence);
                    Serial.print("\r\n");
                }
            }
            gnssModule.clearNMEASentence();
        }

        // Update NMEA server
        nmeaServer.update();
    }
}

static void processKISSFrames() {
    // Main KISS Protocol Processing Loop:
    // This function bridges serial communication with LoRa radio operation.
    // It processes incoming KISS frames from host applications and dispatches
    // them to appropriate handlers for data transmission or configuration.
    //
    // The system supports three main frame types:
    // 1. DATA frames: User data to be transmitted over LoRa
    // 2. SETHARDWARE frames: Configuration commands for radio parameters
    // 3. GETHARDWARE frames: Status queries for system information
    
    // Process all available serial data into KISS frame buffer
    // Serial data arrives asynchronously, so we process it in chunks
    while (Serial.available() > 0) {
        uint8_t byte = Serial.read();
        kiss.processSerialByte(byte);
    }

    // Check if we have a complete KISS frame ready for processing
    if (kiss.hasFrame()) {
        uint8_t* frame = kiss.getFrame();
        size_t frameLen = kiss.getFrameLength();

        if (frameLen > 0) {
            // Extract command from first byte (lower 4 bits)
            uint8_t cmd = frame[0] & 0x0F;

            if (cmd == CMD_SETHARDWARE && frameLen > 1) {
                // Hardware Configuration Frame:
                // Contains radio parameter settings from client application
                // Updates LoRa radio configuration and refreshes display
                handleHardwareConfig(frame, frameLen);

                // Update OLED display with new radio parameters
                // Ensures user sees current configuration on device
                LoRaConfig currentConfig;
                loraRadio.getCurrentConfig(currentConfig);
                displayManager.setRadioConfig(
                    currentConfig.frequency,
                    currentConfig.bandwidth,
                    currentConfig.spreading,
                    currentConfig.codingRate,
                    currentConfig.power,
                    currentConfig.syncWord
                );
            }
            else if (cmd == CMD_GETHARDWARE && frameLen > 1) {
                // Hardware Query Frame:
                // Client requesting status information (config, battery, GNSS, etc.)
                // Responses sent as separate KISS frames with structured data
                handleHardwareQuery(frame, frameLen);
            }
            else if (cmd == CMD_DATA && frameLen > 1) {
                // Data Transmission Frame:
                // Contains user data payload to be transmitted over LoRa
                // Skip command byte and transmit remaining data
                if (frameLen - 1 <= LORA_BUFFER_SIZE) {
                    loraRadio.transmit(frame + 1, frameLen - 1);
                }
                // Silently drop oversized frames to prevent buffer issues
            }
            // Other command types (TXDELAY, etc.) handled in KISS layer
        }

        // Clear frame buffer for next frame processing
        kiss.clearFrame();
    }
}

static void processReceivedPackets() {
    // LoRa Packet Reception and Distribution:
    // This function handles the reverse data flow - from LoRa radio to clients.
    // Received packets are distributed to both serial KISS clients and TCP KISS clients.
    // The system supports multiple simultaneous client types for maximum flexibility.
    
    // Check for received LoRa packets (non-blocking)
    size_t rxLen = 0;
    if (loraRadio.receive(rxBuffer, &rxLen)) {
        if (rxLen > 0) {
            // Send received packet to serial KISS client (host application)
            // This is the primary interface for most KISS-compatible software
            kiss.sendFrame(rxBuffer, rxLen);

            // Also distribute to TCP KISS clients if any are connected
            // TCP clients get the same data but with full KISS framing
            if (tcpKissServer.hasClients()) {
                // Build complete KISS frame for TCP transmission
                // TCP framing includes FEND delimiters and command byte
                uint8_t tcpFrame[LORA_BUFFER_SIZE + 10];  // Extra space for framing
                size_t tcpFrameLen = 0;

                // Start KISS frame with FEND delimiter
                tcpFrame[tcpFrameLen++] = FEND;
                // Add command byte (data frame, port 0)
                tcpFrame[tcpFrameLen++] = CMD_DATA;

                // Apply KISS byte stuffing to the received data
                // Must escape any FEND or FESC characters in the payload
                for (size_t i = 0; i < rxLen; i++) {
                    if (rxBuffer[i] == FEND) {
                        tcpFrame[tcpFrameLen++] = FESC;
                        tcpFrame[tcpFrameLen++] = TFEND;
                    } else if (rxBuffer[i] == FESC) {
                        tcpFrame[tcpFrameLen++] = FESC;
                        tcpFrame[tcpFrameLen++] = TFESC;
                    } else {
                        tcpFrame[tcpFrameLen++] = rxBuffer[i];
                    }
                }

                // FEND
                tcpFrame[tcpFrameLen++] = FEND;

                // Send to all TCP clients
                tcpKissServer.sendKISSFrame(tcpFrame, tcpFrameLen);
            }
        }
    }
}

void loop() {
    // Update WiFi manager
    wifiManager.update();
    
    // Update web server
    webServer.update();
    
    // Update TCP KISS server
    tcpKissServer.update();
    
    // Update GNSS module and handle NMEA data
    updateGNSSData();
    
    // Handle button press events from queue
    bool buttonEvent;
    while (xQueueReceive(buttonEventQueue, &buttonEvent, 0) == pdTRUE) {
        displayManager.handleButtonPress();
    }
    
    // Update display
    displayManager.update();
    
    // Update periodic status information
    updateBatteryVoltage();
    updateWiFiStatus();
    updateGNSSStatus();
    
    // Process KISS frames from serial
    processKISSFrames();
    
    // Process received LoRa packets
    processReceivedPackets();
    
    // Yield to allow other tasks and prevent watchdog resets
    yield();
}
