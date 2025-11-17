#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// Battery charge states are defined in config.h

class BatteryMonitor {
private:
    // Battery monitoring state
    bool ready;
    BatteryChargeState chargeState;
    float voltage;
    float percent;
    uint8_t sampleIndex;
    float voltageReadings[BATTERY_SAMPLE_COUNT];
    float percentReadings[BATTERY_SAMPLE_COUNT];
    bool voltageDecreasing;
    float previousVoltage;
    float stateChangeVoltage;
    uint8_t chargeConfirmations;

    // Mutex for thread-safe access
    SemaphoreHandle_t mutex;

    // Private methods
    void updateChargeState();
    float calculateAverage(float* readings, uint8_t count);

public:
    BatteryMonitor();
    ~BatteryMonitor();

    // Initialize the battery monitor
    bool begin();

    // Update battery readings
    void update();

    // Get battery status (thread-safe)
    bool isReady() const { return ready; }
    BatteryChargeState getChargeState();
    float getVoltage();
    float getPercent();

    // Force a new reading
    void forceUpdate();
    
    // Get raw battery voltage reading (for compatibility)
    float readBatteryVoltageRaw();
};

#endif // BATTERY_MONITOR_H