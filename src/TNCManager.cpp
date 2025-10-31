/**
 * @file TNCManager.cpp
 * @brief TNC Manager implementation
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "TNCManager.h"

#include "HardwareConfig.h"
#include "WebInterfaceManager.h"

namespace
{
constexpr unsigned long BUTTON_DEBOUNCE_MS = 30UL;
constexpr unsigned long POWER_OFF_WARNING_DELAY_MS = 750UL;
constexpr unsigned long POWER_OFF_HOLD_MS = 3000UL;
constexpr size_t MAX_EXTERNAL_COMMAND_LENGTH = 256;

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

TNCManager::TNCManager() : configManager(&radio), display(), batteryMonitor(), gnss(), webInterface(nullptr)
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

    // Set static instance for callbacks
    instance = this;
}

bool TNCManager::begin()
{
    Serial.println("=== LoRaTNCX Initialization ===");

    bool configLoaded = loadConfigurationFromFlash();
    if (configLoaded)
    {
        Serial.println("Loaded configuration from flash.");
    }
    else
    {
        Serial.println("No saved configuration found; using defaults.");
    }

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

    if (configLoaded)
    {
        gnssEnabled = commandSystem.getStoredGNSSEnabled();
        oledEnabled = commandSystem.getStoredOLEDEnabled();
    }
    else
    {
        gnssEnabled = GNSS_ENABLED;
    }
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
    Serial.println("✓ Command system hardware integration enabled");

    if (configLoaded)
    {
        if (commandSystem.getStoredGNSSEnabled() != isGNSSEnabled())
        {
            setGNSSEnabled(commandSystem.getStoredGNSSEnabled());
        }
        if (commandSystem.getStoredOLEDEnabled() != isOLEDEnabled())
        {
            setOLEDEnabled(commandSystem.getStoredOLEDEnabled());
        }
    }

    Serial.println("\n=== TNC Ready ===");
    Serial.println("Starting in Command mode...");
    Serial.println("Type HELP for available commands");
    Serial.println("Type KISS to enter KISS mode");
    Serial.println(configManager.getConfigStatus());
    Serial.println("===============================");

    initialized = true;

    DisplayManager::StatusData status = buildDisplayStatus();
    display.updateStatus(status);
    if (webInterface)
    {
        webInterface->broadcastStatus(status);
    }
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

    handleUserButton();

    DisplayManager::StatusData status = buildDisplayStatus();
    display.updateStatus(status);
    if (webInterface)
    {
        webInterface->broadcastStatus(status);
    }
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

TNCManager::StatusSnapshot TNCManager::getStatusSnapshot()
{
    StatusSnapshot snapshot;
    snapshot.displayStatus = buildDisplayStatus();
    snapshot.statusText = getStatus();
    return snapshot;
}

DisplayManager::StatusData TNCManager::getDisplayStatusSnapshot()
{
    return buildDisplayStatus();
}

bool TNCManager::processConfigurationCommand(const String &command)
{
    return configManager.processConfigCommand(command);
}

bool TNCManager::processConfigurationCommand(const char *command)
{
    if (command == nullptr)
    {
        return false;
    }

    size_t length = 0;
    while (command[length] != '\0' && length <= MAX_EXTERNAL_COMMAND_LENGTH)
    {
        ++length;
    }

    if (length == 0 || length > MAX_EXTERNAL_COMMAND_LENGTH)
    {
        return false;
    }

    String safeCommand(command);
    safeCommand.trim();
    if (safeCommand.length() == 0)
    {
        return false;
    }

    return processConfigurationCommand(safeCommand);
}

bool TNCManager::loadConfigurationFromFlash()
{
    bool loaded = commandSystem.loadConfigurationFromFlash();
    if (loaded)
    {
        commandSystem.setPeripheralStateDefaults(commandSystem.getStoredGNSSEnabled(), commandSystem.getStoredOLEDEnabled());
    }
    return loaded;
}

bool TNCManager::saveConfigurationToFlash()
{
    return commandSystem.saveConfigurationToFlash();
}

bool TNCManager::hasWiFiCredentials() const
{
    return commandSystem.hasWiFiCredentials();
}

String TNCManager::getWiFiSSID() const
{
    return commandSystem.getWiFiSSID();
}

String TNCManager::getWiFiPassword() const
{
    return commandSystem.getWiFiPassword();
}

void TNCManager::setWiFiCredentials(const String &ssid, const String &password)
{
    commandSystem.setWiFiCredentials(ssid, password);
}

bool TNCManager::hasUICredentials() const
{
    return commandSystem.hasUICredentials();
}

String TNCManager::getUIUsername() const
{
    return commandSystem.getUIUsername();
}

String TNCManager::getUIPassword() const
{
    return commandSystem.getUIPassword();
}

void TNCManager::setUICredentials(const String &username, const String &password)
{
    commandSystem.setUICredentials(username, password);
}

void TNCManager::setUIThemePreference(const String &theme, bool overrideEnabled)
{
    commandSystem.setUIThemePreference(theme, overrideEnabled);
}

void TNCManager::getUIThemePreference(String &theme, bool &overrideEnabled) const
{
    theme = commandSystem.getUIThemePreference();
    overrideEnabled = commandSystem.isUIThemeOverrideEnabled();
}

TNCCommandResult TNCManager::executeCommand(const String &command)
{
    String sanitized = command;
    sanitized.trim();

    if (sanitized.length() == 0)
    {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }

    if (sanitized.length() > MAX_EXTERNAL_COMMAND_LENGTH)
    {
        return TNCCommandResult::ERROR_TOO_MANY_ARGS;
    }

    return commandSystem.processCommand(sanitized);
}

TNCCommandResult TNCManager::executeCommand(const char *command)
{
    if (command == nullptr)
    {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }

    size_t length = 0;
    while (command[length] != '\0' && length <= MAX_EXTERNAL_COMMAND_LENGTH)
    {
        ++length;
    }

    if (length == 0)
    {
        return TNCCommandResult::ERROR_INVALID_PARAMETER;
    }

    if (length > MAX_EXTERNAL_COMMAND_LENGTH)
    {
        return TNCCommandResult::ERROR_TOO_MANY_ARGS;
    }

    String safeCommand(command);
    return executeCommand(safeCommand);
}

String TNCManager::getConfigurationStatus() const
{
    return configManager.getConfigStatus();
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

            // Only forward KISS frames to the host when operating in KISS mode.
            if (commandSystem.getCurrentMode() == TNCMode::KISS_MODE)
            {
                kiss.sendData(buffer, length);
            }
            const bool needMonitorPreview = commandSystem.isMonitorEnabled() || commandSystem.getDebugLevel() >= 2;
            const bool needWebPreview = webInterface != nullptr;
            String preview;

            if (needMonitorPreview || needWebPreview)
            {
                const size_t maxPreview = 120;
                size_t previewLength = length < maxPreview ? length : maxPreview;
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
            }

            if (needMonitorPreview)
            {
                String monitorLine = "MON RX (" + String(length) + " bytes, RSSI " + String(rssi, 1) +
                                      " dBm, SNR " + String(snr, 1) + " dB): " + preview;
                commandSystem.sendResponse(monitorLine);
            }

            if (webInterface)
            {
                webInterface->broadcastPacketNotification(length, rssi, snr, preview);
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

    digitalWrite(POWER_CTRL_PIN, POWER_OFF);
    delay(1000);

    while (true)
    {
        delay(1000);
    }
}

bool TNCManager::setGNSSEnabled(bool enable)
{
    bool previousState = isGNSSEnabled();
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

    bool currentState = isGNSSEnabled();
    if (webInterface && (currentState != previousState || !result))
    {
        String message;
        if (!result)
        {
            message = enable ? "Failed to enable GNSS module" : "Failed to disable GNSS module";
        }
        else if (currentState)
        {
            message = "GNSS module enabled";
        }
        else
        {
            message = "GNSS module disabled";
        }

        webInterface->broadcastAlert("gnss", message, currentState);
    }

    return result;
}

bool TNCManager::setOLEDEnabled(bool enable)
{
    bool previousState = display.isEnabled();
    if (!display.isAvailable())
    {
        Serial.println("OLED display hardware not available");
        oledEnabled = false;
        commandSystem.setPeripheralStateDefaults(isGNSSEnabled(), isOLEDEnabled());
        if (webInterface && previousState)
        {
            webInterface->broadcastAlert("oled", "OLED display hardware not available", false);
        }
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

    bool currentState = display.isEnabled();
    if (webInterface && (currentState != previousState || !result))
    {
        String message;
        if (!result)
        {
            message = enable ? "Failed to enable OLED display" : "Failed to disable OLED display";
        }
        else if (currentState)
        {
            message = "OLED display enabled";
        }
        else
        {
            message = "OLED display disabled";
        }

        webInterface->broadcastAlert("oled", message, currentState);
    }

    return result;
}

void TNCManager::setWebInterface(WebInterfaceManager *interface)
{
    webInterface = interface;
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
