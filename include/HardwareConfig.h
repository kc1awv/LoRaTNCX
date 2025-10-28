/**
 * @file HardwareConfig.h
 * @brief Hardware configuration for LoRaTNCX project
 * @author LoRaTNCX Project
 * @date October 28, 2025
 * 
 * This file defines the hardware configuration and pin assignments
 * specifically for the LoRa TNC application on Heltec WiFi LoRa 32 V4
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>
#include "HeltecV4Pins.h"

// ==========================================
// LORA RADIO CONFIGURATION
// ==========================================
#define LORA_SS_PIN        SS        // SPI Slave Select (Pin 8)
#define LORA_RST_PIN       RST_LoRa  // Reset pin (Pin 12)
#define LORA_DIO0_PIN      DIO0      // DIO0/IRQ pin (Pin 14)
#define LORA_BUSY_PIN      BUSY_LoRa // Busy pin (Pin 13)

// LoRa SPI configuration
#define LORA_SCK_PIN       SCK       // SPI Clock (Pin 9)
#define LORA_MOSI_PIN      MOSI      // SPI MOSI (Pin 10)
#define LORA_MISO_PIN      MISO      // SPI MISO (Pin 11)

// ==========================================
// DISPLAY CONFIGURATION
// ==========================================
#define OLED_SDA_PIN       SDA_OLED  // OLED I2C Data (Pin 17)
#define OLED_SCL_PIN       SCL_OLED  // OLED I2C Clock (Pin 18)
#define OLED_RST_PIN       RST_OLED  // OLED Reset (Pin 21)
#define OLED_ADDRESS       0x3C      // Standard OLED I2C address

// Enable/disable display
#define DISPLAY_ENABLED    true

// ==========================================
// STATUS LED CONFIGURATION
// ==========================================
#define STATUS_LED_PIN     LED_BUILTIN  // Built-in LED (Pin 35)
#define LED_ON             HIGH
#define LED_OFF            LOW

// ==========================================
// POWER MANAGEMENT
// ==========================================
#define POWER_CTRL_PIN     Vext      // External power control (Pin 36)
#define POWER_ON           LOW       // Active low
#define POWER_OFF          HIGH

// ==========================================
// KISS TNC INTERFACE
// ==========================================
// Using USB CDC for KISS interface (default Serial)
#define KISS_SERIAL        Serial
#define KISS_BAUD_RATE     115200

// Optional: Hardware serial for debugging
#define DEBUG_SERIAL       Serial
#define DEBUG_BAUD_RATE    115200

// ==========================================
// RADIO CONFIGURATION DEFAULTS
// ==========================================
#define DEFAULT_FREQUENCY     915.0     // MHz (adjust for your region)
#define DEFAULT_TX_POWER      1         // dBm
#define DEFAULT_BANDWIDTH     125.0     // kHz
#define DEFAULT_SPREADING_FACTOR  7     // SF7
#define DEFAULT_CODING_RATE   5         // 4/5

// ==========================================
// TNC CONFIGURATION
// ==========================================
#define TNC_CALLSIGN       "NOCALL"    // Default callsign
#define TNC_SSID           0           // Default SSID
#define TNC_MAX_PACKET_SIZE 256        // Maximum packet size
#define TNC_BUFFER_SIZE    512         // Buffer size for packets

// ==========================================
// TIMING CONFIGURATION
// ==========================================
#define LORA_RX_TIMEOUT    30000       // LoRa RX timeout (ms)
#define STATUS_LED_BLINK   500         // Status LED blink rate (ms)
#define DISPLAY_UPDATE     1000        // Display update rate (ms)

// ==========================================
// GNSS MODULE CONFIGURATION
// ==========================================
#define GNSS_VCTL_PIN      34          // GNSS power control (VGNSS Ctl)
#define GNSS_RX_PIN        38          // GNSS UART RX
#define GNSS_TX_PIN        39          // GNSS UART TX
#define GNSS_WAKE_PIN      40          // GNSS wake/enable
#define GNSS_PPS_PIN       41          // GNSS pulse per second
#define GNSS_RST_PIN       42          // GNSS reset

// GNSS Serial configuration (if using Hardware Serial)
#define GNSS_BAUD_RATE     9600        // Standard GNSS baud rate
#define GNSS_ENABLED       false       // Enable/disable GNSS by default

// ==========================================
// AVAILABLE GPIO PINS
// ==========================================
// These pins are available for future expansion:
// GPIO 1, 2, 3, 4, 5, 6, 7, 15, 16, 19, 20
// 
// Reserved/Used pins:
// - Pin 8: LoRa SS
// - Pin 9: SPI SCK
// - Pin 10: SPI MOSI
// - Pin 11: SPI MISO
// - Pin 12: LoRa RST
// - Pin 13: LoRa BUSY
// - Pin 14: LoRa DIO0
// - Pin 17: OLED SDA
// - Pin 18: OLED SCL
// - Pin 21: OLED RST
// - Pin 34: GNSS power control
// - Pin 35: Built-in LED
// - Pin 36: Power control
// - Pin 38: GNSS RX
// - Pin 39: GNSS TX
// - Pin 40: GNSS wake
// - Pin 41: GNSS PPS
// - Pin 42: GNSS reset
// - Pin 43: UART TX
// - Pin 44: UART RX

#endif // HARDWARE_CONFIG_H