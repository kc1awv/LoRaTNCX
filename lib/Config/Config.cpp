#include "Config.h"
#include <WiFi.h>
#include "esp_sleep.h"
#include "esp_bt.h"

ConfigManager config;

bool ConfigManager::begin()
{
    if (!prefs.begin("tnc-config", false))
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[CONFIG] Failed to initialize NVS");
        #endif
        return false;
    }

    loadConfig();
    #ifndef KISS_SERIAL_MODE
    Serial.println("[CONFIG] Configuration manager initialized");
    #endif
    return true;
}

void ConfigManager::loadConfig()
{
    prefs.getBytes("wifi", &wifiConfig, sizeof(WiFiConfig));

    prefs.getBytes("radio", &radioConfig, sizeof(RadioConfig));

    prefs.getBytes("gnss", &gnssConfig, sizeof(GNSSConfig));

    prefs.getBytes("aprs", &aprsConfig, sizeof(APRSConfig));

    prefs.getBytes("battery", &batteryConfig, sizeof(BatteryConfig));

    #ifndef KISS_SERIAL_MODE
    Serial.println("[CONFIG] Configuration loaded from NVS");
    #endif
}

void ConfigManager::saveConfig()
{
    prefs.putBytes("wifi", &wifiConfig, sizeof(WiFiConfig));
    prefs.putBytes("radio", &radioConfig, sizeof(RadioConfig));
    prefs.putBytes("gnss", &gnssConfig, sizeof(GNSSConfig));
    prefs.putBytes("aprs", &aprsConfig, sizeof(APRSConfig));
    prefs.putBytes("battery", &batteryConfig, sizeof(BatteryConfig));

    #ifndef KISS_SERIAL_MODE
    Serial.println("[CONFIG] Configuration saved to NVS");
    #endif
}

void ConfigManager::resetToDefaults()
{
    wifiConfig = WiFiConfig{};
    radioConfig = RadioConfig{};
    gnssConfig = GNSSConfig{};
    aprsConfig = APRSConfig{};
    batteryConfig = BatteryConfig{};

    saveConfig();
    #ifndef KISS_SERIAL_MODE
    Serial.println("[CONFIG] Reset to default configuration");
    #endif
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
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n========== TNC Configuration Menu ==========");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. WiFi Settings");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("2. Radio Settings");
    #endif
#if GNSS_ENABLE
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. GNSS Settings");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. APRS Settings & Operating Mode");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Battery Settings");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("6. Show Current Configuration");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("7. Save & Exit");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("8. Reset to Defaults");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("9. Power Off Device");
    #endif
#else
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. Battery Settings");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. Show Current Configuration");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Save & Exit");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("6. Reset to Defaults");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("7. Power Off Device");
    #endif
#endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Exit without Saving");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("==========================================");
    #endif
#if !GNSS_ENABLE
    #ifndef KISS_SERIAL_MODE
    Serial.println("Note: GNSS/APRS options not available (disabled in build)");
    #endif
#endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select option: ");
    #endif
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
            #ifndef KISS_SERIAL_MODE
            Serial.println();
            #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.println("Configuration saved. Restarting...");
                #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.println("Configuration saved. Restarting...");
                #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.println("Exited configuration menu");
                #endif
                break;
            default:
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid option");
                #endif
                showMainMenu();
                break;
            }
        }
        else if (c == 8 || c == 127)
        {
            if (inputBuffer.length() > 0)
            {
                inputBuffer.remove(inputBuffer.length() - 1);
                #ifndef KISS_SERIAL_MODE
                Serial.print("\b \b");
                #endif
            }
        }
        else if (c >= 32 && c <= 126)
        {
            inputBuffer += c;
            #ifndef KISS_SERIAL_MODE
            Serial.print(c);
            #endif
        }
    }
}

void ConfigManager::handleWiFiMenu()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n--- WiFi Configuration ---");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Current mode: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("AP SSID: %s\n", wifiConfig.ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("STA SSID: %s\n", wifiConfig.sta_ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println();
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. Toggle AP/STA mode");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("2. Set AP SSID");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. Set AP Password");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. Set STA SSID");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Set STA Password");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Back to main menu");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select: ");
    #endif
    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - returning to main menu");
        #endif
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        wifiConfig.useAP = !wifiConfig.useAP;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("WiFi mode set to: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
        #endif
        break;

    case 2:
        input = promptForString("Enter AP SSID: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.ssid, input.c_str(), sizeof(wifiConfig.ssid) - 1);
            wifiConfig.ssid[sizeof(wifiConfig.ssid) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.printf("AP SSID set to: %s\n", wifiConfig.ssid);
            #endif
        }
        break;

    case 3:
        input = promptForString("Enter AP Password: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.password, input.c_str(), sizeof(wifiConfig.password) - 1);
            wifiConfig.password[sizeof(wifiConfig.password) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.println("AP Password updated");
            #endif
        }
        break;

    case 4:
        input = promptForString("Enter STA SSID: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.sta_ssid, input.c_str(), sizeof(wifiConfig.sta_ssid) - 1);
            wifiConfig.sta_ssid[sizeof(wifiConfig.sta_ssid) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.printf("STA SSID set to: %s\n", wifiConfig.sta_ssid);
            #endif
        }
        break;

    case 5:
        input = promptForString("Enter STA Password: ");
        if (input.length() > 0)
        {
            strncpy(wifiConfig.sta_password, input.c_str(), sizeof(wifiConfig.sta_password) - 1);
            wifiConfig.sta_password[sizeof(wifiConfig.sta_password) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.println("STA Password updated");
            #endif
        }
        break;

    case 0:
        showMainMenu();
        return;

    default:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Invalid option");
        #endif
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleRadioMenu()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n--- Radio Configuration ---");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Frequency: %.3f MHz\n", radioConfig.frequency);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Bandwidth: %.1f kHz\n", radioConfig.bandwidth);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Spreading Factor: %d\n", radioConfig.spreadingFactor);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Coding Rate: 4/%d\n", radioConfig.codingRate);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("TX Power: %d dBm\n", radioConfig.txPower);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println();
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== Configuration ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. Set Frequency (MHz)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("2. Set Bandwidth (kHz)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. Set Spreading Factor (7-12)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. Set Coding Rate (5-8)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Set TX Power (dBm)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== Diagnostics ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("6. Run Radio Health Check");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("7. Check Hardware Pins");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("8. Run Transmission Test");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("9. Run Continuous TX Test (30s)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Back to main menu");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select: ");
    #endif
    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - returning to main menu");
        #endif
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
            #ifndef KISS_SERIAL_MODE
            Serial.printf("Frequency set to: %.3f MHz\n", radioConfig.frequency);
            #endif
        }
        break;

    case 2:
        input = promptForString("Enter bandwidth (kHz) [7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500]: ");
        if (input.length() > 0)
        {
            radioConfig.bandwidth = parseFloat(input, radioConfig.bandwidth);
            #ifndef KISS_SERIAL_MODE
            Serial.printf("Bandwidth set to: %.1f kHz\n", radioConfig.bandwidth);
            #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("Spreading factor set to: %d\n", radioConfig.spreadingFactor);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid spreading factor (7-12)");
                #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("Coding rate set to: 4/%d\n", radioConfig.codingRate);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid coding rate (5-8)");
                #endif
            }
        }
    }
    break;

    case 5:
        input = promptForString("Enter TX power (dBm): ");
        if (input.length() > 0)
        {
            radioConfig.txPower = parseInt(input, radioConfig.txPower);
            #ifndef KISS_SERIAL_MODE
            Serial.printf("TX power set to: %d dBm\n", radioConfig.txPower);
            #endif
        }
        break;

    case 6:
        #ifndef KISS_SERIAL_MODE
        Serial.println("\n🔍 Running radio health check...");
        #endif
        runRadioHealthCheck();
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nPress any key to continue...");
        #endif
        waitForInput(30000);
        break;

    case 7:
        #ifndef KISS_SERIAL_MODE
        Serial.println("\n🔧 Checking hardware pins...");
        #endif
        runHardwarePinCheck();
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nPress any key to continue...");
        #endif
        waitForInput(30000);
        break;

    case 8:
        #ifndef KISS_SERIAL_MODE
        Serial.println("\n🧪 Running transmission test...");
        #endif
        runTransmissionTest();
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nPress any key to continue...");
        #endif
        waitForInput(30000);
        break;

    case 9:
        #ifndef KISS_SERIAL_MODE
        Serial.println("\n🎯 Running continuous transmission test...");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("WARNING: This will transmit for 30 seconds!");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("Make sure your SDR is monitoring the frequency!");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("Press 'y' to confirm or any other key to cancel: ");
        #endif
        input = waitForInput(10000);
        if (input.length() > 0 && (input[0] == 'y' || input[0] == 'Y'))
        {
            runContinuousTransmissionTest();
        }
        else
        {
            #ifndef KISS_SERIAL_MODE
            Serial.println("Test cancelled");
            #endif
        }
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nPress any key to continue...");
        #endif
        waitForInput(30000);
        break;

    case 0:
        showMainMenu();
        return;

    default:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Invalid option");
        #endif
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleGNSSMenu()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n--- GNSS Configuration ---");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("GNSS: %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Baud Rate: %lu\n", gnssConfig.baudRate);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Route to TCP: %s\n", gnssConfig.routeToTcp ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Route to USB: %s\n", gnssConfig.routeToUsb ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Synthesize on silence: %s\n", gnssConfig.synthesizeOnSilence ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Silence timeout: %lu seconds\n", gnssConfig.silenceTimeoutMs / 1000);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Verbose logging: %s\n", gnssConfig.verboseLogging ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println();
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. Toggle GNSS On/Off");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("2. Set Baud Rate");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. Toggle TCP routing");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. Toggle USB routing");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Toggle synthesis on silence");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("6. Set silence timeout");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("7. Toggle verbose logging");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("8. Test GNSS module");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Back to main menu");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select: ");
    #endif
    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - returning to main menu");
        #endif
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        gnssConfig.enabled = !gnssConfig.enabled;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("GNSS %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
        #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("Baud rate set to: %lu\n", gnssConfig.baudRate);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid baud rate");
                #endif
            }
        }
    }
    break;

    case 3:
        gnssConfig.routeToTcp = !gnssConfig.routeToTcp;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("TCP routing %s\n", gnssConfig.routeToTcp ? "Enabled" : "Disabled");
        #endif
        break;

    case 4:
        gnssConfig.routeToUsb = !gnssConfig.routeToUsb;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("USB routing %s\n", gnssConfig.routeToUsb ? "Enabled" : "Disabled");
        #endif
        break;

    case 5:
        gnssConfig.synthesizeOnSilence = !gnssConfig.synthesizeOnSilence;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Synthesis on silence %s\n", gnssConfig.synthesizeOnSilence ? "Enabled" : "Disabled");
        #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("Silence timeout set to: %lu seconds\n", timeout);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid timeout (5-300 seconds)");
                #endif
            }
        }
    }
    break;

    case 7:
        gnssConfig.verboseLogging = !gnssConfig.verboseLogging;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("GNSS verbose logging %s\n", gnssConfig.verboseLogging ? "Enabled" : "Disabled");
        #endif
        break;

    case 8:
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\n=== GNSS Module Test ===");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("Testing GNSS hardware and communication...");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("Watch the console for detailed GNSS diagnostics.");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("The enhanced polling will show detailed connection status.");
        #endif
        delay(2000);
    }
    break;

    case 0:
        showMainMenu();
        return;

    default:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Invalid option");
        #endif
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleAPRSMenu()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n--- APRS Configuration ---");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Operating Mode: %s\n", aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("APRS Path: %s\n", aprsConfig.path);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Comment: %s\n", aprsConfig.comment);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Smart Beaconing: %s\n", aprsConfig.smartBeaconing ? "Enabled" : "Disabled");
    #endif
    if (aprsConfig.smartBeaconing)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  Fast Interval: %lu sec, Slow Interval: %lu sec\n",
                      aprsConfig.fastInterval, aprsConfig.slowInterval);
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  Speed Threshold: %.1f km/h, Min Distance: %.0f m\n",
                      aprsConfig.speedThreshold, aprsConfig.minDistance);
        #endif
    }
    #ifndef KISS_SERIAL_MODE
    Serial.println();
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== Operating Mode ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. Toggle Operating Mode (TNC/APRS)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== APRS Settings ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("2. Set Callsign");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("3. Set SSID (0-15)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("4. Set Beacon Interval");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("5. Set APRS Path");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("6. Set Comment/Status");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("7. Set APRS Symbol");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== Smart Beaconing ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("8. Toggle Smart Beaconing");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("9. Set Fast/Slow Intervals");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("10. Set Movement Thresholds");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("=== Position Options ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("11. Toggle Altitude/Speed/Course");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Back to main menu");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select: ");
    #endif
    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - returning to main menu");
        #endif
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        aprsConfig.mode = (aprsConfig.mode == OperatingMode::TNC_MODE) ? OperatingMode::APRS_TRACKER : OperatingMode::TNC_MODE;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Operating mode set to: %s\n",
                      aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("Note: Restart required for mode change to take effect");
        #endif
        break;

    case 2:
        input = promptForString("Enter callsign (3-6 chars): ");
        if (input.length() >= 3 && input.length() <= 6)
        {
            input.toUpperCase();
            strncpy(aprsConfig.callsign, input.c_str(), sizeof(aprsConfig.callsign) - 1);
            aprsConfig.callsign[sizeof(aprsConfig.callsign) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.printf("Callsign set to: %s\n", aprsConfig.callsign);
            #endif
        }
        else
        {
            #ifndef KISS_SERIAL_MODE
            Serial.println("Invalid callsign length");
            #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("SSID set to: %d\n", aprsConfig.ssid);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid SSID (0-15)");
                #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.printf("Beacon interval set to: %lu seconds\n", aprsConfig.beaconInterval);
                #endif
            }
            else
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("Invalid interval (30-3600 seconds)");
                #endif
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
            #ifndef KISS_SERIAL_MODE
            Serial.printf("APRS path set to: %s\n", aprsConfig.path);
            #endif
        }
        break;

    case 6:
        input = promptForString("Enter comment/status: ");
        if (input.length() > 0)
        {
            strncpy(aprsConfig.comment, input.c_str(), sizeof(aprsConfig.comment) - 1);
            aprsConfig.comment[sizeof(aprsConfig.comment) - 1] = '\0';
            #ifndef KISS_SERIAL_MODE
            Serial.printf("Comment set to: %s\n", aprsConfig.comment);
            #endif
        }
        break;

    case 7:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Symbol options: [>] Mobile, [-] House, [[] Person, [j] Jeep, [k] Truck");
        #endif
        input = promptForString("Enter symbol character: ");
        if (input.length() > 0)
        {
            aprsConfig.symbol.symbol = input[0];
            #ifndef KISS_SERIAL_MODE
            Serial.printf("APRS symbol set to: /%c\n", aprsConfig.symbol.symbol);
            #endif
        }
        break;

    case 8:
        aprsConfig.smartBeaconing = !aprsConfig.smartBeaconing;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Smart beaconing %s\n", aprsConfig.smartBeaconing ? "Enabled" : "Disabled");
        #endif
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
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Intervals: Fast=%lu, Slow=%lu seconds\n",
                      aprsConfig.fastInterval, aprsConfig.slowInterval);
        #endif
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
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Thresholds: Speed=%.1f km/h, Distance=%.0f m\n",
                      aprsConfig.speedThreshold, aprsConfig.minDistance);
        #endif
    }
    break;

    case 11:
        aprsConfig.includeAltitude = !aprsConfig.includeAltitude;
        aprsConfig.includeSpeed = !aprsConfig.includeSpeed;
        aprsConfig.includeCourse = !aprsConfig.includeCourse;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Position extras: Alt=%s, Speed=%s, Course=%s\n",
                      aprsConfig.includeAltitude ? "Yes" : "No",
                      aprsConfig.includeSpeed ? "Yes" : "No",
                      aprsConfig.includeCourse ? "Yes" : "No");
        #endif
        break;

    case 0:
        showMainMenu();
        return;

    default:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Invalid option");
        #endif
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::handleBatteryMenu()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n--- Battery Configuration ---");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("Debug Messages: %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println();
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("1. Toggle Debug Messages");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("0. Back to main menu");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("Select: ");
    #endif
    String input = waitForInput(30000);
    if (input.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - returning to main menu");
        #endif
        showMainMenu();
        return;
    }

    int choice = input.toInt();

    switch (choice)
    {
    case 1:
        batteryConfig.debugMessages = !batteryConfig.debugMessages;
        #ifndef KISS_SERIAL_MODE
        Serial.printf("Battery debug messages %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");
        #endif
        break;

    case 0:
        showMainMenu();
        return;

    default:
        #ifndef KISS_SERIAL_MODE
        Serial.println("Invalid option");
        #endif
        break;
    }

    delay(1000);
    showMainMenu();
}

void ConfigManager::showCurrentConfig()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n========== Current Configuration ==========");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("WiFi:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Mode: %s\n", wifiConfig.useAP ? "Access Point" : "Station");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  AP SSID: %s\n", wifiConfig.ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  STA SSID: %s\n", wifiConfig.sta_ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("\nRadio:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Frequency: %.3f MHz\n", radioConfig.frequency);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Bandwidth: %.1f kHz\n", radioConfig.bandwidth);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Spreading Factor: %d\n", radioConfig.spreadingFactor);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Coding Rate: 4/%d\n", radioConfig.codingRate);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  TX Power: %d dBm\n", radioConfig.txPower);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  TX Delay: %d x 10ms\n", radioConfig.txDelay);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Persistence: %d\n", radioConfig.persist);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Slot Time: %d x 10ms\n", radioConfig.slotTime);
    #endif
#if GNSS_ENABLE
    #ifndef KISS_SERIAL_MODE
    Serial.println("\nGNSS:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Status: %s\n", gnssConfig.enabled ? "Enabled" : "Disabled");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Baud Rate: %lu\n", gnssConfig.baudRate);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Route to TCP: %s\n", gnssConfig.routeToTcp ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Route to USB: %s\n", gnssConfig.routeToUsb ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Synthesize on silence: %s\n", gnssConfig.synthesizeOnSilence ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Silence timeout: %lu seconds\n", gnssConfig.silenceTimeoutMs / 1000);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Verbose logging: %s\n", gnssConfig.verboseLogging ? "Yes" : "No");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  V4 GNSS Pins: RX=38, TX=39\n");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("\nAPRS:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Operating Mode: %s\n", aprsConfig.mode == OperatingMode::TNC_MODE ? "KISS TNC" : "APRS Tracker");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  APRS Path: %s\n", aprsConfig.path);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Comment: %s\n", aprsConfig.comment);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Symbol: /%c\n", aprsConfig.symbol.symbol);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Smart Beaconing: %s\n", aprsConfig.smartBeaconing ? "Yes" : "No");
    #endif
    if (aprsConfig.smartBeaconing)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  Fast/Slow Intervals: %lu/%lu seconds\n", aprsConfig.fastInterval, aprsConfig.slowInterval);
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  Speed Threshold: %.1f km/h\n", aprsConfig.speedThreshold);
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  Min Distance: %.0f meters\n", aprsConfig.minDistance);
        #endif
    }
#else
    #ifndef KISS_SERIAL_MODE
    Serial.println("\nGNSS: Not available (disabled in build configuration)");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("APRS: Not available (requires GNSS support)");
    #endif
#endif

    #ifndef KISS_SERIAL_MODE
    Serial.println("\nBattery:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("  Debug Messages: %s\n", batteryConfig.debugMessages ? "Enabled" : "Disabled");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("==========================================");
    #endif
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
                #ifndef KISS_SERIAL_MODE
                Serial.println();
                #endif
                input.trim();
                return input;
            }
            else if (c == 8 || c == 127)
            {
                if (input.length() > 0)
                {
                    input.remove(input.length() - 1);
                    #ifndef KISS_SERIAL_MODE
                    Serial.print("\b \b");
                    #endif
                }
            }
            else if (c >= 32 && c <= 126)
            {
                input += c;
                #ifndef KISS_SERIAL_MODE
                Serial.print(c);
                #endif
            }
        }
        delay(1);
    }

    return "";
}

String ConfigManager::promptForString(const String &prompt, unsigned long timeoutMs)
{
    #ifndef KISS_SERIAL_MODE
    Serial.print(prompt);
    #endif
    String result = waitForInput(timeoutMs);
    if (result.length() == 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("\nTimeout - using previous value");
        #endif
    }
    return result;
}

void ConfigManager::powerOffDevice()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("\n=== POWER OFF ===");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("Shutting down device...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("- Saving current configuration");
    #endif
    saveConfig();

    #ifndef KISS_SERIAL_MODE
    Serial.println("- Stopping WiFi");
    #endif
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    #ifndef KISS_SERIAL_MODE
    Serial.println("- Powering off radios");
    #endif
    btStop();

    #ifndef KISS_SERIAL_MODE
    Serial.println("- Turning off OLED display");
    #endif
    pinMode(36, OUTPUT);
    digitalWrite(36, HIGH);

    #ifndef KISS_SERIAL_MODE
    Serial.println("- Entering deep sleep mode");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("Device will power off in 3 seconds...");
    #endif
    Serial.flush();

    delay(3000);

    esp_deep_sleep_start();
}

void __attribute__((weak)) runRadioHealthCheckImpl()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("Radio diagnostics not available - functions not linked");
    #endif
}

void __attribute__((weak)) runHardwarePinCheckImpl()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("Hardware pin diagnostics not available - functions not linked");
    #endif
}

void __attribute__((weak)) runTransmissionTestImpl()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("Transmission test not available - functions not linked");
    #endif
}

void __attribute__((weak)) runContinuousTransmissionTestImpl()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("Continuous transmission test not available - functions not linked");
    #endif
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