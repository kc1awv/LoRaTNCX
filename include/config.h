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
#define CMD_RETURN      0xFF  // Exit KISS mode

// SETHARDWARE Sub-commands for LoRa Configuration
#define HW_SET_FREQUENCY    0x01  // Set frequency (4 bytes, float MHz)
#define HW_SET_BANDWIDTH    0x02  // Set bandwidth (1 byte: 0=125, 1=250, 2=500 kHz)
#define HW_SET_SPREADING    0x03  // Set spreading factor (1 byte: 7-12)
#define HW_SET_CODINGRATE   0x04  // Set coding rate (1 byte: 5-8 for 4/5 to 4/8)
#define HW_SET_POWER        0x05  // Set TX power (1 byte: dBm)
#define HW_GET_CONFIG       0x06  // Get current configuration
#define HW_SAVE_CONFIG      0x07  // Save configuration to flash
#define HW_RESET_CONFIG     0x08  // Reset to defaults
#define HW_SET_SYNCWORD     0x09  // Set sync word (2 bytes for SX126x)

// LoRa Default Parameters (915 MHz ISM band for North America)
#define LORA_FREQUENCY      915.0    // MHz - US ISM band (902-928 MHz)
#define LORA_BANDWIDTH      125.0    // kHz
#define LORA_SPREADING      12       // SF12
#define LORA_CODINGRATE     7        // 4/7
#define LORA_SYNCWORD       0x1424   // Private network (SX126x format: 0x1424, compatible with SX127x 0x12)
#define LORA_POWER          20       // dBm
#define LORA_PREAMBLE       8        // symbols

// Deaf Period - prevents receiving own transmissions when radios are close
// Set to 0 to disable. For SF12, packets take ~1-2 seconds to transmit.
#define DEAF_PERIOD_MS      2000     // milliseconds to ignore RX after TX

// Buffer Sizes
#define SERIAL_BUFFER_SIZE  512
#define LORA_BUFFER_SIZE    256

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
