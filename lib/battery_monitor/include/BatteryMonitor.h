#pragma once

#include <Arduino.h>
#include "HardwareConfig.h"

/**
 * @brief Helper class for monitoring the Heltec V4 LiPo battery using ADC1_CH0.
 */
class BatteryMonitor
{
public:
    BatteryMonitor(uint8_t adcPin = BATTERY_ADC_PIN, uint8_t controlPin = BATTERY_CTRL_PIN);

    /**
     * @brief Initialize the battery monitoring circuitry.
     *
     * This configures the ADC control pin and prepares the ADC peripheral.
     * @return true if initialization completed.
     */
    bool begin();

    /**
     * @brief Measure the battery voltage in volts.
     *
     * @return Battery voltage in volts. Returns 0.0f if the monitor is disabled.
     */
    float readVoltage();

    /**
     * @brief Estimate the state of charge as a percentage.
     *
     * @param minVoltage Voltage considered 0% (default BATTERY_MIN_VOLTAGE).
     * @param maxVoltage Voltage considered 100% (default BATTERY_MAX_VOLTAGE).
     * @return Percentage between 0 and 100.
     */
    uint8_t readPercentage(float minVoltage = BATTERY_MIN_VOLTAGE, float maxVoltage = BATTERY_MAX_VOLTAGE);

    /**
     * @brief Compute percentage from a provided voltage sample.
     */
    uint8_t computePercentage(float voltage, float minVoltage = BATTERY_MIN_VOLTAGE, float maxVoltage = BATTERY_MAX_VOLTAGE) const;

private:
    static constexpr uint8_t SAMPLE_COUNT = 8;
    static constexpr float DIVIDER_RATIO = (BATTERY_R1_OHMS + BATTERY_R2_OHMS) / BATTERY_R2_OHMS;
    static constexpr uint16_t ADC_MAX = 4095; // 12-bit ADC on ESP32-S3

    uint8_t adcPin;
    uint8_t controlPin;
    bool initialized;

    float readAdcVoltage();
};
