#include "board_config.h"
#include <Arduino.h>

// Global pin variables
int8_t RADIO_SCLK_PIN;
int8_t RADIO_MISO_PIN;
int8_t RADIO_MOSI_PIN;
int8_t RADIO_CS_PIN;
int8_t RADIO_DIO0_PIN;
int8_t RADIO_RST_PIN;
int8_t RADIO_DIO1_PIN;
int8_t RADIO_BUSY_PIN;

BoardType BOARD_TYPE = BOARD_UNKNOWN;

void setupPAControl() {
#ifdef HAS_PA_CONTROL
    #if HAS_PA_CONTROL == 1
        // V4 board: Configure PA control pins
        pinMode(PIN_LORA_PA_POWER, ANALOG);  // PA power as analog input
        pinMode(PIN_LORA_PA_EN, OUTPUT);
        pinMode(PIN_LORA_PA_TX_EN, OUTPUT);
        
        // Enable PA
        digitalWrite(PIN_LORA_PA_EN, HIGH);
        digitalWrite(PIN_LORA_PA_TX_EN, HIGH);
    #endif
#endif
}

void initializeBoardPins() {
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V3
    RADIO_SCLK_PIN = PIN_RADIO_SCLK;
    RADIO_MISO_PIN = PIN_RADIO_MISO;
    RADIO_MOSI_PIN = PIN_RADIO_MOSI;
    RADIO_CS_PIN = PIN_RADIO_CS;
    RADIO_DIO0_PIN = PIN_RADIO_DIO0;
    RADIO_RST_PIN = PIN_RADIO_RST;
    RADIO_DIO1_PIN = PIN_RADIO_DIO1;
    RADIO_BUSY_PIN = PIN_RADIO_BUSY;
    BOARD_TYPE = BOARD_VARIANT;
    
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V4)
    RADIO_SCLK_PIN = PIN_RADIO_SCLK;
    RADIO_MISO_PIN = PIN_RADIO_MISO;
    RADIO_MOSI_PIN = PIN_RADIO_MOSI;
    RADIO_CS_PIN = PIN_RADIO_CS;
    RADIO_DIO0_PIN = PIN_RADIO_DIO0;
    RADIO_RST_PIN = PIN_RADIO_RST;
    RADIO_DIO1_PIN = PIN_RADIO_DIO1;
    RADIO_BUSY_PIN = PIN_RADIO_BUSY;
    BOARD_TYPE = BOARD_VARIANT;
    
#else
    BOARD_TYPE = BOARD_UNKNOWN;
#endif
}

float readBatteryVoltage() {
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
    pinMode(PIN_ADC_CTRL, OUTPUT);
    pinMode(PIN_ADC_BATTERY, INPUT);
    analogReadResolution(resolution);
    analogSetAttenuation(ADC_2_5db);  // 2.5dB attenuation for 0-1.5V range
    
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
    
    // Disable battery voltage measurement circuit to save power
#if ADC_CTRL_ACTIVE_HIGH
    digitalWrite(PIN_ADC_CTRL, LOW);   // V3.2/V4: LOW disables
#else
    digitalWrite(PIN_ADC_CTRL, HIGH);  // V3 original: HIGH disables
#endif
    
    // Calculate battery voltage
    float batteryVoltage = factor * analogValue;
    
    return batteryVoltage;
#else
    return 0.0;  // Battery reading not supported on this board
#endif
}
