#pragma once

#include <Arduino.h>

// Forward declaration to avoid circular dependency
class ConfigManager;

class BatteryMonitor
{
public:
    enum BatteryState
    {
        CHARGING,    // ADC increasing
        FLOATING,    // ADC stable (±3 units)
        DISCHARGING  // ADC decreasing
    };

    struct BatteryStatus
    {
        float voltage;         // Battery voltage in volts
        uint8_t stateOfCharge; // State of charge percentage (0-100)
        BatteryState state;    // Current battery state (charging/floating/discharging)
        bool isConnected;      // True if battery is detected
        bool criticalLevel;    // True if battery needs deep sleep
        uint16_t rawADC;       // Raw ADC reading for debugging
    };

private:
    static const int VBAT_PIN = 1;     // GPIO 1 (ADC1_CH0) for battery voltage reading
    static const int ADC_CTRL_PIN = 37; // GPIO 37 (ADC_CTRL) must be HIGH to enable battery reading

    // V4 board voltage divider: 100Ω / (100Ω + 390Ω) = 0.204
    static constexpr float VOLTAGE_DIVIDER = (100.0 + 390.0) / 100.0; // Inverse of divider ratio
    static constexpr float ADC_REFERENCE = 3.3;       // ESP32 ADC reference voltage
    static constexpr int ADC_RESOLUTION = 4095;       // 12-bit ADC resolution

    // Battery thresholds based on observed behavior (ADC 970 = 3.82V full charge)
    static constexpr float BATTERY_MIN_VOLTAGE = 3.30;    // Empty (0%) - safe minimum
    static constexpr float BATTERY_MAX_VOLTAGE = 3.82;    // Full (100%) - observed maximum
    static constexpr float BATTERY_CRITICAL_VOLTAGE = 3.40; // Critical level for deep sleep (10%)
    static constexpr float DISCONNECTED_THRESHOLD = 2.5;  // If voltage < this, no battery
    
    // ADC thresholds based on calculated values
    static constexpr uint16_t ADC_MIN = 837;    // 3.30V (0%)
    static constexpr uint16_t ADC_MAX = 970;    // 3.82V (100%)  
    static constexpr uint16_t ADC_CRITICAL = 863; // 3.40V (deep sleep threshold)
    static constexpr uint16_t ADC_FLOAT_THRESHOLD = 3; // ±3 ADC units for floating detection

    unsigned long lastReadTime;
    static constexpr unsigned long READ_INTERVAL = 1000; // Read every 1 second
    BatteryStatus currentStatus;

    bool initialized;
    uint16_t readRawADC();
    float calculateVoltage(uint16_t rawADC);
    uint8_t calculateStateOfCharge(uint16_t rawADC);
    BatteryState analyzeBatteryState(uint16_t currentADC);
    bool detectConnection(float voltage);
    bool isCriticalLevel(uint16_t rawADC);
    void enterDeepSleep();
    
    // ADC trend analysis for state detection
    uint16_t previousADC;
    uint16_t adcHistory[5];  // Store last 5 readings for trend analysis
    uint8_t historyIndex;
    unsigned long lastStateChange;
    static constexpr unsigned long STATE_CHANGE_DEBOUNCE = 3000; // 3 seconds

    // Configuration reference
    ConfigManager* configManager;

public:
    BatteryMonitor();

    bool begin(ConfigManager* config = nullptr);
    void poll();
    const BatteryStatus &getStatus() const;

    // Utility functions
    String getVoltageString(int decimals = 2) const;
    String getStateOfChargeString() const;
    String getStatusString() const;
    String getBatteryStateString() const;
    
    // Static helper for web interface
    static String batteryStateToString(BatteryState state);

    // For web interface
    String toJSON() const;
};

extern BatteryMonitor battery;