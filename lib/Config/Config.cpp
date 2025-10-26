#include "Config.h"
#include <WiFi.h>
#include "esp_sleep.h"
#include "esp_bt.h"

ConfigManager config;

bool ConfigManager::begin()
{
    if (!prefs.begin("tnc-config", false))
    {
        Serial.println("[CONFIG] Failed to initialize NVS");
        return false;
    }

    loadConfig();
    Serial.println("[CONFIG] Configuration manager initialized");
    return true;
}

void ConfigManager::loadConfig()
{
    prefs.getBytes("wifi", &wifiConfig, sizeof(WiFiConfig));

    prefs.getBytes("radio", &radioConfig, sizeof(RadioConfig));

    prefs.getBytes("gnss", &gnssConfig, sizeof(GNSSConfig));

    prefs.getBytes("aprs", &aprsConfig, sizeof(APRSConfig));

    prefs.getBytes("battery", &batteryConfig, sizeof(BatteryConfig));

    Serial.println("[CONFIG] Configuration loaded from NVS");
}

void ConfigManager::saveConfig()
{
    prefs.putBytes("wifi", &wifiConfig, sizeof(WiFiConfig));
    prefs.putBytes("radio", &radioConfig, sizeof(RadioConfig));
    prefs.putBytes("gnss", &gnssConfig, sizeof(GNSSConfig));
    prefs.putBytes("aprs", &aprsConfig, sizeof(APRSConfig));
    prefs.putBytes("battery", &batteryConfig, sizeof(BatteryConfig));

    Serial.println("[CONFIG] Configuration saved to NVS");
}

void ConfigManager::resetToDefaults()
{
    wifiConfig = WiFiConfig{};
    radioConfig = RadioConfig{};
    gnssConfig = GNSSConfig{};
    aprsConfig = APRSConfig{};
    batteryConfig = BatteryConfig{};

    saveConfig();
    Serial.println("[CONFIG] Reset to default configuration");
}

void ConfigManager::showMenu()
{
    if (!inMenu)
    {
        inMenu = true;
        inputBuffer = "";
        showMainMenu();
    }
}

void ConfigManager::showMainMenu()
{
    Serial.println("\n========== TNC Configuration Menu ==========");
    Serial.println("1. WiFi Settings");
    Serial.println("2. Radio Settings");
#if GNSS_ENABLE
    Serial.println("3. GNSS Settings");
    Serial.println("4. APRS Settings & Operating Mode");
    Serial.println("5. Battery Settings");
    Serial.println("6. Show Current Configuration");
    Serial.println("7. Save & Exit");
    Serial.println("8. Reset to Defaults");
    Serial.println("9. Power Off Device");
#else
    Serial.println("3. Battery Settings");
    Serial.println("4. Show Current Configuration");
    Serial.println("5. Save & Exit");
    Serial.println("6. Reset to Defaults");
    Serial.println("7. Power Off Device");
#endif
    Serial.println("0. Exit without Saving");
    Serial.println("==========================================");
#if !GNSS_ENABLE
    Serial.println("Note: GNSS/APRS options not available (disabled in build)");
#endif
    Serial.print("Select option: ");
}

void ConfigManager::handleMenuInput()
{
    if (!inMenu)
        return;

    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            Serial.println();

            if (inputBuffer.length() == 0)
            {
                showMainMenu();
                return;
            }

            int choice = inputBuffer.toInt();
            inputBuffer = "";

            switch (choice)
            {
            case 1:
                handleWiFiMenu();
                break;
            case 2:
                handleRadioMenu();
                break;
#if GNSS_ENABLE
            case 3:
                handleGNSSMenu();
                break;
            case 4:
                handleAPRSMenu();
                break;
            case 5:
                handleBatteryMenu();
                break;
            case 6:
                showCurrentConfig();
                showMainMenu();
                break;
            case 7:
                saveConfig();
                Serial.println("Configuration saved. Restarting...");
                delay(1000);
                ESP.restart();
                break;
            case 8:
                resetToDefaults();
                showMainMenu();
                break;
            case 9:
                powerOffDevice();
                break;
#else
            case 3:
                handleBatteryMenu();
                break;
            case 4:
                showCurrentConfig();
                showMainMenu();
                break;
            case 5:
                saveConfig();
                Serial.println("Configuration saved. Restarting...");
                delay(1000);
                ESP.restart();
                break;
            case 6:
                resetToDefaults();
                showMainMenu();
                break;
            case 7:
                powerOffDevice();
                break;
#endif
            case 0:
                inMenu = false;
                Serial.println("Exited configuration menu");
                break;
            default:
                Serial.println("Invalid option");
                showMainMenu();
                break;
            }
        }
        else if (c == 8 || c == 127)
        {
            if (inputBuffer.length() > 0)
            {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.print("\b \b");
            }
        }
        else if (c >= 32 && c <= 126)
        {
            inputBuffer += c;
            Serial.print(c);
        }
    }
}

void ConfigManager::handleWiFiMenu()
{
    Serial.println("\n--- WiFi Configuration ---");
    Serial.printf("Current mode: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
    Serial.printf("AP SSID: %s\n", wifiConfig.ssid);
    Serial.printf("STA SSID: %s\n", wifiConfig.sta_ssid);
    Serial.println();
    Serial.println("1. Toggle AP/STA mode");
    Serial.println("2. Set AP SSID");
    Serial.println("3. Set AP Password");
    Serial.println("4. Set STA SSID");
    Serial.println("5. Set STA Password");
    Serial.println("0. Back to main menu");
    Serial.print("Select: ");

    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        Serial.println("\nTimeout - returning to main menu");
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        wifiConfig.useAP = !wifiConfig.useAP;
        Serial.printf("WiFi mode set to: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
        break;

    case 2:
        input = promptForString("Enter AP SSID: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.ssid, input.c_str(), sizeof(wifiConfig.ssid) - 1);
            wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';
            Serial.printf("AP SSID set to: %s\n", wifiConfig.ssid);
        }
        break;

    case 3:
        input = promptForString("Enter AP Password: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.password, input.c_str(), sizeof(wifiConfig.password) - 1);
            wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';
            Serial.println("AP Password updated");
        }
        break;

    case 4:
        input = promptForString("Enter STA SSID: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.sta_ssid, input.c_str(), sizeof(wifiConfig.sta_ssid) - 1);
            wifiConfig.sta_ssid[sizeof(wifiConfig.sta_ssid) - 1] = '\0';
            Serial.printf("STA SSID set to: %s\n", wifiConfig.sta_ssid);
        }
        break;

    case 5:
        input = promptForString("Enter STA Password: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.sta_password, input.c_str(), sizeof(wifiConfig.sta_password) - 1);
            wifiConfig.sta_password[sizeof(wifiConfig.sta_password) - 1] = '\0';
            Serial.println("STA Password updated");
        }
        break;

    case 0:
        showMainMenu();
        return;

    default:
        Serial.println("Invalid option");
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleRadioMenu()
{
    Serial.println("\n--- Radio Configuration ---");
    Serial.printf("Frequency: %.3f MHz\n", radioConfig.frequency);
    Serial.printf("Bandwidth: %.1f kHz\n", radioConfig.bandwidth);
    Serial.printf("Spreading Factor: %d\n", radioConfig.spreadingFactor);
    Serial.printf("Coding Rate: 4/%d\n", radioConfig.codingRate);
    Serial.printf("TX Power: %d dBm\n", radioConfig.txPower);
    Serial.println();
    Serial.println("=== Configuration ===");
    Serial.println("1. Set Frequency (MHz)");
    Serial.println("2. Set Bandwidth (kHz)");
    Serial.println("3. Set Spreading Factor (7-12)");
    Serial.println("4. Set Coding Rate (5-8)");
    Serial.println("5. Set TX Power (dBm)");
    Serial.println("=== Diagnostics ===");
    Serial.println("6. Run Radio Health Check");
    Serial.println("7. Check Hardware Pins");
    Serial.println("8. Run Transmission Test");
    Serial.println("9. Run Continuous TX Test (30s)");
    Serial.println("0. Back to main menu");
    Serial.print("Select: ");

    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        Serial.println("\nTimeout - returning to main menu");
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        input = promptForString("Enter frequency (MHz): ");
        if (input.length() > 0)
        {
            radioConfig.frequency = parseFloat(input, radioConfig.frequency);
            Serial.printf("Frequency set to: %.3f MHz\n", radioConfig.frequency);
        }
        break;

    case 2:
        input = promptForString("Enter bandwidth (kHz) [7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500]: ");
        if (input.length() > 0)
        {
            radioConfig.bandwidth = parseFloat(input, radioConfig.bandwidth);
            Serial.printf("Bandwidth set to: %.1f kHz\n", radioConfig.bandwidth);
        }
        break;

    case 3:
    {
        input = promptForString("Enter spreading factor (7-12): ");
        if (input.length() > 0)
        {
            int sf = parseInt(input, radioConfig.spreadingFactor);
            if (sf >= 7 && sf <= 12)
            {
                radioConfig.spreadingFactor = sf;
                Serial.printf("Spreading factor set to: %d\n", radioConfig.spreadingFactor);
            }
            else
            {
                Serial.println("Invalid spreading factor (7-12)");
            }
        }
    }
    break;

    case 4:
    {
        input = promptForString("Enter coding rate (5-8): ");
        if (input.length() > 0)
        {
            int cr = parseInt(input, radioConfig.codingRate);
            if (cr >= 5 && cr <= 8)
            {
                radioConfig.codingRate = cr;
                Serial.printf("Coding rate set to: 4/%d\n", radioConfig.codingRate);
            }
            else
            {
                Serial.println("Invalid coding rate (5-8)");
            }
        }
    }
    break;

    case 5:
        input = promptForString("Enter TX power (dBm): ");
        if (input.length() > 0)
        {
            radioConfig.txPower = parseInt(input, radioConfig.txPower);
            Serial.printf("TX power set to: %d dBm\n", radioConfig.txPower);
        }
        break;

    case 6:
        Serial.println("\n🔍 Running radio health check...");
        runRadioHealthCheck();
        Serial.println("\nPress any key to continue...");
        waitForInput(30000);
        break;

    case 7:
        Serial.println("\n🔧 Checking hardware pins...");
        runHardwarePinCheck();
        Serial.println("\nPress any key to continue...");
        waitForInput(30000);
        break;

    case 8:
        Serial.println("\n🧪 Running transmission test...");
        runTransmissionTest();
        Serial.println("\nPress any key to continue...");
        waitForInput(30000);
        break;

    case 9:
        Serial.println("\n🎯 Running continuous transmission test...");
        Serial.println("WARNING: This will transmit for 30 seconds!");
        Serial.println("Make sure your SDR is monitoring the frequency!");
        Serial.println("Press 'y' to confirm or any other key to cancel: ");
        input = waitForInput(10000);
        if (input.length() > 0 && (input[0] == 'y' || input[0] == 'Y'))
        {
            runContinuousTransmissionTest();
        }
        else
        {
            Serial.println("Test cancelled");
        }
        Serial.println("\nPress any key to continue...");
        waitForInput(30000);
        break;

    case 0:
        showMainMenu();
        return;

    default:
        Serial.println("Invalid option");
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleGNSSMenu()
{
    Serial.println("\n--- GNSS Configuration ---");
    Serial.printf("GNSS: %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
    Serial.printf("Baud Rate: %lu\n", gnssConfig.baudRate);
    Serial.printf("Route to TCP: %s\n", gnssConfig.routeToTcp ? "Yes" : "No");
    Serial.printf("Route to USB: %s\n", gnssConfig.routeToUsb ? "Yes" : "No");
    Serial.printf("Synthesize on silence: %s\n", gnssConfig.synthesizeOnSilence ? "Yes" : "No");
    Serial.printf("Silence timeout: %lu seconds\n", gnssConfig.silenceTimeoutMs / 1000);
    Serial.printf("Verbose logging: %s\n", gnssConfig.verboseLogging ? "Yes" : "No");
    Serial.println();
    Serial.println("1. Toggle GNSS On/Off");
    Serial.println("2. Set Baud Rate");
    Serial.println("3. Toggle TCP routing");
    Serial.println("4. Toggle USB routing");
    Serial.println("5. Toggle synthesis on silence");
    Serial.println("6. Set silence timeout");
    Serial.println("7. Toggle verbose logging");
    Serial.println("8. Test GNSS module");
    Serial.println("0. Back to main menu");
    Serial.print("Select: ");

    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        Serial.println("\nTimeout - returning to main menu");
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        gnssConfig.enabled = !gnssConfig.enabled;
        Serial.printf("GNSS %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
        break;

    case 2:
    {
        input = promptForString("Enter baud rate [9600, 19200, 38400, 57600, 115200]: ");
        if (input.length() > 0)
        {
            uint32_t baud = parseInt(input, gnssConfig.baudRate);
            if (baud == 9600 || baud == 19200 || baud == 38400 || baud == 57600 || baud == 115200)
            {
                gnssConfig.baudRate = baud;
                Serial.printf("Baud rate set to: %lu\n", gnssConfig.baudRate);
            }
            else
            {
                Serial.println("Invalid baud rate");
            }
        }
    }
    break;

    case 3:
        gnssConfig.routeToTcp = !gnssConfig.routeToTcp;
        Serial.printf("TCP routing %s\n", gnssConfig.routeToTcp ? "Enabled" : "Disabled");
        break;

    case 4:
        gnssConfig.routeToUsb = !gnssConfig.routeToUsb;
        Serial.printf("USB routing %s\n", gnssConfig.routeToUsb ? "Enabled" : "Disabled");
        break;

    case 5:
        gnssConfig.synthesizeOnSilence = !gnssConfig.synthesizeOnSilence;
        Serial.printf("Synthesis on silence %s\n", gnssConfig.synthesizeOnSilence ? "Enabled" : "Disabled");
        break;

    case 6:
    {
        input = promptForString("Enter silence timeout (seconds): ");
        if (input.length() > 0)
        {
            uint32_t timeout = parseInt(input, gnssConfig.silenceTimeoutMs / 1000);
            if (timeout >= 5 && timeout <= 300)
            {
                gnssConfig.silenceTimeoutMs = timeout * 1000;
                Serial.printf("Silence timeout set to: %lu seconds\n", timeout);
            }
            else
            {
                Serial.println("Invalid timeout (5-300 seconds)");
            }
        }
    }
    break;

    case 7:
        gnssConfig.verboseLogging = !gnssConfig.verboseLogging;
        Serial.printf("GNSS verbose logging %s\n", gnssConfig.verboseLogging ? "Enabled" : "Disabled");
        break;

    case 8:
    {
        Serial.println("\n=== GNSS Module Test ===");
        Serial.println("Testing GNSS hardware and communication...");

        Serial.println("Watch the console for detailed GNSS diagnostics.");
        Serial.println("The enhanced polling will show detailed connection status.");

        delay(2000);
    }
    break;

    case 0:
        showMainMenu();
        return;

    default:
        Serial.println("Invalid option");
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleAPRSMenu()
{
    Serial.println("\n--- APRS Configuration ---");
    Serial.printf("Operating Mode: %s\n", aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
    Serial.printf("Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    Serial.printf("Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    Serial.printf("APRS Path: %s\n", aprsConfig.path);
    Serial.printf("Comment: %s\n", aprsConfig.comment);
    Serial.printf("Smart Beaconing: %s\n", aprsConfig.smartBeaconing ? "Enabled" : "Disabled");
    if (aprsConfig.smartBeaconing)
    {
        Serial.printf("  Fast Interval: %lu sec, Slow Interval: %lu sec\n",
                      aprsConfig.fastInterval, aprsConfig.slowInterval);
        Serial.printf("  Speed Threshold: %.1f km/h, Min Distance: %.0f m\n",
                      aprsConfig.speedThreshold, aprsConfig.minDistance);
    }
    Serial.println();
    Serial.println("=== Operating Mode ===");
    Serial.println("1. Toggle Operating Mode (TNC/APRS)");
    Serial.println("=== APRS Settings ===");
    Serial.println("2. Set Callsign");
    Serial.println("3. Set SSID (0-15)");
    Serial.println("4. Set Beacon Interval");
    Serial.println("5. Set APRS Path");
    Serial.println("6. Set Comment/Status");
    Serial.println("7. Set APRS Symbol");
    Serial.println("=== Smart Beaconing ===");
    Serial.println("8. Toggle Smart Beaconing");
    Serial.println("9. Set Fast/Slow Intervals");
    Serial.println("10. Set Movement Thresholds");
    Serial.println("=== Position Options ===");
    Serial.println("11. Toggle Altitude/Speed/Course");
    Serial.println("0. Back to main menu");
    Serial.print("Select: ");

    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        Serial.println("\nTimeout - returning to main menu");
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        aprsConfig.mode = (aprsConfig.mode == OperatingMode::TNC_MODE) ? OperatingMode::APRS_TRACKER : OperatingMode::TNC_MODE;
        Serial.printf("Operating mode set to: %s\n",
                      aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
        Serial.println("Note: Restart required for mode change to take effect");
        break;

    case 2:
        input = promptForString("Enter callsign (3-6 chars): ");
        if (input.length() >= 3 && input.length() <= 6)
        {
            input.toUpperCase();
            strncpy(aprsConfig.callsign, input.c_str(), sizeof(aprsConfig.callsign) - 1);
            aprsConfig.callsign[sizeof(aprsConfig.callsign) - 1] = '\0';
            Serial.printf("Callsign set to: %s\n", aprsConfig.callsign);
        }
        else
        {
            Serial.println("Invalid callsign length");
        }
        break;

    case 3:
    {
        input = promptForString("Enter SSID (0-15): ");
        if (input.length() > 0)
        {
            int ssid = parseInt(input, aprsConfig.ssid);
            if (ssid >= 0 && ssid <= 15)
            {
                aprsConfig.ssid = ssid;
                Serial.printf("SSID set to: %d\n", aprsConfig.ssid);
            }
            else
            {
                Serial.println("Invalid SSID (0-15)");
            }
        }
    }
    break;

    case 4:
    {
        input = promptForString("Enter beacon interval (30-3600 seconds): ");
        if (input.length() > 0)
        {
            uint32_t interval = parseInt(input, aprsConfig.beaconInterval);
            if (interval >= 30 && interval <= 3600)
            {
                aprsConfig.beaconInterval = interval;
                Serial.printf("Beacon interval set to: %lu seconds\n", aprsConfig.beaconInterval);
            }
            else
            {
                Serial.println("Invalid interval (30-3600 seconds)");
            }
        }
    }
    break;

    case 5:
        input = promptForString("Enter APRS path: ");
        if (input.length() > 0)
        {
            strncpy(aprsConfig.path, input.c_str(), sizeof(aprsConfig.path) - 1);
            aprsConfig.path[sizeof(aprsConfig.path) - 1] = '\0';
            Serial.printf("APRS path set to: %s\n", aprsConfig.path);
        }
        break;

    case 6:
        input = promptForString("Enter comment/status: ");
        if (input.length() > 0)
        {
            strncpy(aprsConfig.comment, input.c_str(), sizeof(aprsConfig.comment) - 1);
            aprsConfig.comment[sizeof(aprsConfig.comment) - 1] = '\0';
            Serial.printf("Comment set to: %s\n", aprsConfig.comment);
        }
        break;

    case 7:
        Serial.println("Symbol options: [>] Mobile, [-] House, [[] Person, [j] Jeep, [k] Truck");
        input = promptForString("Enter symbol character: ");
        if (input.length() > 0)
        {
            aprsConfig.symbol.symbol = input[0];
            Serial.printf("APRS symbol set to: /%c\n", aprsConfig.symbol.symbol);
        }
        break;

    case 8:
        aprsConfig.smartBeaconing = !aprsConfig.smartBeaconing;
        Serial.printf("Smart beaconing %s\n", aprsConfig.smartBeaconing ? "Enabled" : "Disabled");
        break;

    case 9:
    {
        input = promptForString("Enter fast interval (30-600 sec): ");
        if (input.length() > 0)
        {
            uint32_t fast = parseInt(input, aprsConfig.fastInterval);
            if (fast >= 30 && fast <= 600)
            {
                aprsConfig.fastInterval = fast;
            }
        }
        input = promptForString("Enter slow interval (600-3600 sec): ");
        if (input.length() > 0)
        {
            uint32_t slow = parseInt(input, aprsConfig.slowInterval);
            if (slow >= 600 && slow <= 3600)
            {
                aprsConfig.slowInterval = slow;
            }
        }
        Serial.printf("Intervals: Fast=%lu, Slow=%lu seconds\n",
                      aprsConfig.fastInterval, aprsConfig.slowInterval);
    }
    break;

    case 10:
    {
        input = promptForString("Enter speed threshold (km/h): ");
        if (input.length() > 0)
        {
            aprsConfig.speedThreshold = parseFloat(input, aprsConfig.speedThreshold);
        }
        input = promptForString("Enter min distance (meters): ");
        if (input.length() > 0)
        {
            aprsConfig.minDistance = parseFloat(input, aprsConfig.minDistance);
        }
        Serial.printf("Thresholds: Speed=%.1f km/h, Distance=%.0f m\n",
                      aprsConfig.speedThreshold, aprsConfig.minDistance);
    }
    break;

    case 11:
        aprsConfig.includeAltitude = !aprsConfig.includeAltitude;
        aprsConfig.includeSpeed = !aprsConfig.includeSpeed;
        aprsConfig.includeCourse = !aprsConfig.includeCourse;
        Serial.printf("Position extras: Alt=%s, Speed=%s, Course=%s\n",
                      aprsConfig.includeAltitude ? "Yes" : "No",
                      aprsConfig.includeSpeed ? "Yes" : "No",
                      aprsConfig.includeCourse ? "Yes" : "No");
        break;

    case 0:
        showMainMenu();
        return;

    default:
        Serial.println("Invalid option");
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleBatteryMenu()
{
    Serial.println("\n--- Battery Configuration ---");
    Serial.printf("Debug Messages: %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");
    Serial.println();
    Serial.println("1. Toggle Debug Messages");
    Serial.println("0. Back to main menu");
    Serial.print("Select: ");

    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        Serial.println("\nTimeout - returning to main menu");
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        batteryConfig.debugMessages = !batteryConfig.debugMessages;
        Serial.printf("Battery debug messages %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");
        break;

    case 0:
        showMainMenu();
        return;

    default:
        Serial.println("Invalid option");
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::showCurrentConfig()
{
    Serial.println("\n========== Current Configuration ==========");

    Serial.println("WiFi:");
    Serial.printf("  Mode: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
    Serial.printf("  AP SSID: %s\n", wifiConfig.ssid);
    Serial.printf("  STA SSID: %s\n", wifiConfig.sta_ssid);

    Serial.println("\nRadio:");
    Serial.printf("  Frequency: %.3f MHz\n", radioConfig.frequency);
    Serial.printf("  Bandwidth: %.1f kHz\n", radioConfig.bandwidth);
    Serial.printf("  Spreading Factor: %d\n", radioConfig.spreadingFactor);
    Serial.printf("  Coding Rate: 4/%d\n", radioConfig.codingRate);
    Serial.printf("  TX Power: %d dBm\n", radioConfig.txPower);
    Serial.printf("  TX Delay: %d x 10ms\n", radioConfig.txDelay);
    Serial.printf("  Persistence: %d\n", radioConfig.persist);
    Serial.printf("  Slot Time: %d x 10ms\n", radioConfig.slotTime);

#if GNSS_ENABLE
    Serial.println("\nGNSS:");
    Serial.printf("  Status: %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
    Serial.printf("  Baud Rate: %lu\n", gnssConfig.baudRate);
    Serial.printf("  Route to TCP: %s\n", gnssConfig.routeToTcp ? "Yes" : "No");
    Serial.printf("  Route to USB: %s\n", gnssConfig.routeToUsb ? "Yes" : "No");
    Serial.printf("  Synthesize on silence: %s\n", gnssConfig.synthesizeOnSilence ? "Yes" : "No");
    Serial.printf("  Silence timeout: %lu seconds\n", gnssConfig.silenceTimeoutMs / 1000);
    Serial.printf("  Verbose logging: %s\n", gnssConfig.verboseLogging ? "Yes" : "No");
    Serial.printf("  V4 GNSS Pins: RX=38, TX=39\n");

    Serial.println("\nAPRS:");
    Serial.printf("  Operating Mode: %s\n", aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
    Serial.printf("  Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    Serial.printf("  Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    Serial.printf("  APRS Path: %s\n", aprsConfig.path);
    Serial.printf("  Comment: %s\n", aprsConfig.comment);
    Serial.printf("  Symbol: /%c\n", aprsConfig.symbol.symbol);
    Serial.printf("  Smart Beaconing: %s\n", aprsConfig.smartBeaconing ? "Yes" : "No");
    if (aprsConfig.smartBeaconing)
    {
        Serial.printf("  Fast/Slow Intervals: %lu/%lu seconds\n", aprsConfig.fastInterval, aprsConfig.slowInterval);
        Serial.printf("  Speed Threshold: %.1f km/h\n", aprsConfig.speedThreshold);
        Serial.printf("  Min Distance: %.0f meters\n", aprsConfig.minDistance);
    }
#else
    Serial.println("\nGNSS: Not available (disabled in build configuration)");
    Serial.println("APRS: Not available (requires GNSS support)");
#endif

    Serial.println("\nBattery:");
    Serial.printf("  Debug Messages: %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");

    Serial.println("==========================================");
}

float ConfigManager::parseFloat(const String &str, float defaultVal)
{
    if (str.length() == 0)
        return defaultVal;
    float val = str.toFloat();
    return (val == 0.0 && str != "0" && str != "0.0") ? defaultVal : val;
}

int ConfigManager::parseInt(const String &str, int defaultVal)
{
    if (str.length() == 0)
        return defaultVal;
    int val = str.toInt();
    return (val == 0 && str != "0") ? defaultVal : val;
}

bool ConfigManager::parseYesNo(const String &str)
{
    String lower = str;
    lower.toLowerCase();
    return (lower == "y" || lower == "yes" || lower == "1" || lower == "true");
}

String ConfigManager::waitForInput(unsigned long timeoutMs)
{
    String input = "";
    unsigned long startTime = millis();

    while (Serial.available())
    {
        Serial.read();
    }

    while (millis() - startTime < timeoutMs)
    {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c == '\n' || c == '\r')
            {
                Serial.println();
                input.trim();
                return input;
            }
            else if (c == 8 || c == 127)
            {
                if (input.length() > 0)
                {
                    input.remove(input.length() - 1);
                    Serial.print("\b \b");
                }
            }
            else if (c >= 32 && c <= 126)
            {
                input += c;
                Serial.print(c);
            }
        }
        delay(1);
    }

    return "";
}

String ConfigManager::promptForString(const String &prompt, unsigned long timeoutMs)
{
    Serial.print(prompt);
    String result = waitForInput(timeoutMs);
    if (result.length() == 0)
    {
        Serial.println("\nTimeout - using previous value");
    }
    return result;
}

void ConfigManager::powerOffDevice()
{
    Serial.println("\n=== POWER OFF ===");
    Serial.println("Shutting down device...");
    Serial.println("- Saving current configuration");
    saveConfig();

    Serial.println("- Stopping WiFi");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.println("- Powering off radios");
    btStop();

    Serial.println("- Turning off OLED display");
    pinMode(36, OUTPUT);
    digitalWrite(36, HIGH);

    Serial.println("- Entering deep sleep mode");
    Serial.println("Device will power off in 3 seconds...");
    Serial.flush();

    delay(3000);

    esp_deep_sleep_start();
}

void __attribute__((weak)) runRadioHealthCheckImpl()
{
    Serial.println("Radio diagnostics not available - functions not linked");
}

void __attribute__((weak)) runHardwarePinCheckImpl()
{
    Serial.println("Hardware pin diagnostics not available - functions not linked");
}

void __attribute__((weak)) runTransmissionTestImpl()
{
    Serial.println("Transmission test not available - functions not linked");
}

void __attribute__((weak)) runContinuousTransmissionTestImpl()
{
    Serial.println("Continuous transmission test not available - functions not linked");
}

void ConfigManager::runRadioHealthCheck()
{
    runRadioHealthCheckImpl();
}

void ConfigManager::runHardwarePinCheck()
{
    runHardwarePinCheckImpl();
}

void ConfigManager::runTransmissionTest()
{
    runTransmissionTestImpl();
}

void ConfigManager::runContinuousTransmissionTest()
{
    runContinuousTransmissionTestImpl();
}