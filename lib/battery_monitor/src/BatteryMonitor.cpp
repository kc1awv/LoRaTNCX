#include "BatteryMonitor.h"

#include <algorithm>
#include <cmath>
#include <driver/adc.h>
#include <esp32-hal-adc.h>

BatteryMonitor::BatteryMonitor(uint8_t adcPin, uint8_t controlPin)
    : adcPin(adcPin), controlPin(controlPin), initialized(false)
{
}

bool BatteryMonitor::begin()
{
    pinMode(controlPin, OUTPUT);
    digitalWrite(controlPin, HIGH); // Enable battery voltage divider per Heltec docs

    analogReadResolution(12);
    analogSetPinAttenuation(adcPin, ADC_11db);

    initialized = true;
    return true;
}

float BatteryMonitor::readVoltage()
{
    if (!initialized)
    {
        return 0.0f;
    }

    float vAdc = readAdcVoltage();
    if (vAdc <= 0.0f)
    {
        return 0.0f;
    }

    return vAdc * DIVIDER_RATIO;
}

uint8_t BatteryMonitor::readPercentage(float minVoltage, float maxVoltage)
{
    float voltage = readVoltage();
    return computePercentage(voltage, minVoltage, maxVoltage);
}

uint8_t BatteryMonitor::computePercentage(float voltage, float minVoltage, float maxVoltage) const
{
    if (maxVoltage <= minVoltage)
    {
        return 0;
    }

    float percent = (voltage - minVoltage) / (maxVoltage - minVoltage);
    if (percent < 0.0f)
    {
        percent = 0.0f;
    }
    else if (percent > 1.0f)
    {
        percent = 1.0f;
    }
    return static_cast<uint8_t>(std::round(percent * 100.0f));
}

float BatteryMonitor::readAdcVoltage()
{
    digitalWrite(controlPin, HIGH);
    delay(2);

    uint32_t accumulator = 0;
    for (uint8_t i = 0; i < SAMPLE_COUNT; ++i)
    {
        accumulator += analogRead(adcPin);
        delayMicroseconds(250);
    }

    float average = static_cast<float>(accumulator) / static_cast<float>(SAMPLE_COUNT);
    return (average / static_cast<float>(ADC_MAX)) * BATTERY_VREF;
}
