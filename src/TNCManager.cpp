/**
 * @file TNCManager.cpp
 * @brief TNC Manager implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "TNCManager.h"

#include "HardwareConfig.h"

#include <cstring>
#include <esp_sleep.h>

namespace
{
constexpr unsigned long BUTTON_DEBOUNCE_MS = 30UL;
constexpr unsigned long POWER_OFF_WARNING_DELAY_MS = 750UL;
constexpr unsigned long POWER_OFF_HOLD_MS = 3000UL;

inline float clamp01(float value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}
}

// Static member definition
TNCManager* TNCManager::instance = nullptr;

TNCManager::TNCManager()
    : configManager(&radio),
      display(),
      batteryMonitor(),
      gnss(),
      wifiManager(),
      webServer(),
      kissTcpServer(KISS_TCP_PORT),
      nmeaTcpServer(NMEA_TCP_PORT)
{
    initialized = false;
    lastStatus = 0;
    serialBuffer = "";
    lastBatteryVoltage = 0.0f;
    lastBatteryPercent = 0;
    lastBatterySample = 0;
    lastPacketRSSI = 0.0f;
    lastPacketSNR = 0.0f;
    lastPacketTimestamp = 0;
    hasRecentPacket = false;
    buttonStableState = false;
    buttonLastReading = false;
    buttonLastChange = 0;
    buttonPressStart = 0;
    buttonLongPressHandled = false;
    powerOffWarningActive = false;
    powerOffProgress = 0.0f;
    powerOffInitiated = false;
    powerOffComplete = false;
    gnssEnabled = false;
    gnssInitialised = false;
    oledEnabled = true;
    kissServerRunning = false;
    nmeaServerRunning = false;

    // Set static instance for callbacks
    instance = this;

    gnss.setNMEACallback([this](const String &line) {
        handleNMEASentence(line);
    });
}

bool TNCManager::begin()
{
    Serial.println("=== LoRaTNCX Initialization ===");

    if (display.begin())
    {
        oledEnabled = true;
        display.showBootScreen();
    }
    else
    {
        oledEnabled = false;
        Serial.println("OLED display not available");
    }

    batteryMonitor.begin();
    lastBatteryVoltage = batteryMonitor.readVoltage();
    lastBatteryPercent = batteryMonitor.computePercentage(lastBatteryVoltage);
    lastBatterySample = millis();

    gnssEnabled = GNSS_ENABLED;
    gnssInitialised = false;

    if (gnssEnabled)
    {
        Serial.println("Initializing GNSS module...");
        if (gnss.begin())
        {
            Serial.println("✓ GNSS module initialized");
            gnssInitialised = true;
        }
        else
        {
            Serial.println("✗ GNSS initialization failed");
            gnssEnabled = false;
        }
    }
    else
    {
        Serial.println("GNSS module disabled");
    }

    pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
    bool initialReading = (digitalRead(USER_BUTTON_PIN) == USER_BUTTON_ACTIVE_STATE);
    buttonStableState = initialReading;
    buttonLastReading = initialReading;
    unsigned long now = millis();
    buttonLastChange = now;
    buttonPressStart = now;
    buttonLongPressHandled = false;
    powerOffWarningActive = false;
    powerOffProgress = 0.0f;
    powerOffInitiated = false;
    powerOffComplete = false;

    initialized = false;
    lastStatus = 0;

    // Initialize KISS protocol
    if (!kiss.begin())
    {
        Serial.println("✗ KISS protocol initialization failed");
        return false;
    }

    // Initialize with default amateur radio configuration (balanced)
    Serial.println("✓ KISS protocol initialized");
    Serial.println("Configuring LoRa radio with amateur radio settings...");

    if (!configManager.setConfiguration(LoRaConfigPreset::BALANCED))
    {
        Serial.println("✗ LoRa radio configuration failed");
        return false;
    }

    Serial.println("✓ LoRa radio configured");

    // Connect radio to command system for hardware integration
    commandSystem.setRadio(&radio);
    commandSystem.setGNSSCallbacks(gnssSetEnabledCallback, gnssGetEnabledCallback);
    commandSystem.setOLEDCallbacks(oledSetEnabledCallback, oledGetEnabledCallback);
    commandSystem.setPeripheralStateDefaults(isGNSSEnabled(), isOLEDEnabled());
    commandSystem.setWiFiCallbacks(wifiAddNetworkCallback, wifiRemoveNetworkCallback,
                                   wifiListNetworksCallback, wifiStatusCallback);
    Serial.println("✓ Command system hardware integration enabled");

    // Initialize web server subsystem
    if (webServer.begin())
    {
        Serial.println("✓ Web server subsystem initialized");
        
        // Setup web server callbacks
        webServer.setCallbacks(
            [this]() { return getSystemStatusForWeb(); },
            [this]() { return getLoRaStatusForWeb(); },
            [this]() { return getWiFiNetworksForWeb(); },
            [this](const String& ssid, const String& password, String& message) {
                bool result = wifiManager.addNetwork(ssid, password);
                message = result ? "Network added successfully" : "Failed to add network";
                return result;
            },
            [this](const String& ssid, String& message) {
                bool result = wifiManager.removeNetwork(ssid);
                message = result ? "Network removed successfully" : "Network not found";
                return result;
            }
        );
        Serial.println("✓ Web server callbacks configured");
    }
    else
    {
        Serial.println("✗ Web server initialization failed");
    }

    if (wifiManager.begin())
    {
        Serial.println("✓ WiFi subsystem initialized");
        
        // Setup WiFi state change callback for web server management
        wifiManager.onStateChange([this](bool ready) {
            onWiFiStateChange(ready);
        });
        Serial.println("✓ WiFi state change callbacks configured");
    }
    else
    {
        Serial.println("✗ WiFi initialization failed");
    }

    // Initialize web server subsystem
    if (webServer.begin())
    {
        Serial.println("✓ Web server subsystem initialized");
        
        // Set up callbacks for web server API endpoints
        webServer.setCallbacks(
            [this]() { return this->getSystemStatusForWeb(); },
            [this]() { return this->getLoRaStatusForWeb(); },
            [this]() { return this->getWiFiNetworksForWeb(); },
            [this](const String& ssid, const String& password, String& message) {
                bool result = this->wifiManager.addNetwork(ssid, password);
                message = result ? "Network added successfully" : "Failed to add network";
                return result;
            },
            [this](const String& ssid, String& message) {
                bool result = this->wifiManager.removeNetwork(ssid);
                message = result ? "Network removed successfully" : "Network not found";
                return result;
            }
        );
        Serial.println("✓ Web server callbacks configured");
    }
    else
    {
        Serial.println("✗ Web server initialization failed");
    }

    Serial.println("\n=== TNC Ready ===");
    Serial.println("Starting in Command mode...");
    Serial.println("Type HELP for available commands");
    Serial.println("Type KISS to enter KISS mode");
    Serial.println(configManager.getConfigStatus());
    Serial.println("===============================");

    initialized = true;

    display.updateStatus(buildDisplayStatus());
    return true;
}

void TNCManager::update()
{
    if (!initialized)
    {
        return;
    }

    if (gnssEnabled && gnssInitialised)
    {
        gnss.update();
        const auto &timeStatus = gnss.getTimeStatus();
        if (timeStatus.valid && !timeStatus.synced)
        {
            if (gnss.syncSystemTime())
            {
                Serial.println("System clock synchronized via GNSS.");
            }
        }
    }

    // Handle incoming serial data (could be commands or KISS)
    handleIncomingSerial();

    // Handle incoming LoRa packets
    handleIncomingRadio();

    // Print periodic status (less frequently in KISS mode)
    if (millis() - lastStatus >= 30000)
    { // Every 30 seconds
        printStatus();
        lastStatus = millis();
    }

    if (millis() - lastBatterySample >= BATTERY_SAMPLE_INTERVAL)
    {
        lastBatteryVoltage = batteryMonitor.readVoltage();
        lastBatteryPercent = batteryMonitor.computePercentage(lastBatteryVoltage);
        lastBatterySample = millis();
    }

    wifiManager.update();
    webServer.update();
    updateTcpServers();
    processKISSTcpClients();

    handleUserButton();

    display.updateStatus(buildDisplayStatus());
}

String TNCManager::getStatus()
{
    String status = "=== TNC Status ===\n";
    status += "Initialized: " + String(initialized ? "Yes" : "No") + "\n";
    status += "Uptime: " + String(millis() / 1000) + " seconds\n\n";
    status += configManager.getConfigStatus() + "\n\n";
    status += kiss.getStatus() + "\n\n";
    status += radio.getStatus();
    return status;
}

bool TNCManager::processConfigurationCommand(const String &command)
{
    return configManager.processConfigCommand(command);
}

void TNCManager::handleIncomingSerial()
{
    // Read available serial data
    static bool lastInputWasCR = false;
    while (Serial.available())
    {
        uint8_t byte = Serial.read();

        bool shouldEcho = (commandSystem.getCurrentMode() != TNCMode::KISS_MODE) && commandSystem.isLocalEchoEnabled();
        if (shouldEcho)
        {
            if (byte == '\r' || byte == '\n')
            {
                if (!(byte == '\n' && lastInputWasCR))
                {
                    if (commandSystem.isLineEndingCREnabled())
                    {
                        Serial.write('\r');
                    }
                    if (commandSystem.isLineEndingLFEnabled())
                    {
                        Serial.write('\n');
                    }
                }
            }
            else if (byte == 8 || byte == 127)
            {
                if (serialBuffer.length() > 0)
                {
                    Serial.write('\b');
                    Serial.write(' ');
                    Serial.write('\b');
                }
            }
            else
            {
                Serial.write(byte);
            }
        }

        lastInputWasCR = (byte == '\r');

        // Handle special initialization sequences before KISS auto-detection
        if (commandSystem.getCurrentMode() != TNCMode::KISS_MODE)
        {
            // Handle common TNC initialization sequences
            static uint8_t initState = 0;
            static uint32_t lastInitByte = 0;

            // Reset init state if too much time has passed
            if (millis() - lastInitByte > 1000)
            {
                initState = 0;
            }
            lastInitByte = millis();

            // State machine for ESC @ k CR sequence
            bool skipNormalProcessing = false;

            switch (initState)
            {
            case 0:               // Waiting for ESC or KISS
                if (byte == 0x1B) // ESC
                {
                    initState = 1;
                    skipNormalProcessing = true;
                }
                else if (byte == 0xC0) // KISS frame start
                {
                    // Auto-switch to KISS mode
                    commandSystem.setMode(TNCMode::KISS_MODE);
                    serialBuffer = "";
                    initState = 0;
                }
                break;

            case 1:               // Got ESC, expecting @
                if (byte == 0x40) // @
                {
                    initState = 2;
                    skipNormalProcessing = true;
                }
                else
                {
                    initState = 0; // Reset on unexpected byte
                }
                break;

            case 2:               // Got ESC @, expecting k
                if (byte == 0x6B) // k
                {
                    initState = 3;
                    skipNormalProcessing = true;
                }
                else
                {
                    initState = 0; // Reset on unexpected byte
                }
                break;

            case 3:               // Got ESC @ k, expecting CR
                if (byte == 0x0D) // CR
                {
                    // Complete "ESC @ k CR" sequence - enter KISS mode command
                    // No message - KISS mode must be silent
                    commandSystem.setMode(TNCMode::KISS_MODE);
                    serialBuffer = "";
                    initState = 0;
                    return;
                }
                else
                {
                    initState = 0; // Reset on unexpected byte
                }
                break;
            }

            // If we're processing an init sequence, skip normal command handling
            if (skipNormalProcessing)
            {
                continue;
            }
        }

        if (commandSystem.getCurrentMode() == TNCMode::KISS_MODE)
        {
            // Check for ESC character to exit KISS mode (TAPR TNC2 standard)
            if (byte == 0x1B) // ESC
            {
                commandSystem.setMode(TNCMode::COMMAND_MODE);
                // No message in KISS mode - must be silent
                serialBuffer = "";
                return;
            }

            // In KISS mode, process bytes directly through KISS protocol
            // The KISS protocol will handle frame assembly and parsing
            if (kiss.processByte(byte))
            {
                handleIncomingKISS();
            }

            // Check if KISS protocol requested exit (CMD_RETURN 0xFF)
            if (kiss.isExitRequested())
            {
                commandSystem.setMode(TNCMode::COMMAND_MODE);
                kiss.clearExitRequest();
                // No message in KISS mode - must be silent
                serialBuffer = "";
                return;
            }
        }
        else
        {
            // In command mode, handle line-by-line text commands
            char c = (char)byte;

            if (c == '\r' || c == '\n')
            {
                if (serialBuffer.length() > 0)
                {
                    // Process command
                    commandSystem.processCommand(serialBuffer);
                    serialBuffer = "";
                }
                else
                {
                    // Empty line, show prompt
                    commandSystem.sendPrompt();
                }
            }
            else if (c >= ' ') // Printable characters
            {
                serialBuffer += c;
            }
            else if (c == 8 || c == 127) // Backspace or DEL
            {
                if (serialBuffer.length() > 0)
                {
                    serialBuffer.remove(serialBuffer.length() - 1);
                }
            }
        }
    }
}

void TNCManager::handleIncomingKISS()
{
    // Process incoming serial data for KISS frames
    uint8_t buffer[512];
    size_t length = kiss.getFrame(buffer, sizeof(buffer));

    if (length > 0)
    {
        // Check if this is a configuration command (text starting with CONFIG, SETCONFIG, LISTCONFIG, or GETCONFIG)
        String frameData = "";
        bool isText = true;
        for (size_t i = 0; i < length; i++)
        {
            if (buffer[i] >= 32 && buffer[i] < 127)
            {
                frameData += (char)buffer[i];
            }
            else if (buffer[i] != 0)
            { // Allow null termination
                isText = false;
                break;
            }
        }

        if (isText && (frameData.startsWith("CONFIG") || frameData.startsWith("SETCONFIG") ||
                       frameData.startsWith("LISTCONFIG") || frameData.startsWith("GETCONFIG")))
        {
            if (commandSystem.getDebugLevel() >= 2)
            {
                Serial.println("TNC: Processing configuration command: " + frameData);
            }
            processConfigurationCommand(frameData);
            return; // Don't transmit configuration commands over radio
        }

        // Regular data packet - transmit via LoRa
        // Only show debug output in command mode (not KISS mode)
        if (commandSystem.getCurrentMode() != TNCMode::KISS_MODE && commandSystem.getDebugLevel() >= 2)
        {
            Serial.print("TNC: Transmitting ");
            Serial.print(length);
            Serial.print(" bytes: ");
            for (size_t i = 0; i < length && i < 20; i++)
            {
                if (buffer[i] >= 32 && buffer[i] < 127)
                {
                    Serial.print((char)buffer[i]);
                }
                else
                {
                    Serial.print('.');
                }
            }
            Serial.println();
        }

        // Transmit packet
        if (radio.transmit(buffer, length))
        {
            // Only show debug output in command mode (not KISS mode)
            if (commandSystem.getCurrentMode() != TNCMode::KISS_MODE && commandSystem.getDebugLevel() >= 2)
            {
                Serial.println("TNC: Transmission successful");
            }
        }
        else
        {
            // Only show debug output in command mode (not KISS mode)
            if (commandSystem.getCurrentMode() != TNCMode::KISS_MODE && commandSystem.getDebugLevel() >= 2)
            {
                Serial.println("TNC: Transmission failed");
            }
        }
    }
}

void TNCManager::handleIncomingRadio()
{
    // Check for incoming LoRa packets
    if (radio.available())
    {
        uint8_t buffer[512];
        size_t length = radio.receive(buffer, sizeof(buffer));

        if (length > 0)
        {
            // Convert to string for processing
            String packet = "";
            for (size_t i = 0; i < length; i++)
            {
                packet += (char)buffer[i];
            }

            // Get signal quality
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();

            lastPacketRSSI = rssi;
            lastPacketSNR = snr;
            lastPacketTimestamp = millis();
            hasRecentPacket = true;

            if (commandSystem.getDebugLevel() >= 2)
            {
                Serial.print("TNC: Received ");
                Serial.print(length);
                Serial.print(" bytes via LoRa: ");
                for (size_t i = 0; i < length && i < 20; i++)
                {
                    if (buffer[i] >= 32 && buffer[i] < 127)
                    {
                        Serial.print((char)buffer[i]);
                    }
                    else
                    {
                        Serial.print('.');
                    }
                }
                Serial.print(" (RSSI: ");
                Serial.print(rssi, 1);
                Serial.print(" dBm, SNR: ");
                Serial.print(snr, 1);
                Serial.println(" dB)");
            }

            // Process packet for connection management and protocol handling
            commandSystem.processReceivedPacket(packet, rssi, snr);

            bool forwarded = false;
            if (commandSystem.getCurrentMode() == TNCMode::KISS_MODE)
            {
                kiss.sendData(buffer, length);
                forwarded = true;
            }

            if (hasActiveKISSClients())
            {
                broadcastKISSFrame(buffer, length);
                forwarded = true;
            }

            if (!forwarded && (commandSystem.isMonitorEnabled() || commandSystem.getDebugLevel() >= 2))
            {
                const size_t maxPreview = 120;
                size_t previewLength = length < maxPreview ? length : maxPreview;
                String preview;
                preview.reserve(static_cast<unsigned int>(previewLength));
                for (size_t i = 0; i < previewLength; i++)
                {
                    char c = static_cast<char>(buffer[i]);
                    if (c >= 32 && c <= 126)
                    {
                        preview += c;
                    }
                    else
                    {
                        preview += '.';
                    }
                }
                if (length > maxPreview)
                {
                    preview += "...";
                }

                String monitorLine = "MON RX (" + String(length) + " bytes, RSSI " + String(rssi, 1) +
                                      " dBm, SNR " + String(snr, 1) + " dB): " + preview;
                commandSystem.sendResponse(monitorLine);
            }
        }
    }
}

void TNCManager::printStatus()
{
    // In KISS mode, minimize status output to avoid interfering with protocol
    // Only print to debug output if available, otherwise skip

    // Could implement a debug output mechanism here
    // For now, silent operation in KISS mode

    // If we want status output, it should go to a separate debug interface
    // or be disabled entirely in production KISS mode
}

void TNCManager::handleUserButton()
{
    if (powerOffInitiated)
    {
        return;
    }

    unsigned long now = millis();
    bool rawPressed = (digitalRead(USER_BUTTON_PIN) == USER_BUTTON_ACTIVE_STATE);

    if (rawPressed != buttonLastReading)
    {
        buttonLastReading = rawPressed;
        buttonLastChange = now;
    }

    if ((now - buttonLastChange) >= BUTTON_DEBOUNCE_MS && rawPressed != buttonStableState)
    {
        buttonStableState = rawPressed;
        if (buttonStableState)
        {
            buttonPressStart = now;
            buttonLongPressHandled = false;
        }
        else
        {
            if (!buttonLongPressHandled)
            {
                display.nextScreen();
            }

            powerOffWarningActive = false;
            powerOffProgress = 0.0f;
            buttonLongPressHandled = false;
        }
    }

    if (buttonStableState)
    {
        unsigned long held = now - buttonPressStart;

        if (held >= POWER_OFF_HOLD_MS)
        {
            powerOffWarningActive = true;
            powerOffProgress = 1.0f;
            buttonLongPressHandled = true;
            performPowerOff();
        }
        else if (held >= POWER_OFF_WARNING_DELAY_MS)
        {
            powerOffWarningActive = true;
            float denom = static_cast<float>(POWER_OFF_HOLD_MS - POWER_OFF_WARNING_DELAY_MS);
            float progress = denom > 0.0f ? static_cast<float>(held - POWER_OFF_WARNING_DELAY_MS) / denom : 1.0f;
            powerOffProgress = clamp01(progress);
            buttonLongPressHandled = true;
        }
    }
    else if (powerOffWarningActive)
    {
        powerOffWarningActive = false;
        powerOffProgress = 0.0f;
    }
}

DisplayManager::StatusData TNCManager::buildDisplayStatus()
{
    DisplayManager::StatusData status;
    status.mode = commandSystem.getCurrentMode();
    status.txCount = radio.getTxCount();
    status.rxCount = radio.getRxCount();
    status.batteryVoltage = lastBatteryVoltage;
    status.batteryPercent = lastBatteryPercent;
    status.hasRecentPacket = hasRecentPacket;
    status.lastRSSI = lastPacketRSSI;
    status.lastSNR = lastPacketSNR;
    status.lastPacketMillis = lastPacketTimestamp;
    status.frequency = radio.getFrequency();
    status.bandwidth = radio.getBandwidth();
    status.spreadingFactor = radio.getSpreadingFactor();
    status.codingRate = radio.getCodingRate();
    status.txPower = radio.getTxPower();
    status.uptimeMillis = millis();
    status.powerOffActive = powerOffWarningActive;
    status.powerOffProgress = powerOffProgress;
    status.powerOffComplete = powerOffComplete;

    auto wifiInfo = wifiManager.getStatus();
    using WiFiMode = DisplayManager::StatusData::WiFiMode;

    if (wifiInfo.state == SimpleWiFiManager::State::STA_CONNECTED)
    {
        status.wifiMode = WiFiMode::STATION;
    }
    else if (wifiInfo.state == SimpleWiFiManager::State::AP_READY)
    {
        status.wifiMode = WiFiMode::ACCESS_POINT;
    }
    else
    {
        status.wifiMode = WiFiMode::OFF;
    }

    status.wifiConnected = (wifiInfo.state == SimpleWiFiManager::State::STA_CONNECTED) || 
                          (status.wifiMode == WiFiMode::ACCESS_POINT);
    status.wifiConnecting = (wifiInfo.state == SimpleWiFiManager::State::STA_CONNECTING);
    status.wifiHasIPAddress = false;
    status.wifiSSID[0] = '\0';
    status.wifiIPAddress[0] = '\0';
    status.wifiAPPassword[0] = '\0';

    auto copyString = [](char *destination, size_t destinationSize, const String &value) {
        if (destinationSize == 0)
        {
            return;
        }

        if (value.length() >= destinationSize)
        {
            value.substring(0, destinationSize - 1).toCharArray(destination, destinationSize);
        }
        else
        {
            value.toCharArray(destination, destinationSize);
        }
    };

    if (wifiInfo.state == SimpleWiFiManager::State::STA_CONNECTED)
    {
        copyString(status.wifiSSID, sizeof(status.wifiSSID), wifiInfo.currentSSID);

        String ipString = wifiInfo.currentIP.toString();
        if (ipString != "0.0.0.0")
        {
            copyString(status.wifiIPAddress, sizeof(status.wifiIPAddress), ipString);
            status.wifiHasIPAddress = true;
        }
    }
    else if (wifiInfo.state == SimpleWiFiManager::State::AP_READY)
    {
        copyString(status.wifiSSID, sizeof(status.wifiSSID), wifiInfo.apSSID);

        String ipString = wifiInfo.apIP.toString();
        if (ipString != "0.0.0.0")
        {
            copyString(status.wifiIPAddress, sizeof(status.wifiIPAddress), ipString);
            status.wifiHasIPAddress = true;
        }
        
        // Get the AP password for display
        if (!wifiInfo.apPassword.isEmpty())
        {
            copyString(status.wifiAPPassword, sizeof(status.wifiAPPassword), wifiInfo.apPassword);
        }
    }
    else if (!wifiInfo.currentSSID.isEmpty())
    {
        copyString(status.wifiSSID, sizeof(status.wifiSSID), wifiInfo.currentSSID);
    }

    status.gnssEnabled = gnssEnabled && gnssInitialised;
    if (status.gnssEnabled)
    {
        const auto &fix = gnss.getFixData();
        status.gnssHasFix = fix.valid;
        status.gnssIs3DFix = fix.is3DFix;
        status.gnssLatitude = fix.latitude;
        status.gnssLongitude = fix.longitude;
        status.gnssAltitude = fix.altitudeMeters;
        status.gnssSpeed = fix.speedKnots;
        status.gnssCourse = fix.courseDegrees;
        status.gnssHdop = fix.hdop;
        status.gnssSatellites = fix.satellites;

        const auto &timeStatus = gnss.getTimeStatus();
        status.gnssTimeValid = timeStatus.valid;
        status.gnssTimeSynced = timeStatus.synced;
        status.gnssYear = timeStatus.year;
        status.gnssMonth = timeStatus.month;
        status.gnssDay = timeStatus.day;
        status.gnssHour = timeStatus.hour;
        status.gnssMinute = timeStatus.minute;
        status.gnssSecond = timeStatus.second;

        const auto pps = gnss.getPPSStatus();
        status.gnssPpsAvailable = pps.available;
        status.gnssPpsLastMillis = pps.lastPulseMillis;
        status.gnssPpsCount = pps.pulseCount;
    }
    return status;
}

void TNCManager::performPowerOff()
{
    if (powerOffInitiated)
    {
        return;
    }

    powerOffInitiated = true;
    powerOffComplete = true;
    powerOffWarningActive = true;
    powerOffProgress = 1.0f;

    Serial.println("Power-off button hold detected. Shutting down.");

    display.updateStatus(buildDisplayStatus());
    delay(500);

    // Shutdown all peripherals first
    Serial.println("Shutting down peripherals...");
    
    // Stop all TCP clients and servers
    stopClients(kissTcpClients);
    stopClients(nmeaTcpClients);
    kissTcpServer.stop();
    nmeaTcpServer.stop();
    webServer.stop();
    
    // Turn off external power (OLED, sensors, etc.)
    digitalWrite(POWER_CTRL_PIN, POWER_OFF);
    
    // Shutdown GNSS if enabled
    if (gnssEnabled && gnssInitialised)
    {
        gnss.end();
    }
    
    // Shutdown LoRa radio (set to lowest power mode)
    digitalWrite(LORA_PA_EN_PIN, LOW);
    digitalWrite(LORA_PA_TX_EN_PIN, LOW);
    analogWrite(LORA_PA_POWER_PIN, 0);
    
    // Shutdown WiFi completely
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_MODE_NULL);
    
    // Final delay before deep sleep
    delay(1000);
    
    Serial.println("Entering deep sleep mode...");
    Serial.flush(); // Ensure message is sent before sleep
    
    // Configure wake-up source (button press)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Wake on button press (active LOW)
    
    // Enter deep sleep - this will truly power off the ESP32
    esp_deep_sleep_start();
    
    // This line should never be reached
    while (true)
    {
        delay(1000);
    }
}

void TNCManager::updateTcpServers()
{
    auto wifiInfo = wifiManager.getStatus();
    bool wifiReady = wifiInfo.isReady;

    if (wifiReady)
    {
        if (!kissServerRunning)
        {
            kissTcpServer.begin();
            kissTcpServer.setNoDelay(true);
            kissServerRunning = true;
        }

        if (!nmeaServerRunning)
        {
            nmeaTcpServer.begin();
            nmeaTcpServer.setNoDelay(true);
            nmeaServerRunning = true;
        }

        acceptClient(kissTcpServer, kissTcpClients);
        acceptClient(nmeaTcpServer, nmeaTcpClients);
    }
    else
    {
        if (kissServerRunning)
        {
            stopClients(kissTcpClients);
            kissTcpServer.stop();
            kissServerRunning = false;
        }

        if (nmeaServerRunning)
        {
            stopClients(nmeaTcpClients);
            nmeaTcpServer.stop();
            nmeaServerRunning = false;
        }
    }

    pruneClients(kissTcpClients);
    pruneClients(nmeaTcpClients);
}

void TNCManager::acceptClient(WiFiServer &server, std::array<WiFiClient, MAX_TCP_CLIENTS> &clients)
{
    if (!server.hasClient())
    {
        return;
    }

    WiFiClient incoming = server.accept();
    if (!incoming)
    {
        return;
    }

    for (auto &client : clients)
    {
        if (client && client.remoteIP() == incoming.remoteIP() && client.remotePort() == incoming.remotePort())
        {
            incoming.stop();
            return;
        }
    }

    for (auto &client : clients)
    {
        if (!client || !client.connected())
        {
            if (client)
            {
                client.stop();
            }
            client = incoming;
            client.setNoDelay(true);
            return;
        }
    }

    incoming.stop();
}

void TNCManager::pruneClients(std::array<WiFiClient, MAX_TCP_CLIENTS> &clients)
{
    for (auto &client : clients)
    {
        if (client && !client.connected())
        {
            client.stop();
        }
    }
}

void TNCManager::stopClients(std::array<WiFiClient, MAX_TCP_CLIENTS> &clients)
{
    for (auto &client : clients)
    {
        if (client)
        {
            client.stop();
        }
    }
}

void TNCManager::processKISSTcpClients()
{
    for (auto &client : kissTcpClients)
    {
        if (!client || !client.connected())
        {
            continue;
        }

        while (client.available())
        {
            int value = client.read();
            if (value < 0)
            {
                break;
            }

            if (kiss.processByte(static_cast<uint8_t>(value)))
            {
                handleIncomingKISS();
            }
        }
    }
}

void TNCManager::broadcastKISSFrame(const uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return;
    }

    for (auto &client : kissTcpClients)
    {
        if (!client || !client.connected())
        {
            continue;
        }

        client.write(static_cast<uint8_t>(FEND));
        client.write(static_cast<uint8_t>(CMD_DATA));
        for (size_t i = 0; i < length; ++i)
        {
            uint8_t byte = data[i];
            if (byte == FEND)
            {
                client.write(static_cast<uint8_t>(FESC));
                client.write(static_cast<uint8_t>(TFEND));
            }
            else if (byte == FESC)
            {
                client.write(static_cast<uint8_t>(FESC));
                client.write(static_cast<uint8_t>(TFESC));
            }
            else
            {
                client.write(byte);
            }
        }
        client.write(static_cast<uint8_t>(FEND));
        // Note: flush() is deprecated, data is sent immediately for WiFiClient
    }
}

void TNCManager::broadcastNMEALine(const String &line)
{
    if (line.length() == 0)
    {
        return;
    }

    String payload = line;
    if (!payload.endsWith("\r\n"))
    {
        payload += "\r\n";
    }

    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(payload.c_str());
    size_t length = payload.length();

    for (auto &client : nmeaTcpClients)
    {
        if (!client || !client.connected())
        {
            continue;
        }

        client.write(buffer, length);
        // Note: flush() is deprecated, data is sent immediately for WiFiClient
    }
}

bool TNCManager::hasActiveKISSClients()
{
    for (auto &client : kissTcpClients)
    {
        if (client && client.connected())
        {
            return true;
        }
    }

    return false;
}

void TNCManager::handleNMEASentence(const String &line)
{
    broadcastNMEALine(line);
}

bool TNCManager::setGNSSEnabled(bool enable)
{
    bool result = true;

    if (enable)
    {
        if (!gnssEnabled || !gnssInitialised)
        {
            Serial.println("Enabling GNSS module...");
            gnssEnabled = true;
            if (gnss.begin())
            {
                Serial.println("✓ GNSS module enabled");
                gnssInitialised = true;
            }
            else
            {
                Serial.println("✗ GNSS initialization failed");
                gnssEnabled = false;
                gnssInitialised = false;
                result = false;
            }
        }
    }
    else
    {
        if (gnssEnabled || gnssInitialised)
        {
            Serial.println("Disabling GNSS module...");
            if (gnssInitialised)
            {
                gnss.end();
            }
            gnssEnabled = false;
            gnssInitialised = false;
            Serial.println("✓ GNSS module disabled");
        }
    }

    commandSystem.setPeripheralStateDefaults(isGNSSEnabled(), isOLEDEnabled());
    return result;
}

bool TNCManager::setOLEDEnabled(bool enable)
{
    if (!display.isAvailable())
    {
        Serial.println("OLED display hardware not available");
        oledEnabled = false;
        commandSystem.setPeripheralStateDefaults(isGNSSEnabled(), isOLEDEnabled());
        return false;
    }

    bool result = true;

    if (enable)
    {
        if (!oledEnabled || !display.isEnabled())
        {
            Serial.println("Enabling OLED display...");
            if (display.setEnabled(true))
            {
                oledEnabled = true;
                Serial.println("✓ OLED display enabled");
                display.updateStatus(buildDisplayStatus());
            }
            else
            {
                Serial.println("✗ OLED display enable failed");
                oledEnabled = display.isEnabled();
                result = false;
            }
        }
        else
        {
            oledEnabled = true;
        }
    }
    else
    {
        if (oledEnabled && display.isEnabled())
        {
            Serial.println("Disabling OLED display...");
            if (display.setEnabled(false))
            {
                oledEnabled = false;
                Serial.println("✓ OLED display disabled");
            }
            else
            {
                Serial.println("✗ OLED display disable failed");
                oledEnabled = display.isEnabled();
                result = false;
            }
        }
        else
        {
            oledEnabled = false;
            if (display.isEnabled())
            {
                display.setEnabled(false);
            }
        }
    }

    oledEnabled = display.isEnabled();
    commandSystem.setPeripheralStateDefaults(isGNSSEnabled(), isOLEDEnabled());
    return result;
}

// Static callback functions for TNCCommands
bool TNCManager::gnssSetEnabledCallback(bool enable) {
    if (instance) {
        return instance->setGNSSEnabled(enable);
    }
    return false;
}

bool TNCManager::oledSetEnabledCallback(bool enable) {
    if (instance) {
        return instance->setOLEDEnabled(enable);
    }
    return false;
}

bool TNCManager::oledGetEnabledCallback() {
    if (instance) {
        return instance->isOLEDEnabled();
    }
    return false;
}

bool TNCManager::gnssGetEnabledCallback() {
    if (instance) {
        return instance->isGNSSEnabled();
    }
    return false;
}

bool TNCManager::wifiAddNetworkCallback(const String &ssid, const String &password, String &message)
{
    if (instance)
    {
        bool result = instance->wifiManager.addNetwork(ssid, password);
        message = result ? "Network added successfully" : "Failed to add network";
        return result;
    }
    message = "WiFi manager unavailable";
    return false;
}

bool TNCManager::wifiRemoveNetworkCallback(const String &ssid, String &message)
{
    if (instance)
    {
        bool result = instance->wifiManager.removeNetwork(ssid);
        message = result ? "Network removed successfully" : "Network not found";
        return result;
    }
    message = "WiFi manager unavailable";
    return false;
}

void TNCManager::wifiListNetworksCallback(String &output)
{
    if (instance)
    {
        auto networks = instance->wifiManager.getStoredNetworks();
        output = "Stored networks:\n";
        for (size_t i = 0; i < networks.size(); i++) {
            output += String(i + 1) + ". " + networks[i].ssid + "\n";
        }
        if (networks.empty()) {
            output = "No networks stored";
        }
    }
    else
    {
        output = "WiFi manager unavailable";
    }
}

void TNCManager::wifiStatusCallback(String &output)
{
    if (instance)
    {
        auto status = instance->wifiManager.getStatus();
        output = "WiFi Status:\n";
        output += "State: ";
        switch (status.state) {
            case SimpleWiFiManager::State::INIT: output += "Initializing"; break;
            case SimpleWiFiManager::State::STA_CONNECTING: output += "Connecting to station"; break;
            case SimpleWiFiManager::State::STA_CONNECTED: output += "Connected to station"; break;
            case SimpleWiFiManager::State::AP_STARTING: output += "Starting access point"; break;
            case SimpleWiFiManager::State::AP_READY: output += "Access point ready"; break;
            case SimpleWiFiManager::State::ERROR: output += "Error"; break;
        }
        output += "\n";
        
        if (!status.currentSSID.isEmpty()) {
            output += "Connected to: " + status.currentSSID + "\n";
            output += "IP Address: " + status.currentIP.toString() + "\n";
        }
        
        if (!status.apSSID.isEmpty()) {
            output += "AP SSID: " + status.apSSID + "\n";
            output += "AP IP: " + status.apIP.toString() + "\n";
        }
    }
    else
    {
        output = "WiFi manager unavailable";
    }
}

void TNCManager::onWiFiStateChange(bool ready)
{
    if (ready)
    {
        // Only start web server if it's not already running
        if (!webServer.isRunning())
        {
            Serial.println("WiFi is ready - starting web server...");
            if (webServer.start())
            {
                auto wifiInfo = wifiManager.getStatus();
                if (wifiInfo.state == SimpleWiFiManager::State::STA_CONNECTED)
                {
                    Serial.print("Web interface available at: http://");
                    Serial.println(wifiInfo.currentIP.toString());
                }
                else if (wifiInfo.state == SimpleWiFiManager::State::AP_READY)
                {
                    Serial.print("Web interface available at: http://");
                    Serial.println(wifiInfo.apIP.toString());
                    Serial.print("AP Password: ");
                    Serial.println(wifiInfo.apPassword);
                }
            }
        }
    }
    else
    {
        // Only stop web server if WiFi is truly disconnected (not just a temporary state change)
        auto wifiInfo = wifiManager.getStatus();
        bool actuallyDisconnected = !wifiInfo.isReady;
        
        if (actuallyDisconnected && webServer.isRunning())
        {
            Serial.println("WiFi disconnected - stopping web server...");
            webServer.stop();
        }
        else if (!actuallyDisconnected)
        {
            Serial.println("WiFi state change detected, but connection still available - keeping web server running");
        }
    }
}

String TNCManager::getSystemStatusForWeb()
{
    return getStatus();
}

String TNCManager::getLoRaStatusForWeb()
{
    return radio.getStatus();
}

String TNCManager::getWiFiNetworksForWeb()
{
    String result = "[";
    auto networks = wifiManager.getStoredNetworks();
    
    for (size_t i = 0; i < networks.size(); i++) {
        if (i > 0) {
            result += ",";
        }
        result += "{\"ssid\":\"" + networks[i].ssid + "\"}";
    }
    
    result += "]";
    return result;
}

