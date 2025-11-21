#include "battery_monitor.h"
#include "board_config.h"
#include "debug.h"
#include <Arduino.h>

BatteryMonitor::BatteryMonitor()
    : ready(false), chargeState(BATTERY_CHARGE_UNKNOWN), voltage(0.0f), percent(0.0f),
      sampleIndex(0), voltageDecreasing(false), previousVoltage(0.0f),
      stateChangeVoltage(0.0f), chargeConfirmations(0), mutex(nullptr) {
    // Initialize readings arrays
    for (uint8_t i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
        voltageReadings[i] = 0.0f;
        percentReadings[i] = 0.0f;
    }
}

BatteryMonitor::~BatteryMonitor() {
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

bool BatteryMonitor::begin() {
    // Create mutex for thread-safe access
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        return false;
    }

    // Initialize battery monitoring pins
    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, ADC_CTRL_ACTIVE_HIGH ? HIGH : LOW);

    // Configure ADC pin
    pinMode(PIN_ADC_BATTERY, INPUT);
    adcAttachPin(PIN_ADC_BATTERY);
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_2_5db);

    ready = true;
    return true;
}

float BatteryMonitor::calculateAverage(float* readings, uint8_t count) {
    float sum = 0.0f;
    uint8_t validCount = 0;

    for (uint8_t i = 0; i < count; i++) {
        if (readings[i] > 0.0f) {  // Only count valid readings
            sum += readings[i];
            validCount++;
        }
    }

    return validCount > 0 ? sum / validCount : 0.0f;
}

void BatteryMonitor::updateChargeState() {
    // Voltage change detection with hysteresis
    float voltage_change = previousVoltage - voltage;

    if (voltageDecreasing) {
        if (voltage_change < 0.01f) {  // Voltage rising
            voltageDecreasing = false;
            stateChangeVoltage = voltage;
        }
    } else {
        if (voltage_change > 0.01f) {  // Voltage falling
            voltageDecreasing = true;
            stateChangeVoltage = voltage;
        }
    }

    // Determine battery charge state
    if (voltageDecreasing && voltage < BATTERY_FLOAT_VOLTAGE) {
        // Voltage is decreasing and below float voltage - discharging
        if (chargeState != BATTERY_CHARGE_DISCHARGING) {
            chargeState = BATTERY_CHARGE_DISCHARGING;
            chargeConfirmations = 0;
        }
    } else {
        // Voltage not decreasing or above float voltage
        if (chargeConfirmations < 8) {
            chargeConfirmations++;
        } else {
            if (percent < 100.0f) {
                // Confirmed charged but percentage not 100% - charging
                if (chargeState != BATTERY_CHARGE_CHARGING) {
                    chargeState = BATTERY_CHARGE_CHARGING;
                }
            } else {
                // Fully charged
                if (chargeState != BATTERY_CHARGE_CHARGED) {
                    chargeState = BATTERY_CHARGE_CHARGED;
                }
            }
        }
    }
}

void BatteryMonitor::update() {
    if (!ready || !mutex) {
        return;
    }

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Read battery voltage using the same logic as the original function
        float newVoltage = readBatteryVoltageRaw();

        // Store reading in circular buffer
        voltageReadings[sampleIndex] = newVoltage;
        sampleIndex = (sampleIndex + 1) % BATTERY_SAMPLE_COUNT;

        // Calculate averages when we have enough samples
        if (sampleIndex == 0) {
            voltage = calculateAverage(voltageReadings, BATTERY_SAMPLE_COUNT);

            // Convert to percentage
            if (voltage >= BATTERY_VOLTAGE_MAX) {
                percent = 100.0f;
            } else if (voltage <= BATTERY_VOLTAGE_MIN) {
                percent = 0.0f;
            } else {
                percent = ((voltage - BATTERY_VOLTAGE_MIN) /
                          (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0f;
            }

            // Clamp percentage to valid range
            if (percent > 100.0f) percent = 100.0f;
            if (percent < 0.0f) percent = 0.0f;

            // Initialize previous voltage on first run
            if (previousVoltage == 0.0f) previousVoltage = voltage;
            if (stateChangeVoltage == 0.0f) stateChangeVoltage = voltage;

            // Update charge state detection
            updateChargeState();

            // Update previous voltage
            previousVoltage = voltage;
        }

        xSemaphoreGive(mutex);
    }
}

float BatteryMonitor::readBatteryVoltageRaw() {
#ifdef PIN_ADC_BATTERY
    // ADC resolution
    const int resolution = ADC_RESOLUTION;
    const int adcMax = pow(2, resolution) - 1;  // 4095
    // Use 2.5dB attenuation for better accuracy in the 0.8-1.0V range
    // ADC_2_5db: 0-1.5V range (better resolution for our battery divider voltage)
    const float adcMaxVoltage = ADC_MAX_VOLTAGE;

    // On-board voltage divider
    const int R1 = BATTERY_R1;  // 390k ohm
    const int R2 = BATTERY_R2;  // 100k ohm

    // Calibration measurements (measure your actual battery voltage with multimeter)
    const float measuredVoltage = BATTERY_CAL_VOLTAGE;    // Actual battery voltage
    const float reportedVoltage = BATTERY_CAL_REPORTED;  // What the ADC reports with 2.5dB attenuation

    // Calculate calibration factor
    const float factor = (adcMaxVoltage / adcMax) * ((R1 + R2) / (float)R2) * (measuredVoltage / reportedVoltage);

    // Configure pins
    // ADC is already configured in begin(), just set pin mode
    pinMode(PIN_ADC_BATTERY, INPUT);

    // Enable battery voltage measurement circuit
    // IMPORTANT: Logic depends on board version:
    // - V3 (original): ADC_Ctrl LOW enables, HIGH disables
    // - V3.2/V4: ADC_Ctrl HIGH enables, LOW disables (inverted due to added transistor)
#if ADC_CTRL_ACTIVE_HIGH
    digitalWrite(PIN_ADC_CTRL, HIGH);  // V3.2/V4: HIGH enables
#else
    digitalWrite(PIN_ADC_CTRL, LOW);   // V3 original: LOW enables
#endif

    // Wait for circuit to stabilize
    delay(ADC_STABILIZE_DELAY);

    // Read ADC value
    int analogValue = analogRead(PIN_ADC_BATTERY);
    DEBUG_PRINT("Battery ADC raw value: ");
    DEBUG_PRINTLN(analogValue);

    // For more accurate voltage reading, we could use analogReadMilliVolts()
    // but we'll stick with the calibrated analogRead() for now

    // Disable battery voltage measurement circuit to save power
#if ADC_CTRL_ACTIVE_HIGH
    digitalWrite(PIN_ADC_CTRL, LOW);   // V3.2/V4: LOW disables
#else
    digitalWrite(PIN_ADC_CTRL, HIGH);  // V3 original: HIGH disables
#endif

    // Calculate battery voltage
    return factor * analogValue;
#else
    return 0.0f;  // Battery reading not supported on this board
#endif
}

BatteryChargeState BatteryMonitor::getChargeState() {
    if (!mutex) return BATTERY_CHARGE_UNKNOWN;

    BatteryChargeState state;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = chargeState;
        xSemaphoreGive(mutex);
    } else {
        state = BATTERY_CHARGE_UNKNOWN;
    }
    return state;
}

float BatteryMonitor::getVoltage() {
    if (!mutex) return 0.0f;

    float volt;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        volt = voltage;
        xSemaphoreGive(mutex);
    } else {
        volt = 0.0f;
    }
    return volt;
}

float BatteryMonitor::getPercent() {
    if (!mutex) return 0.0f;

    float perc;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        perc = percent;
        xSemaphoreGive(mutex);
    } else {
        perc = 0.0f;
    }
    return perc;
}

void BatteryMonitor::forceUpdate() {
    update();
}