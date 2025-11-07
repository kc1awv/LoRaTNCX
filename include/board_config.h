#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "config.h"

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
#endif

void initializeBoardPins();

#endif // BOARD_CONFIG_H
