#include <Arduino.h>
#include <SPIFFS.h>
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

// Temporary debug mode
// #define DEBUG_MODE 1

#ifdef DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Global objects
KISSProtocol kiss;
LoRaRadio loraRadio;
ConfigManager configManager;
WiFiManager wifiManager;
TNCWebServer webServer(&wifiManager, &loraRadio, &configManager);
TCPKISSServer tcpKissServer;
GNSSModule gnssModule;
NMEAServer nmeaServer;

// Buffers
uint8_t rxBuffer[LORA_BUFFER_SIZE];

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("\n=== LoRaTNCX Debug Mode ===");
    
    // Initialize SPIFFS for web server
    DEBUG_PRINTLN("Initializing SPIFFS...");
    if (!SPIFFS.begin(true)) {
        DEBUG_PRINTLN("SPIFFS mount failed!");
    } else {
        DEBUG_PRINTLN("SPIFFS mounted successfully");
    }
    
    // Initialize board-specific pins
    DEBUG_PRINTLN("Initializing board pins...");
    initializeBoardPins();
    
    // Board type check - halt if unknown
    if (BOARD_TYPE == BOARD_UNKNOWN) {
        DEBUG_PRINTLN("FATAL: Unknown board type!");
        while (1) {
            delay(1000);
        }
    }
    DEBUG_PRINTLN("Board initialized");
    
    // Initialize display
    DEBUG_PRINTLN("Initializing display...");
    displayManager.begin();
    DEBUG_PRINTLN("Display initialized");
    
    // Setup user button interrupt
    pinMode(PIN_USER_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_USER_BUTTON), buttonInterruptHandler, FALLING);
    
    // Setup PA control (V4 only)
    setupPAControl();
    
    // Initialize configuration manager
    DEBUG_PRINTLN("Initializing config manager...");
    configManager.begin();
    
    // Initialize LoRa radio first - halt if failed
    DEBUG_PRINTLN("Initializing radio...");
    int radioState = loraRadio.beginWithState();
    if (radioState != RADIOLIB_ERR_NONE) {
        DEBUG_PRINT("FATAL: Radio init failed with code: ");
        DEBUG_PRINTLN(radioState);
        while (1) {
            delay(1000);
        }
    }
    DEBUG_PRINTLN("Radio initialized!");
    
    // Now try to load and apply saved configuration
    LoRaConfig savedConfig;
    if (configManager.loadConfig(savedConfig)) {
        DEBUG_PRINTLN("Applying saved config...");
        loraRadio.applyConfig(savedConfig);
        loraRadio.reconfigure();  // Reconfigure radio with saved settings
    } else {
        DEBUG_PRINTLN("Using default config");
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
    
    // Initialize WiFi manager
    DEBUG_PRINTLN("Initializing WiFi manager...");
    if (wifiManager.begin()) {
        DEBUG_PRINTLN("WiFi manager initialized");
        
        // Switch to WiFi startup screen
        displayManager.setScreen(SCREEN_WIFI_STARTUP);
        displayManager.setWiFiStartupMessage("Starting WiFi...");
        displayManager.update();
        
        // Start WiFi with saved/default configuration
        if (wifiManager.start()) {
            DEBUG_PRINTLN("WiFi started");
            
            // Wait for WiFi to be ready (either AP started or STA connected)
            // with a timeout of 30 seconds
            unsigned long wifiStartTime = millis();
            const unsigned long WIFI_TIMEOUT = 30000;  // 30 seconds
            
            while (!wifiManager.isReady() && (millis() - wifiStartTime < WIFI_TIMEOUT)) {
                wifiManager.update();
                displayManager.setWiFiStartupMessage(wifiManager.getStatusMessage());
                displayManager.update();
                delay(100);
            }
            
            if (wifiManager.isReady()) {
                DEBUG_PRINTLN("WiFi ready!");
                
                // Show WiFi info on display
                if (wifiManager.isAPActive()) {
                    DEBUG_PRINT("AP IP: ");
                    DEBUG_PRINTLN(wifiManager.getAPIPAddress());
                    displayManager.setWiFiStartupMessage("AP: " + wifiManager.getAPIPAddress());
                }
                if (wifiManager.isConnected()) {
                    DEBUG_PRINT("STA IP: ");
                    DEBUG_PRINTLN(wifiManager.getIPAddress());
                    displayManager.setWiFiStartupMessage("Connected: " + wifiManager.getIPAddress());
                }
                displayManager.update();
                delay(2000);  // Show WiFi status for 2 seconds
                
                // Start web server
                DEBUG_PRINTLN("Starting web server...");
                webServer.setGNSS(&gnssModule, &nmeaServer);  // Set GNSS references
                if (webServer.begin()) {
                    DEBUG_PRINTLN("Web server started on port 80");
                    DEBUG_PRINTLN("Access via: http://loratncx.local");
                } else {
                    DEBUG_PRINTLN("Failed to start web server");
                }
                
                // Start TCP KISS server if enabled
                WiFiConfig wifiConfig;
                wifiManager.getCurrentConfig(wifiConfig);
                if (wifiConfig.tcp_kiss_enabled) {
                    DEBUG_PRINT("Starting TCP KISS server on port ");
                    DEBUG_PRINTLN(wifiConfig.tcp_kiss_port);
                    if (tcpKissServer.begin(wifiConfig.tcp_kiss_port)) {
                        DEBUG_PRINTLN("TCP KISS server started");
                    } else {
                        DEBUG_PRINTLN("Failed to start TCP KISS server");
                    }
                }
                
                // Initialize GNSS if enabled
                GNSSConfig gnssConfig;
                if (configManager.loadGNSSConfig(gnssConfig)) {
                    DEBUG_PRINTLN("Loaded GNSS config from NVS");
                } else {
                    DEBUG_PRINTLN("No saved GNSS config, using defaults");
                    configManager.resetGNSSToDefaults(gnssConfig);
                }
                
                if (gnssConfig.enabled && gnssConfig.pinRX >= 0 && gnssConfig.pinTX >= 0) {
                    DEBUG_PRINTLN("Initializing GNSS module...");
                    if (gnssModule.begin(gnssConfig.pinRX, gnssConfig.pinTX, 
                                        gnssConfig.pinCtrl, gnssConfig.pinWake,
                                        gnssConfig.pinPPS, gnssConfig.pinRST,
                                        gnssConfig.baudRate)) {
                        DEBUG_PRINTLN("GNSS module initialized");
                        
                        // Start NMEA TCP server
                        DEBUG_PRINT("Starting NMEA server on port ");
                        DEBUG_PRINTLN(gnssConfig.tcpPort);
                        if (nmeaServer.begin(gnssConfig.tcpPort)) {
                            DEBUG_PRINTLN("NMEA server started");
                        } else {
                            DEBUG_PRINTLN("Failed to start NMEA server");
                        }
                    } else {
                        DEBUG_PRINTLN("Failed to initialize GNSS module");
                    }
                } else {
                    DEBUG_PRINTLN("GNSS disabled or not configured");
                }
            } else {
                DEBUG_PRINTLN("WiFi timeout - continuing anyway");
                displayManager.setWiFiStartupMessage("WiFi Timeout");
                displayManager.update();
                delay(2000);
            }
        } else {
            DEBUG_PRINTLN("WiFi start failed or disabled");
            displayManager.setWiFiStartupMessage("WiFi Disabled");
            displayManager.update();
            delay(2000);
        }
    } else {
        DEBUG_PRINTLN("WiFi manager init failed");
    }
    
    DEBUG_PRINTLN("Entering KISS mode (debug enabled)\n");
    // TNC is now ready and enters KISS mode (silent operation)
}

void handleHardwareQuery(uint8_t* frame, size_t frameLen) {
    if (frameLen < 2) return;  // Need at least command and subcommand
    
    uint8_t subCmd = frame[1];
    
    switch (subCmd) {
        case HW_QUERY_CONFIG:
            // Send current radio config
            // Format: HW_QUERY_CONFIG, freq(4 bytes float), bw(4 bytes float), 
            //         sf(1 byte), cr(1 byte), pwr(1 byte signed), sync(2 bytes)
            {
                uint8_t config[14];
                config[0] = HW_QUERY_CONFIG;
                
                // Frequency (4 bytes, little-endian float)
                float freq = loraRadio.getFrequency();
                memcpy(&config[1], &freq, sizeof(float));
                
                // Bandwidth (4 bytes, little-endian float)
                float bw = loraRadio.getBandwidth();
                memcpy(&config[5], &bw, sizeof(float));
                
                // Spreading factor (1 byte)
                config[9] = loraRadio.getSpreadingFactor();
                
                // Coding rate (1 byte)
                config[10] = loraRadio.getCodingRate();
                
                // Output power (1 byte, signed)
                int8_t pwr = loraRadio.getOutputPower();
                memcpy(&config[11], &pwr, sizeof(int8_t));
                
                // Sync word (2 bytes, little-endian)
                uint16_t sw = loraRadio.getSyncWord();
                memcpy(&config[12], &sw, sizeof(uint16_t));
                
                kiss.sendCommand(CMD_GETHARDWARE, config, 14);
            }
            break;
            
        case HW_QUERY_BATTERY:
            // Send battery voltage
            // Format: HW_QUERY_BATTERY, voltage(4 bytes float)
            {
                uint8_t battData[5];
                battData[0] = HW_QUERY_BATTERY;
                
                float battVoltage = readBatteryVoltage();
                memcpy(&battData[1], &battVoltage, sizeof(float));
                
                kiss.sendCommand(CMD_GETHARDWARE, battData, 5);
            }
            break;
            
        case HW_QUERY_BOARD:
            // Send board information
            // Format: HW_QUERY_BOARD, board_type(1 byte), board_name(string)
            {
                const char* boardName = BOARD_NAME;
                size_t nameLen = strlen(boardName);
                uint8_t boardData[32];  // Max 32 bytes for board info
                
                boardData[0] = HW_QUERY_BOARD;
                boardData[1] = (uint8_t)BOARD_TYPE;
                memcpy(&boardData[2], boardName, nameLen);
                
                kiss.sendCommand(CMD_GETHARDWARE, boardData, 2 + nameLen);
            }
            break;
            
        case HW_QUERY_ALL:
            // Send all information: config, battery, and board
            // We'll send them as separate responses for simplicity
            {
                // Radio config
                uint8_t config[14];
                config[0] = HW_QUERY_CONFIG;
                float freq = loraRadio.getFrequency();
                memcpy(&config[1], &freq, sizeof(float));
                float bw = loraRadio.getBandwidth();
                memcpy(&config[5], &bw, sizeof(float));
                config[9] = loraRadio.getSpreadingFactor();
                config[10] = loraRadio.getCodingRate();
                int8_t pwr = loraRadio.getOutputPower();
                memcpy(&config[11], &pwr, sizeof(int8_t));
                uint16_t sw = loraRadio.getSyncWord();
                memcpy(&config[12], &sw, sizeof(uint16_t));
                kiss.sendCommand(CMD_GETHARDWARE, config, 14);
                
                // Battery voltage
                uint8_t battData[5];
                battData[0] = HW_QUERY_BATTERY;
                float battVoltage = readBatteryVoltage();
                memcpy(&battData[1], &battVoltage, sizeof(float));
                kiss.sendCommand(CMD_GETHARDWARE, battData, 5);
                
                // Board info
                const char* boardName = BOARD_NAME;
                size_t nameLen = strlen(boardName);
                uint8_t boardData[32];
                boardData[0] = HW_QUERY_BOARD;
                boardData[1] = (uint8_t)BOARD_TYPE;
                memcpy(&boardData[2], boardName, nameLen);
                kiss.sendCommand(CMD_GETHARDWARE, boardData, 2 + nameLen);
            }
            break;
    }
}

void handleHardwareConfig(uint8_t* frame, size_t frameLen) {
    if (frameLen < 2) return;  // Need at least command and subcommand
    
    uint8_t subCmd = frame[1];
    bool needsReconfig = false;
    
    switch (subCmd) {
        case HW_SET_FREQUENCY:
            if (frameLen >= 6) {  // Command + subcommand + 4 bytes for float
                float freq;
                memcpy(&freq, &frame[2], sizeof(float));
                if (freq >= 150.0 && freq <= 960.0) {  // SX1262 range
                    loraRadio.setFrequency(freq);
                    needsReconfig = true;
                }
            }
            break;
            
        case HW_SET_BANDWIDTH:
            if (frameLen >= 3) {
                uint8_t bwIdx = frame[2];
                float bw = 125.0;
                if (bwIdx == 0) bw = 125.0;
                else if (bwIdx == 1) bw = 250.0;
                else if (bwIdx == 2) bw = 500.0;
                loraRadio.setBandwidth(bw);
                needsReconfig = true;
            }
            break;
            
        case HW_SET_SPREADING:
            if (frameLen >= 3) {
                uint8_t sf = frame[2];
                if (sf >= 7 && sf <= 12) {
                    loraRadio.setSpreadingFactor(sf);
                    needsReconfig = true;
                }
            }
            break;
            
        case HW_SET_CODINGRATE:
            if (frameLen >= 3) {
                uint8_t cr = frame[2];
                if (cr >= 5 && cr <= 8) {
                    loraRadio.setCodingRate(cr);
                    needsReconfig = true;
                }
            }
            break;
            
        case HW_SET_POWER:
            if (frameLen >= 3) {
                int8_t power = (int8_t)frame[2];
                if (power >= -9 && power <= 22) {  // SX1262 range
                    loraRadio.setOutputPower(power);
                    needsReconfig = true;
                }
            }
            break;
            
        case HW_SET_SYNCWORD:
            if (frameLen >= 4) {  // Command + subcommand + 2 bytes for sync word
                uint16_t sw;
                memcpy(&sw, &frame[2], sizeof(uint16_t));
                loraRadio.setSyncWord(sw);
                needsReconfig = true;
            }
            break;
            
        case HW_GET_CONFIG:
            // Send current config back via KISS
            // Format: HW_GET_CONFIG, freq(4 bytes float), bw(4 bytes float), 
            //         sf(1 byte), cr(1 byte), pwr(1 byte signed), sync(2 bytes)
            {
                uint8_t config[14];
                config[0] = HW_GET_CONFIG;
                
                // Frequency (4 bytes, little-endian float)
                float freq = loraRadio.getFrequency();
                memcpy(&config[1], &freq, sizeof(float));
                
                // Bandwidth (4 bytes, little-endian float)
                float bw = loraRadio.getBandwidth();
                memcpy(&config[5], &bw, sizeof(float));
                
                // Spreading factor (1 byte)
                config[9] = loraRadio.getSpreadingFactor();
                
                // Coding rate (1 byte)
                config[10] = loraRadio.getCodingRate();
                
                // Output power (1 byte, signed)
                int8_t pwr = loraRadio.getOutputPower();
                memcpy(&config[11], &pwr, sizeof(int8_t));
                
                // Sync word (2 bytes, little-endian)
                uint16_t sw = loraRadio.getSyncWord();
                memcpy(&config[12], &sw, sizeof(uint16_t));
                
                kiss.sendCommand(CMD_SETHARDWARE, config, 14);
            }
            break;
            
        case HW_SAVE_CONFIG:
            // Save current configuration to NVS
            {
                LoRaConfig currentConfig;
                loraRadio.getCurrentConfig(currentConfig);
                configManager.saveConfig(currentConfig);
            }
            break;
            
        case HW_RESET_CONFIG:
            loraRadio.setFrequency(LORA_FREQUENCY);
            loraRadio.setBandwidth(LORA_BANDWIDTH);
            loraRadio.setSpreadingFactor(LORA_SPREADING);
            loraRadio.setCodingRate(LORA_CODINGRATE);
            loraRadio.setOutputPower(LORA_POWER);
            loraRadio.setSyncWord(LORA_SYNCWORD);
            needsReconfig = true;
            // Also clear saved config from NVS
            configManager.clearConfig();
            break;
    }
    
    if (needsReconfig) {
        loraRadio.reconfigure();
    }
}

void loop() {
    // Update WiFi manager
    wifiManager.update();
    
    // Update web server
    webServer.update();
    
    // Update TCP KISS server
    tcpKissServer.update();
    
    // Update GNSS module
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
    
    // Handle button press
    if (buttonPressed) {
        buttonPressed = false;
        displayManager.handleButtonPress();
    }
    
    // Update display
    displayManager.update();
    
    // Update battery voltage periodically (every 10 seconds when not on boot screen)
    static uint32_t lastBatteryUpdate = 0;
    if (!displayManager.isBootScreenActive() && millis() - lastBatteryUpdate >= 10000) {
        float battVoltage = readBatteryVoltage();
        displayManager.setBatteryVoltage(battVoltage);
        lastBatteryUpdate = millis();
    }
    
    // Update WiFi status periodically (every 5 seconds when not on boot screen)
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
    
    // Update GNSS status periodically (every 2 seconds when not on boot screen)
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
    
    // Process incoming serial data (KISS frames from host)
    while (Serial.available() > 0) {
        uint8_t byte = Serial.read();
        kiss.processSerialByte(byte);
    }
    
    // Check if we have a complete KISS frame
    if (kiss.hasFrame()) {
        uint8_t* frame = kiss.getFrame();
        size_t frameLen = kiss.getFrameLength();
        
        if (frameLen > 0) {
            // First byte is the command
            uint8_t cmd = frame[0] & 0x0F;
            
            if (cmd == CMD_SETHARDWARE && frameLen > 1) {
                // Handle hardware configuration
                handleHardwareConfig(frame, frameLen);
                
                // Update display with new config
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
                // Handle hardware query
                handleHardwareQuery(frame, frameLen);
            }
            else if (cmd == CMD_DATA && frameLen > 1) {
                // Transmit data frame (skip command byte at frame[0])
                if (frameLen - 1 <= LORA_BUFFER_SIZE) {
                    loraRadio.transmit(frame + 1, frameLen - 1);
                }
            }
        }
        
        kiss.clearFrame();
    }
    
    // Check for received LoRa packets
    size_t rxLen = 0;
    if (loraRadio.receive(rxBuffer, &rxLen)) {
        if (rxLen > 0) {
            // Send received packet to host via KISS (serial)
            kiss.sendFrame(rxBuffer, rxLen);
            
            // Also send to TCP KISS clients if any are connected
            if (tcpKissServer.hasClients()) {
                // Build KISS frame for TCP clients
                uint8_t tcpFrame[LORA_BUFFER_SIZE + 10];  // Extra space for KISS framing
                size_t tcpFrameLen = 0;
                
                // FEND
                tcpFrame[tcpFrameLen++] = FEND;
                // Command (port 0, data frame)
                tcpFrame[tcpFrameLen++] = CMD_DATA;
                
                // Escape and add data
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
    
    // Small delay to prevent tight looping
    delay(1);
}
