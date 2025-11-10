#include "display.h"
#include "board_config.h"

// Global instances
DisplayManager displayManager;
volatile bool buttonPressed = false;

// Interrupt handler for button press
void IRAM_ATTR buttonInterruptHandler() {
    buttonPressed = true;
}

// Constructor
DisplayManager::DisplayManager() 
    : u8g2(U8G2_R0, /* reset=*/ RST_OLED, /* clock=*/ SCL_OLED, /* data=*/ SDA_OLED),
      currentScreen(SCREEN_BOOT),
      lastScreen(SCREEN_BOOT),
      bootScreenActive(false),
      bootScreenStartTime(0),
      bootScreenDuration(2000),
      radioFreq(LORA_FREQUENCY),
      radioBW(LORA_BANDWIDTH),
      radioSF(LORA_SPREADING),
      radioCR(LORA_CODINGRATE),
      radioPower(LORA_POWER),
      radioSyncWord(LORA_SYNCWORD),
      batteryVoltage(0.0),
      gnssEnabled(false),
      gnssHasFix(false),
      gnssLatitude(0.0),
      gnssLongitude(0.0),
      gnssSatellites(0),
      gnssClients(0),
      lastButtonPress(0)
{
}

void DisplayManager::begin() {
    // Enable Vext power for OLED
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);  // Vext active LOW
    delay(100);
    
    // Initialize display
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    
    // Show boot screen
    showBootScreen();
}

void DisplayManager::showBootScreen(uint32_t duration_ms) {
    bootScreenActive = true;
    bootScreenStartTime = millis();
    bootScreenDuration = duration_ms;
    currentScreen = SCREEN_BOOT;
    lastScreen = SCREEN_STATUS;  // Force re-render by making it different
    
    // Immediately render and display the boot screen
    u8g2.clearBuffer();
    renderBootScreen();
    u8g2.sendBuffer();
}

void DisplayManager::update() {
    // Check if boot screen should expire
    if (bootScreenActive) {
        if (millis() - bootScreenStartTime >= bootScreenDuration) {
            bootScreenActive = false;
            currentScreen = SCREEN_STATUS;  // Move to status screen after boot
            lastScreen = SCREEN_BOOT;  // Force re-render
        }
    }
    
    // Don't update if display is off
    if (currentScreen == SCREEN_OFF) {
        return;
    }
    
    // Only update display if screen changed
    if (currentScreen != lastScreen) {
        u8g2.clearBuffer();
        
        switch (currentScreen) {
            case SCREEN_BOOT:
                renderBootScreen();
                break;
            case SCREEN_WIFI_STARTUP:
                renderWiFiStartupScreen();
                break;
            case SCREEN_STATUS:
                renderStatusScreen();
                break;
            case SCREEN_WIFI:
                renderWiFiScreen();
                break;
            case SCREEN_BATTERY:
                renderBatteryScreen();
                break;
            case SCREEN_GNSS:
                renderGNSSScreen();
                break;
            case SCREEN_OFF:
                renderOffScreen();
                break;
            default:
                break;
        }
        
        u8g2.sendBuffer();
        lastScreen = currentScreen;
    }
}

void DisplayManager::setScreen(DisplayScreen screen) {
    if (!bootScreenActive && screen != currentScreen) {
        currentScreen = screen;
    }
}

void DisplayManager::nextScreen() {
    if (bootScreenActive) {
        // Skip boot screen if button pressed
        bootScreenActive = false;
        currentScreen = SCREEN_STATUS;
        return;
    }
    
    // If display is off, turn it back on
    if (currentScreen == SCREEN_OFF) {
        displayOn();
        currentScreen = SCREEN_STATUS;
        return;
    }
    
    // Cycle through screens: STATUS -> WIFI -> BATTERY -> GNSS -> OFF -> STATUS
    switch (currentScreen) {
        case SCREEN_STATUS:
            currentScreen = SCREEN_WIFI;
            break;
        case SCREEN_WIFI:
            currentScreen = SCREEN_BATTERY;
            break;
        case SCREEN_BATTERY:
            currentScreen = SCREEN_GNSS;
            break;
        case SCREEN_GNSS:
            currentScreen = SCREEN_OFF;
            break;
        case SCREEN_OFF:
            currentScreen = SCREEN_STATUS;
            break;
        default:
            currentScreen = SCREEN_STATUS;
            break;
    }
}

void DisplayManager::handleButtonPress() {
    uint32_t now = millis();
    
    // Debounce
    if (now - lastButtonPress < BUTTON_DEBOUNCE_MS) {
        return;
    }
    
    lastButtonPress = now;
    
    // Check for long press (power off feature)
    uint32_t pressStart = now;
    uint32_t pressDuration = 0;
    
    // Wait while button is held, checking duration
    while (digitalRead(0) == LOW && pressDuration < BUTTON_LONG_PRESS_MS + 100) {
        delay(50);
        pressDuration = millis() - pressStart;
    }
    
    if (pressDuration >= BUTTON_LONG_PRESS_MS) {
        // Long press detected - power off
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(10, 32, "Powering Off...");
        u8g2.sendBuffer();
        delay(1000);
        
        // Turn off display
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        
        // Disable Vext (powers off OLED and other peripherals)
        digitalWrite(Vext, HIGH);
        
        delay(100);
        
        // Enter deep sleep (no wake sources configured = permanent sleep until reset)
        esp_deep_sleep_start();
    } else {
        // Short press - cycle screens
        nextScreen();
    }
}

void DisplayManager::setRadioConfig(float freq, float bw, uint8_t sf, uint8_t cr, int8_t pwr, uint16_t sw) {
    radioFreq = freq;
    radioBW = bw;
    radioSF = sf;
    radioCR = cr;
    radioPower = pwr;
    radioSyncWord = sw;
    
    // Trigger update if on status screen
    if (currentScreen == SCREEN_STATUS && !bootScreenActive) {
        lastScreen = SCREEN_BOOT;  // Force re-render
    }
}

void DisplayManager::setBatteryVoltage(float voltage) {
    batteryVoltage = voltage;
    
    // Trigger update if on battery screen
    if (currentScreen == SCREEN_BATTERY && !bootScreenActive) {
        lastScreen = SCREEN_BOOT;  // Force re-render
    }
}

void DisplayManager::setWiFiStatus(bool apActive, bool staConnected, String apIP, String staIP, int rssi) {
    wifiAPActive = apActive;
    wifiSTAConnected = staConnected;
    wifiAPIP = apIP;
    wifiSTAIP = staIP;
    wifiRSSI = rssi;
    
    // Trigger update if on WiFi screen
    if (currentScreen == SCREEN_WIFI && !bootScreenActive) {
        lastScreen = SCREEN_BOOT;  // Force re-render
    }
}

void DisplayManager::setWiFiStartupMessage(String message) {
    wifiStartupMessage = message;
    
    // Trigger update if on WiFi startup screen
    if (currentScreen == SCREEN_WIFI_STARTUP) {
        lastScreen = SCREEN_BOOT;  // Force re-render
    }
}

void DisplayManager::setGNSSStatus(bool enabled, bool hasFix, double lat, double lon, uint8_t sats, uint8_t clients) {
    gnssEnabled = enabled;
    gnssHasFix = hasFix;
    gnssLatitude = lat;
    gnssLongitude = lon;
    gnssSatellites = sats;
    gnssClients = clients;
    
    // Trigger update if on GNSS screen
    if (currentScreen == SCREEN_GNSS && !bootScreenActive) {
        lastScreen = SCREEN_BOOT;  // Force re-render
    }
}

void DisplayManager::displayOff() {
    currentScreen = SCREEN_OFF;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    // Turn off Vext power to OLED
    digitalWrite(Vext, HIGH);
}

void DisplayManager::displayOn() {
    // Turn on Vext power to OLED
    digitalWrite(Vext, LOW);
    delay(100);
    // Reinitialize display
    u8g2.begin();
    lastScreen = SCREEN_BOOT;  // Force re-render
}

bool DisplayManager::isBootScreenActive() {
    return bootScreenActive;
}

void DisplayManager::renderBootScreen() {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    
    // Calculate centered position for title
    const char* title = "LoRaTNCX";
    int titleWidth = u8g2.getStrWidth(title);
    u8g2.drawStr((128 - titleWidth) / 2, 20, title);
    
    // Board name
    u8g2.setFont(u8g2_font_ncenB08_tr);
    const char* board = BOARD_NAME;
    int boardWidth = u8g2.getStrWidth(board);
    u8g2.drawStr((128 - boardWidth) / 2, 38, board);
    
    // Status
    u8g2.setFont(u8g2_font_6x10_tr);
    const char* status = "Initializing...";
    int statusWidth = u8g2.getStrWidth(status);
    u8g2.drawStr((128 - statusWidth) / 2, 55, status);
}

void DisplayManager::renderWiFiStartupScreen() {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    
    // Title
    const char* title = "WiFi Setup";
    int titleWidth = u8g2.getStrWidth(title);
    u8g2.drawStr((128 - titleWidth) / 2, 15, title);
    
    // Status message
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Word wrap the status message
    String msg = wifiStartupMessage;
    if (msg.length() > 0) {
        // Line 1
        if (msg.length() > 21) {
            String line1 = msg.substring(0, 21);
            u8g2.drawStr(0, 35, line1.c_str());
            
            // Line 2
            if (msg.length() > 42) {
                String line2 = msg.substring(21, 42);
                u8g2.drawStr(0, 47, line2.c_str());
                
                // Line 3
                String line3 = msg.substring(42);
                if (line3.length() > 21) line3 = line3.substring(0, 21);
                u8g2.drawStr(0, 59, line3.c_str());
            } else {
                String line2 = msg.substring(21);
                u8g2.drawStr(0, 47, line2.c_str());
            }
        } else {
            // Center single line
            int msgWidth = u8g2.getStrWidth(msg.c_str());
            u8g2.drawStr((128 - msgWidth) / 2, 40, msg.c_str());
        }
    }
}

void DisplayManager::renderStatusScreen() {
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Title
    u8g2.drawStr(0, 10, "Radio Config");
    
    // Frequency
    String freqStr = "Freq: " + formatFrequency(radioFreq);
    u8g2.drawStr(0, 22, freqStr.c_str());
    
    // Bandwidth and Spreading Factor
    String bwStr = "BW: " + formatBandwidth(radioBW);
    String sfStr = " SF: " + String(radioSF);
    String line2 = bwStr + sfStr;
    u8g2.drawStr(0, 34, line2.c_str());
    
    // Coding Rate and Power
    String crStr = "CR: 4/" + String(radioCR);
    String pwrStr = " Pwr: " + String(radioPower) + "dBm";
    String line3 = crStr + pwrStr;
    u8g2.drawStr(0, 46, line3.c_str());
    
    // Sync Word
    String swStr = "Sync: 0x" + String(radioSyncWord, HEX);
    u8g2.drawStr(0, 58, swStr.c_str());
}

void DisplayManager::renderBatteryScreen() {
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Title
    u8g2.drawStr(0, 10, "Battery Status");
    
    // Voltage
    String voltStr = "Voltage: " + String(batteryVoltage, 2) + "V";
    u8g2.drawStr(0, 24, voltStr.c_str());
    
    // Battery percentage estimate
    uint8_t percentage = getBatteryPercentage(batteryVoltage);
    String pctStr = "Level: " + String(percentage) + "%";
    u8g2.drawStr(0, 36, pctStr.c_str());
    
    // Battery bar graph
    int barWidth = 100;
    int barHeight = 15;
    int barX = 14;
    int barY = 45;
    
    // Draw battery outline
    u8g2.drawFrame(barX, barY, barWidth, barHeight);
    u8g2.drawFrame(barX + barWidth, barY + 3, 3, barHeight - 6);  // Battery terminal
    
    // Fill battery level
    int fillWidth = (barWidth - 4) * percentage / 100;
    if (fillWidth > 0) {
        u8g2.drawBox(barX + 2, barY + 2, fillWidth, barHeight - 4);
    }
}

void DisplayManager::renderWiFiScreen() {
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Title
    u8g2.drawStr(0, 10, "WiFi Status");
    
    // Determine WiFi mode
    String mode = "Off";
    if (wifiAPActive && wifiSTAConnected) {
        mode = "AP + Station";
    } else if (wifiAPActive) {
        mode = "AP Mode";
    } else if (wifiSTAConnected) {
        mode = "Station";
    }
    
    String modeStr = "Mode: " + mode;
    u8g2.drawStr(0, 24, modeStr.c_str());
    
    // AP IP if active
    if (wifiAPActive) {
        String apStr = "AP: " + wifiAPIP;
        u8g2.drawStr(0, 36, apStr.c_str());
    }
    
    // Station IP if connected
    if (wifiSTAConnected) {
        String staStr = "STA: " + wifiSTAIP;
        int y = wifiAPActive ? 48 : 36;  // Adjust Y position if showing both
        u8g2.drawStr(0, y, staStr.c_str());
        
        // RSSI signal strength if connected
        if (wifiRSSI != 0) {
            String rssiStr = "Signal: " + String(wifiRSSI) + " dBm";
            int rssiY = wifiAPActive ? 60 : 48;
            u8g2.drawStr(0, rssiY, rssiStr.c_str());
        }
    }
}

String DisplayManager::formatFrequency(float freq) {
    if (freq >= 1000.0) {
        return String(freq / 1000.0, 3) + " GHz";
    } else {
        return String(freq, 1) + " MHz";
    }
}

String DisplayManager::formatBandwidth(float bw) {
    return String((int)bw) + "kHz";
}

uint8_t DisplayManager::getBatteryPercentage(float voltage) {
    // LiPo battery voltage to percentage estimation
    // 4.2V = 100%, 3.7V = 50%, 3.3V = 0% (approximate)
    if (voltage >= 4.2) return 100;
    if (voltage <= 3.3) return 0;
    
    // Linear approximation between 3.3V and 4.2V
    return (uint8_t)((voltage - 3.3) / (4.2 - 3.3) * 100);
}

void DisplayManager::renderGNSSScreen() {
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Title
    u8g2.drawStr(0, 10, "GNSS Status");
    
    if (!gnssEnabled) {
        // GNSS disabled
        u8g2.setFont(u8g2_font_ncenB08_tr);
        const char* msg = "GNSS Disabled";
        int msgWidth = u8g2.getStrWidth(msg);
        u8g2.drawStr((128 - msgWidth) / 2, 35, msg);
        return;
    }
    
    u8g2.setFont(u8g2_font_6x10_tr);
    
    // Fix status
    String fixStr = "Fix: ";
    if (gnssHasFix) {
        fixStr += "YES";
    } else {
        fixStr += "NO";
    }
    fixStr += "  Sats: " + String(gnssSatellites);
    u8g2.drawStr(0, 24, fixStr.c_str());
    
    if (gnssHasFix) {
        // Latitude
        String latStr = "Lat: " + String(gnssLatitude, 6);
        u8g2.drawStr(0, 36, latStr.c_str());
        
        // Longitude
        String lonStr = "Lon: " + String(gnssLongitude, 6);
        u8g2.drawStr(0, 48, lonStr.c_str());
    } else {
        // Waiting for fix
        u8g2.setFont(u8g2_font_6x10_tr);
        const char* msg = "Waiting for GPS fix...";
        int msgWidth = u8g2.getStrWidth(msg);
        u8g2.drawStr((128 - msgWidth) / 2, 40, msg);
    }
    
    // NMEA TCP clients
    String clientStr = "NMEA Clients: " + String(gnssClients);
    u8g2.drawStr(0, 60, clientStr.c_str());
}

void DisplayManager::renderOffScreen() {
    // Clear the display - this will be called once when switching to OFF mode
    u8g2.clearDisplay();
}
