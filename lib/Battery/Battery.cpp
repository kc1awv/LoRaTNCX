#include "Battery.h"
#include "Config.h"

BatteryMonitor battery;

BatteryMonitor::BatteryMonitor()
    : lastReadTime(0), initialized(false), previousADC(0), historyIndex(0), lastStateChange(0), configManager(nullptr)
{
    currentStatus.voltage = 0.0;
    currentStatus.stateOfCharge = 0;
    currentStatus.state = FLOATING;
    currentStatus.isConnected = false;
    currentStatus.criticalLevel = false;
    currentStatus.rawADC = 0;
    
    // Initialize ADC history
    for (int i = 0; i < 5; i++) {
        adcHistory[i] = 0;
    }
}

bool BatteryMonitor::begin(ConfigManager* config)
{
    configManager = config;
    
    bool debugEnabled = (configManager && configManager->getBatteryConfig().debugMessages);
    
    if (debugEnabled) {
#ifndef KISS_SERIAL_MODE
        Serial.println("[BATTERY] Initializing battery monitor...");
#endif
    }

    // Configure ADC_CTRL pin - must be HIGH to enable battery reading
    pinMode(ADC_CTRL_PIN, OUTPUT);
    digitalWrite(ADC_CTRL_PIN, HIGH);
    if (debugEnabled) {
#ifndef KISS_SERIAL_MODE
        Serial.printf("[BATTERY] ADC_CTRL pin %d set HIGH\n", ADC_CTRL_PIN);
#endif
    }
    delay(10);

    // Configure battery voltage pin (GPIO 1)
    pinMode(VBAT_PIN, INPUT);

    // Set ADC configuration
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);
    
    // Set pin-specific attenuation for battery pin
    analogSetPinAttenuation(VBAT_PIN, ADC_11db);

    if (debugEnabled) {
#ifndef KISS_SERIAL_MODE
        Serial.printf("[BATTERY] Using GPIO %d for battery voltage reading\n", VBAT_PIN);
        Serial.printf("[BATTERY] Voltage divider ratio: %.2f\n", VOLTAGE_DIVIDER);
#endif
    }

    poll();

    initialized = true;
    if (debugEnabled) {
#ifndef KISS_SERIAL_MODE
        Serial.printf("[BATTERY] Battery monitor initialized - Status: %s\n", getStatusString().c_str());
#endif
    }

    return true;
}

void BatteryMonitor::poll()
{
    unsigned long now = millis();

    if (now - lastReadTime < READ_INTERVAL)
    {
        return;
    }

    lastReadTime = now;

    // Read battery ADC from GPIO 1
    currentStatus.rawADC = readRawADC();
    currentStatus.voltage = calculateVoltage(currentStatus.rawADC);

    currentStatus.isConnected = detectConnection(currentStatus.voltage);

    if (currentStatus.isConnected)
    {
        // Analyze battery state based on ADC trend
        currentStatus.state = analyzeBatteryState(currentStatus.rawADC);
        
        // Calculate state of charge based on ADC value
        currentStatus.stateOfCharge = calculateStateOfCharge(currentStatus.rawADC);
        
        // Check for critical battery level
        currentStatus.criticalLevel = isCriticalLevel(currentStatus.rawADC);
        
        // Enter deep sleep if battery is critically low
        if (currentStatus.criticalLevel && currentStatus.state == DISCHARGING)
        {
            // Always show critical battery messages regardless of debug setting
#ifndef KISS_SERIAL_MODE
            Serial.println("[BATTERY] CRITICAL LEVEL - Entering deep sleep to protect battery!");
#endif
            enterDeepSleep();
        }
    }
    else
    {
        currentStatus.stateOfCharge = 0;
        currentStatus.state = FLOATING;
        currentStatus.criticalLevel = false;
    }

    // Enhanced debug output (only if debug is enabled)
    bool debugEnabled = (configManager && configManager->getBatteryConfig().debugMessages);
    if (debugEnabled) {
#ifndef KISS_SERIAL_MODE
        Serial.printf("[BATTERY] Raw ADC: %d, Battery: %.2fV, SoC: %d%%, State: %s%s\n", 
                      currentStatus.rawADC, currentStatus.voltage, 
                      currentStatus.stateOfCharge, getBatteryStateString().c_str(),
                      currentStatus.criticalLevel ? " [CRITICAL]" : "");
#endif
    }
}

uint16_t BatteryMonitor::readRawADC()
{
    const int NUM_SAMPLES = 10;
    uint32_t sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        sum += analogRead(VBAT_PIN);
        delayMicroseconds(100);
    }

    return sum / NUM_SAMPLES;
}

float BatteryMonitor::calculateVoltage(uint16_t rawADC)
{
    // V4 board voltage calculation: VBAT = (100 + 390) / 100 × VADC_IN1
    float adcVoltage = (float)rawADC / ADC_RESOLUTION * ADC_REFERENCE;
    float batteryVoltage = adcVoltage * VOLTAGE_DIVIDER;
    
    return batteryVoltage;
}

uint8_t BatteryMonitor::calculateStateOfCharge(uint16_t rawADC)
{
    if (rawADC < ADC_MIN)
    {
        return 0;
    }

    if (rawADC > ADC_MAX)
    {
        return 100;
    }

    // Calculate percentage based on ADC range
    float percentage = ((float)(rawADC - ADC_MIN) / (float)(ADC_MAX - ADC_MIN)) * 100.0;

    return (uint8_t)constrain(percentage, 0, 100);
}

BatteryMonitor::BatteryState BatteryMonitor::analyzeBatteryState(uint16_t currentADC)
{
    unsigned long now = millis();
    
    // Store current ADC in history
    adcHistory[historyIndex] = currentADC;
    historyIndex = (historyIndex + 1) % 5;
    
    // Need at least 3 readings for trend analysis
    if (previousADC == 0) {
        previousADC = currentADC;
        return FLOATING; // Default state until we have enough data
    }
    
    // Calculate trend over last few readings
    int16_t shortTrend = (int16_t)currentADC - (int16_t)previousADC;
    
    // Calculate longer trend if we have enough history
    int16_t longTrend = 0;
    uint8_t validReadings = 0;
    for (int i = 0; i < 5; i++) {
        if (adcHistory[i] > 0) validReadings++;
    }
    
    if (validReadings >= 3) {
        // Calculate trend from oldest to newest reading
        uint16_t oldest = adcHistory[(historyIndex + 5 - validReadings) % 5];
        longTrend = (int16_t)currentADC - (int16_t)oldest;
    }
    
    BatteryState newState = currentStatus.state;
    
    // Determine state based on trends
    if (abs(shortTrend) <= ADC_FLOAT_THRESHOLD && abs(longTrend) <= ADC_FLOAT_THRESHOLD * 2) {
        newState = FLOATING;  // ADC is stable
    } else if (shortTrend > ADC_FLOAT_THRESHOLD || longTrend > ADC_FLOAT_THRESHOLD * 2) {
        newState = CHARGING;  // ADC is increasing
    } else if (shortTrend < -ADC_FLOAT_THRESHOLD || longTrend < -ADC_FLOAT_THRESHOLD * 2) {
        newState = DISCHARGING; // ADC is decreasing
    }
    
    // Apply debouncing - only change state if it's been stable for a while
    if (newState != currentStatus.state) {
        if (now - lastStateChange > STATE_CHANGE_DEBOUNCE) {
            lastStateChange = now;
            previousADC = currentADC;
            return newState;
        }
    } else {
        lastStateChange = now; // Reset timer if state matches
    }
    
    previousADC = currentADC;
    return currentStatus.state; // Keep current state if not enough time has passed
}

bool BatteryMonitor::isCriticalLevel(uint16_t rawADC)
{
    return rawADC <= ADC_CRITICAL;
}

void BatteryMonitor::enterDeepSleep()
{
#ifndef KISS_SERIAL_MODE
    Serial.println("[BATTERY] Preparing for deep sleep...");
    Serial.flush();
#endif
    
    // TODO: Add any necessary cleanup here (save state, turn off peripherals, etc.)
    
    // Configure wake-up source (you may want to adjust this)
    esp_sleep_enable_timer_wakeup(60 * 1000000); // Wake up every 60 seconds to check battery
    
#ifndef KISS_SERIAL_MODE
    Serial.println("[BATTERY] Entering deep sleep mode");
    Serial.flush();
#endif
    
    esp_deep_sleep_start();
}

bool BatteryMonitor::detectConnection(float voltage)
{
    return voltage > DISCONNECTED_THRESHOLD;
}

const BatteryMonitor::BatteryStatus &BatteryMonitor::getStatus() const
{
    return currentStatus;
}

String BatteryMonitor::getVoltageString(int decimals) const
{
    if (!currentStatus.isConnected)
    {
        return "N/A";
    }
    return String(currentStatus.voltage, decimals) + "V";
}

String BatteryMonitor::getStateOfChargeString() const
{
    if (!currentStatus.isConnected)
    {
        return "N/A";
    }
    return String(currentStatus.stateOfCharge) + "%";
}

String BatteryMonitor::getStatusString() const
{
    if (!currentStatus.isConnected)
    {
        return "No Battery";
    }

    String status = getVoltageString(2) + " (" + getStateOfChargeString() + ") " + getBatteryStateString();
    
    if (currentStatus.criticalLevel)
    {
        status += " [CRITICAL]";
    }
    
    return status;
}

String BatteryMonitor::getBatteryStateString() const
{
    if (!currentStatus.isConnected)
    {
        return "N/A";
    }
    
    return batteryStateToString(currentStatus.state);
}

String BatteryMonitor::batteryStateToString(BatteryState state)
{
    switch (state)
    {
        case CHARGING:    return "Charging";
        case FLOATING:    return "Floating";
        case DISCHARGING: return "Discharging";
        default:          return "Unknown";
    }
}

String BatteryMonitor::toJSON() const
{
    String json = "{";
    json += "\"voltage\":" + String(currentStatus.voltage, 3) + ",";
    json += "\"stateOfCharge\":" + String(currentStatus.stateOfCharge) + ",";
    json += "\"batteryState\":\"" + getBatteryStateString() + "\",";
    json += "\"isConnected\":" + String(currentStatus.isConnected ? "true" : "false") + ",";
    json += "\"criticalLevel\":" + String(currentStatus.criticalLevel ? "true" : "false") + ",";
    json += "\"rawADC\":" + String(currentStatus.rawADC) + ",";
    json += "\"status\":\"" + getStatusString() + "\"";
    json += "}";
    return json;
}