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
    
    // Battery voltage measurement (V3.2 uses same logic as V4)
    // Note: V3 original uses inverted logic, but V3.2 is more common
    // V3.2 added transistor to control circuit, inverting the logic
    // Use -DV3_ORIGINAL_BATTERY_LOGIC build flag for original V3 boards
    #define PIN_ADC_BATTERY     1   // GPIO1 - ADC input
    #define PIN_ADC_CTRL        37  // GPIO37 - MOSFET/transistor control
    #ifdef V3_ORIGINAL_BATTERY_LOGIC
        #define ADC_CTRL_ACTIVE_HIGH 0  // Original V3: active LOW
    #else
        #define ADC_CTRL_ACTIVE_HIGH 1  // V3.2: active HIGH (same as V4)
    #endif
    
    // V3 does not have built-in GNSS port, but users can attach external module
    // Default pins are suggestions - users should configure via web interface
    #define HAS_GNSS_PORT       0
    #define PIN_GNSS_RX         -1  // User-configurable
    #define PIN_GNSS_TX         -1  // User-configurable
    #define PIN_GNSS_VEXT       -1  // Not available on V3
    #define PIN_GNSS_CTRL       -1  // Optional power control
    #define PIN_GNSS_WAKE       -1  // Optional wake pin
    #define PIN_GNSS_PPS        -1  // Optional PPS pin
    #define PIN_GNSS_RST        -1  // Optional reset pin
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
    
    // V4 PA gain control for non-linear GC1109 power amplifier
    #define PA_MAX_OUTPUT       28  // Maximum output power (dBm)
    #define PA_GAIN_POINTS      22  // Number of gain points (0-21 corresponding to 7-28 dBm)
    #define PA_GAIN_VALUES      {11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 16, 10, 10, 9, 9, 8, 7}
    
    // Battery voltage measurement (V4 uses NPN transistor, active HIGH)
    #define PIN_ADC_BATTERY     1   // GPIO1 - ADC input
    #define PIN_ADC_CTRL        37  // GPIO37 - Transistor control (active HIGH for V4)
    #define ADC_CTRL_ACTIVE_HIGH 1  // V4 uses active HIGH control (transistor inverts)
    
    // V4 has built-in GNSS port with dedicated pins
    #define HAS_GNSS_PORT       1
    #define PIN_GNSS_RX         39  // GPIO39 - GNSS TX -> MCU RX
    #define PIN_GNSS_TX         38  // GPIO38 - MCU TX -> GNSS RX
    #define PIN_GNSS_VEXT       36  // GPIO36 - GNSS Vext control (active LOW)
    #define PIN_GNSS_CTRL       34  // GPIO34 - VGNSS_CTRL (power control - LOW to enable)
    #define PIN_GNSS_WAKE       40  // GPIO40 - GNSS Wake
    #define PIN_GNSS_PPS        41  // GPIO41 - GNSS PPS (pulse per second)
    #define PIN_GNSS_RST        42  // GPIO42 - GNSS Reset
#endif

// Battery voltage reading
float readBatteryVoltage();

void initializeBoardPins();

#endif // BOARD_CONFIG_H
