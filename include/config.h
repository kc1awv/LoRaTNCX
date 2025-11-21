#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// KISS Protocol Constants
#define FEND  0xC0  // Frame End
#define FESC  0xDB  // Frame Escape
#define TFEND 0xDC  // Transposed Frame End
#define TFESC 0xDD  // Transposed Frame Escape

// KISS Commands
// Note: Traditional KISS parameters (TXDELAY, P, SLOTTIME, TXTAIL, FULLDUPLEX)
// are specific to VHF/UHF FM radio operation and are not applicable to LoRa.
// Commands are accepted for KISS protocol compatibility but have no effect.
#define CMD_DATA        0x00
#define CMD_TXDELAY     0x01  // Not used - LoRa has instant TX
#define CMD_P           0x02  // Not used - LoRa uses CAD, not CSMA
#define CMD_SLOTTIME    0x03  // Not used - LoRa uses CAD, not CSMA  
#define CMD_TXTAIL      0x04  // Not used - No squelch tail in LoRa
#define CMD_FULLDUPLEX  0x05  // Not used - SX1262 is half-duplex only
#define CMD_SETHARDWARE 0x06  // Hardware-specific settings (LoRa parameters)
#define CMD_GETHARDWARE 0x07  // Get hardware status (LoRa config, battery, etc.)
#define CMD_RETURN      0xFF  // Exit KISS mode

// SETHARDWARE Sub-commands for LoRa Configuration
#define HW_SET_FREQUENCY    0x01  // Set frequency (4 bytes, float MHz)
#define HW_SET_BANDWIDTH    0x02  // Set bandwidth (1 byte: 0=125, 1=250, 2=500 kHz)
#define HW_SET_SPREADING    0x03  // Set spreading factor (1 byte: 7-12)
#define HW_SET_CODINGRATE   0x04  // Set coding rate (1 byte: 5-8 for 4/5 to 4/8)
#define HW_SET_POWER        0x05  // Set TX power (1 byte: dBm)
#define HW_GET_CONFIG       0x06  // Get current configuration
#define HW_SAVE_CONFIG      0x07  // Save configuration to flash
#define HW_SET_SYNCWORD     0x08  // Set sync word (2 bytes for SX126x)
#define HW_SET_GNSS_ENABLE  0x09  // Enable/disable GNSS (1 byte: 0=disable, 1=enable)
#define HW_RESET_CONFIG     0xFF  // Reset to defaults

// GETHARDWARE Sub-commands for reading hardware status
#define HW_QUERY_CONFIG     0x01  // Query current radio configuration
#define HW_QUERY_BATTERY    0x02  // Query battery status: voltage(float), avg_voltage(float), percent(float), state(uint8), ready(uint8)
#define HW_QUERY_BOARD      0x03  // Query board information
#define HW_QUERY_GNSS       0x04  // Query GNSS status and configuration
#define HW_QUERY_ALL        0xFF  // Query everything (config + battery + board + gnss)

// LoRa Buffer Sizes
#define LORA_BUFFER_SIZE        256
#define LORA_MAX_FRAME_SIZE     240  // Leave room for headers
#define SERIAL_BUFFER_SIZE      512  // Serial buffer size

// Serial Configuration
#define SERIAL_BAUD_RATE        115200
#define SERIAL_INIT_DELAY       100

// WiFi Configuration
#define WIFI_TIMEOUT_MS         30000
#define WIFI_INIT_DELAY_MS      100
#define WIFI_STATUS_DELAY_MS    2000
#define WIFI_CHANGE_DELAY_MS    1000

// GNSS Configuration
#define GNSS_DEFAULT_BAUD       9600
#define GNSS_POWER_ON_DELAY_MS  100
#define GNSS_STABILIZE_DELAY_MS 1000

// Web Server Configuration
#define WEB_SERVER_PORT         80
#define WEB_CACHE_MAX_AGE       86400  // 24 hours

// Battery Monitoring
#define BATTERY_SAMPLE_INTERVAL 10000  // 10 seconds

// Watchdog Configuration
#define WDT_TIMEOUT_SECONDS     30

// Radio Parameter Validation Ranges
#define RADIO_FREQ_MIN              150.0f
#define RADIO_FREQ_MAX              960.0f
#define RADIO_SF_MIN                7
#define RADIO_SF_MAX                12
#define RADIO_CR_MIN                5
#define RADIO_CR_MAX                8
#define RADIO_POWER_MIN             -9
// Power limits depend on board type (V4 supports higher power)
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
#define RADIO_POWER_MAX             28  // V4 supports up to 28dBm with PA gain control
#else
#define RADIO_POWER_MAX             22  // V3 and other boards limited to 22dBm
#endif
#define RADIO_SYNCWORD_MIN          0x0000
#define RADIO_SYNCWORD_MAX          0xFFFF

// ADC Configuration
#define ADC_RESOLUTION      12
#define ADC_MAX_VOLTAGE     1.5f   // 1.5V with 2.5dB attenuation
#define ADC_STABILIZE_DELAY 100    // ms

// Battery Measurement Circuit
#define BATTERY_R1          390    // 390k ohm
#define BATTERY_R2          100    // 100k ohm
#define BATTERY_CAL_VOLTAGE 4.2f   // Target voltage to display (standard LiPo)
#define BATTERY_CAL_REPORTED 4.095f // What ADC actually reports for target voltage

// Battery Monitoring Constants
#define BATTERY_SAMPLE_COUNT    10     // Number of samples to average
#define BATTERY_VOLTAGE_MIN     3.0f   // Minimum battery voltage (0%)
#define BATTERY_VOLTAGE_MAX     4.2f   // Maximum battery voltage (100%) - standard LiPo max
#define BATTERY_FLOAT_VOLTAGE   4.1f   // Float voltage threshold for charged state

// Battery Charge States
enum BatteryChargeState {
    BATTERY_CHARGE_UNKNOWN = 0,
    BATTERY_CHARGE_DISCHARGING = 1,
    BATTERY_CHARGE_CHARGING = 2,
    BATTERY_CHARGE_CHARGED = 3
};

// GNSS Configuration
#define GNSS_ENABLED        true     // Enable/disable GNSS by default
#define GNSS_BAUD_RATE      9600     // Standard GNSS baud rate
#define GNSS_TCP_PORT       10110    // TCP port for NMEA data streaming (standard NMEA-over-TCP port)

// LoRa Default Parameters (915 MHz ISM band for North America)
#define LORA_FREQUENCY      915.0    // MHz - US ISM band (902-928 MHz)
#define LORA_BANDWIDTH      125.0    // kHz
#define LORA_SPREADING      12       // SF12
#define LORA_CODINGRATE     7        // 4/7
#define LORA_SYNCWORD       0x1424   // Private network (SX126x format: 0x1424, compatible with SX127x 0x12)
#define LORA_POWER          20       // dBm
#define LORA_PREAMBLE       8        // symbols

// Board Pin Definitions (will be set at runtime)
extern int8_t RADIO_SCLK_PIN;
extern int8_t RADIO_MISO_PIN;
extern int8_t RADIO_MOSI_PIN;
extern int8_t RADIO_CS_PIN;
extern int8_t RADIO_DIO0_PIN;
extern int8_t RADIO_RST_PIN;
extern int8_t RADIO_DIO1_PIN;
extern int8_t RADIO_BUSY_PIN;

// Board identification
enum BoardType {
    BOARD_UNKNOWN = 0,
    BOARD_V3 = 3,
    BOARD_V4 = 4
};

extern BoardType BOARD_TYPE;

#endif // CONFIG_H
