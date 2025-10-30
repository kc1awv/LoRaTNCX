/**
 * @file LoRaRadio.h
 * @brief LoRa radio interface using proven ping/pong communication methods
 * @author LoRaTNCX Project
 * @date October 28, 2025
 *
 * This class encapsulates all the LoRa radio functionality using the
 * reliable communication methods established in our ping/pong testing.
 *
 * Key success factors incorporated:
 * - PA_POWER_PIN in ANALOG mode (factory firmware insight)
 * - 40ms total settling delays (20ms + 20ms)
 * - Proper PA control for TX/RX modes
 */

#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "HardwareConfig.h"

// LoRa configuration constants - using proven ping/pong settings
#define LORA_FREQUENCY 915.0    // MHz (adjust for your region)
#define LORA_BANDWIDTH 125.0    // kHz
#define LORA_SPREADING_FACTOR 7 // 7-12
#define LORA_CODING_RATE 5      // 5-8
#define LORA_OUTPUT_POWER 22    // dBm
#define LORA_PREAMBLE_LENGTH 8  // symbols
#define LORA_SYNC_WORD 0x12     // proven sync word from ping/pong

class LoRaRadio
{
public:
    /**
     * @brief Constructor
     */
    LoRaRadio();
    /**
     * @brief Initialize the LoRa radio with default configuration
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Initialize the LoRa radio with custom configuration
     * @param frequency Frequency in MHz
     * @param bandwidth Bandwidth in kHz
     * @param spreadingFactor Spreading factor (5-12)
     * @param codingRate Coding rate (5-8, representing 4/5 to 4/8)
     * @return true if initialization successful, false otherwise
     */
    bool begin(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate);

    /**
     * @brief Send a packet over LoRa
     * @param data Pointer to data buffer
     * @param length Length of data to send
     * @return true if transmission successful, false otherwise
     */
    bool transmit(const uint8_t *data, size_t length);

    /**
     * @brief Send a string over LoRa
     * @param message String message to send
     * @return true if transmission successful, false otherwise
     */
    bool transmit(const String &message);

    /**
     * @brief Check if a packet has been received
     * @return true if packet available, false otherwise
     */
    bool available();

    /**
     * @brief Read received packet
     * @param buffer Buffer to store received data
     * @param maxLength Maximum length to read
     * @return Number of bytes read, 0 if no data or error
     */
    size_t receive(uint8_t *buffer, size_t maxLength);

    /**
     * @brief Read received packet as string
     * @param message String to store received message
     * @return true if message received, false otherwise
     */
    bool receive(String &message);

    /**
     * @brief Get RSSI of last received packet
     * @return RSSI in dBm
     */
    float getRSSI();

    /**
     * @brief Get SNR of last received packet
     * @return SNR in dB
     */
    float getSNR();

    /**
     * @brief Get radio status information
     * @return Status string
     */
    String getStatus();

    /**
     * @brief Set frequency (requires radio reinitialization)
     * @param frequency Frequency in MHz
     * @return true if successful, false otherwise
     */
    bool setFrequency(float frequency);

    /**
     * @brief Set TX power (requires radio reinitialization)
     * @param power TX power in dBm
     * @return true if successful, false otherwise
     */
    bool setTxPower(int8_t power);

    /**
     * @brief Set spreading factor (requires radio reinitialization)
     * @param sf Spreading factor (5-12)
     * @return true if successful, false otherwise
     */
    bool setSpreadingFactor(uint8_t sf);

    /**
     * @brief Set bandwidth (requires radio reinitialization)
     * @param bw Bandwidth in kHz
     * @return true if successful, false otherwise
     */
    bool setBandwidth(float bw);

    /**
     * @brief Set coding rate (requires radio reinitialization)
     * @param cr Coding rate (5-8, representing 4/5 to 4/8)
     * @return true if successful, false otherwise
     */
    bool setCodingRate(uint8_t cr);

    /**
     * @brief Set sync word (requires radio reinitialization)
     * @param syncWord Sync word (0x00-0xFF)
     * @return true if successful, false otherwise
     */
    bool setSyncWord(uint8_t syncWord);

    /**
     * @brief Get current frequency
     * @return Current frequency in MHz
     */
    float getFrequency() const;

    /**
     * @brief Get current TX power
     * @return Current TX power in dBm
     */
    int8_t getTxPower() const;

    /**
     * @brief Get current spreading factor
     * @return Current spreading factor
     */
    uint8_t getSpreadingFactor() const;

    /**
     * @brief Get current bandwidth
     * @return Current bandwidth in kHz
     */
    float getBandwidth() const;

    /**
     * @brief Get current coding rate
     * @return Current coding rate
     */
    uint8_t getCodingRate() const;

    /**
     * @brief Get current sync word
     * @return Current sync word
     */
    uint8_t getSyncWord() const;

private:
    SX1262 *radio;         // RadioLib SX1262 instance pointer
    bool initialized;      // Initialization status
    unsigned long txCount; // Transmission counter
    unsigned long rxCount; // Reception counter
    float lastRSSI;        // Last RSSI reading
    float lastSNR;         // Last SNR reading

    // Current radio parameters
    float currentFrequency;
    int8_t currentTxPower;
    uint8_t currentSpreadingFactor;
    float currentBandwidth;
    uint8_t currentCodingRate;
    uint8_t currentSyncWord;

    /**
     * @brief Configure PA control pins and modes
     * Using proven methods from ping/pong testing
     */
    void initializePAControl();

    /**
     * @brief Set PA mode for transmission or reception
     * @param transmit true for TX mode, false for RX mode
     *
     * This uses the proven PA control method:
     * - PA_POWER_PIN in ANALOG mode (factory firmware insight)
     * - 20ms settling delays for reliable operation
     */
    void setPA(bool transmit);

    /**
     * @brief Initialize SPI interface
     */
    void initializeSPI();
};

#endif // LORA_RADIO_H