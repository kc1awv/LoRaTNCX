/**
 * @file HeltecV4Pins.h
 * @brief Pin reference for Heltec WiFi LoRa 32 V4 board
 * @author LoRaTNCX Project
 * @date October 28, 2025
 *
 * This file provides a reference of pin assignments for the Heltec V4 board.
 * The actual pin definitions are provided by the board's pins_arduino.h file.
 *
 * Pin definitions source: https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 */

#ifndef HELTEC_V4_PINS_H
#define HELTEC_V4_PINS_H

#include <Arduino.h>

// ==========================================
// PIN REFERENCE (for documentation)
// ==========================================
// These are the pin assignments from the official Heltec pins_arduino.h:
//
// LED_BUILTIN = 35
// TX = 43, RX = 44
// SDA = 3, SCL = 4 (but note: OLED uses different pins)
// SS = 8, MOSI = 10, MISO = 11, SCK = 9
//
// OLED Display:
// RST_OLED = 21, SCL_OLED = 18, SDA_OLED = 17
//
// LoRa Radio (SX1262):
// RST_LoRa = 12, BUSY_LoRa = 13, DIO0 = 14
// LORA_PA_POWER = 7, LORA_PA_EN = 2, LORA_PA_TX_EN = 46
//
// GNSS Module (optional attachment):
// VGNSS_CTL = 34, GNSS_RX = 38, GNSS_TX = 39
// GNSS_WAKE = 40, GNSS_PPS = 41, GNSS_RST = 42
//
// Power Control:
// Vext = 36 (External power control)
//
// Available GPIO pins for expansion:
// 1, 2, 3, 4, 5, 6, 7, 15, 16, 19, 20, etc.

// ==========================================
// DISPLAY CONFIGURATION
// ==========================================
#ifndef HELTEC_V4_DISPLAY_HEIGHT
#define HELTEC_V4_DISPLAY_HEIGHT 64
#endif

#ifndef HELTEC_V4_DISPLAY_WIDTH
#define HELTEC_V4_DISPLAY_WIDTH 128
#endif

#endif // HELTEC_V4_PINS_H