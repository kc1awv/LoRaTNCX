#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "config.h"

// Common pins for both V3 and V4
#define PIN_USER_BUTTON  0   // GPIO0 - User button (boot button)

// PA control function (V4 only)
void setupPAControl();

// Heltec WiFi LoRa 32 V3 Pin Definitions
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V3
    #define PIN_RADIO_SCLK  9
    #define PIN_RADIO_MISO  11
    #define PIN_RADIO_MOSI  10
    #define PIN_RADIO_CS    8
    #define PIN_RADIO_DIO0  14
    #define PIN_RADIO_RST   12
    #define PIN_RADIO_DIO1  14
    #define PIN_RADIO_BUSY  13
    #define BOARD_VARIANT   BOARD_V3
    #define BOARD_NAME      "Heltec WiFi LoRa 32 V3"
    
    // V3 has no external PA - direct SX1262 control
    #define HAS_PA_CONTROL      0
    
    // Battery voltage measurement (V3/V3.2 - try active HIGH as many V3 boards are actually V3.2)
    #define PIN_ADC_BATTERY     1   // GPIO1 - ADC input
    #define PIN_ADC_CTRL        37  // GPIO37 - MOSFET/transistor control
    #define BATTERY_DIVIDER     (100.0 / (100.0 + 390.0))  // 390k/100k divider (~0.2041)
    #define ADC_CTRL_ACTIVE_HIGH 1  // V3.2 uses active HIGH (many V3 boards are actually V3.2)
#endif

// Heltec WiFi LoRa 32 V4 Pin Definitions  
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
    #define PIN_RADIO_SCLK  9
    #define PIN_RADIO_MISO  11
    #define PIN_RADIO_MOSI  10
    #define PIN_RADIO_CS    8
    #define PIN_RADIO_DIO0  14
    #define PIN_RADIO_RST   12
    #define PIN_RADIO_DIO1  14
    #define PIN_RADIO_BUSY  13
    #define BOARD_VARIANT   BOARD_V4
    #define BOARD_NAME      "Heltec WiFi LoRa 32 V4"
    
    // V4 has an external PA that requires control pins
    #define PIN_LORA_PA_EN      2   // PA enable
    #define PIN_LORA_PA_TX_EN   46  // PA TX enable
    #define PIN_LORA_PA_POWER   7   // PA power control
    #define HAS_PA_CONTROL      1   // Flag indicating PA control is needed
    
    // Battery voltage measurement (V4 uses NPN transistor, active HIGH)
    #define PIN_ADC_BATTERY     1   // GPIO1 - ADC input
    #define PIN_ADC_CTRL        37  // GPIO37 - Transistor control (active HIGH for V4)
    #define BATTERY_DIVIDER     (100.0 / (100.0 + 390.0))  // 390k/100k divider (~0.2041)
    #define ADC_CTRL_ACTIVE_HIGH 1  // V4 uses active HIGH control (transistor inverts)
#endif

// Battery voltage reading
float readBatteryVoltage();

void initializeBoardPins();

#endif // BOARD_CONFIG_H
