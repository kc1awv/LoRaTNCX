#include "board_config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
