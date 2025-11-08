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
    // Configure the ADC control pin
    pinMode(PIN_ADC_CTRL, OUTPUT);
    
    // Enable the battery voltage measurement circuit
    // V3 uses active LOW (P-channel MOSFET directly controlled)
    // V4/V3.2 uses active HIGH (NPN transistor inverts signal to P-channel MOSFET)
#if ADC_CTRL_ACTIVE_HIGH
    digitalWrite(PIN_ADC_CTRL, HIGH);  // V4: HIGH enables transistor -> MOSFET on
#else
    digitalWrite(PIN_ADC_CTRL, LOW);   // V3: LOW enables MOSFET directly
#endif
    
    // Small delay to allow the circuit to stabilize
    delay(10);
    
    // Read ADC value (12-bit, 0-4095)
    // ESP32-S3 uses default 11dB attenuation (0-3.3V range)
    uint16_t adcValue = analogRead(PIN_ADC_BATTERY);
    
    // Disable the battery voltage measurement circuit to save power
#if ADC_CTRL_ACTIVE_HIGH
    digitalWrite(PIN_ADC_CTRL, LOW);   // V4: disable
#else
    digitalWrite(PIN_ADC_CTRL, HIGH);  // V3: disable
#endif
    
    // Convert to voltage using default 11dB attenuation (0-3.3V range)
    float adcVoltage = (adcValue / 4095.0) * 3.3;
    
    // Account for voltage divider to get actual battery voltage
    // 390k/100k divider means battery voltage = ADC voltage / 0.2041
    float batteryVoltage = adcVoltage / BATTERY_DIVIDER;
    
    return batteryVoltage;
#else
    return 0.0;  // Battery reading not supported on this board
#endif
}
