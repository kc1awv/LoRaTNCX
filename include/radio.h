#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <RadioLib.h>
#include "config.h"
#include "config_manager.h"

/**
 * @brief LoRa Radio Interface using SX1262 module
 *
 * This class provides a high-level interface to the SX1262 LoRa radio module
 * through the RadioLib library. It handles initialization, configuration,
 * transmission, and reception of LoRa packets.
 *
 * Key features:
 * - Automatic SPI bus management and module initialization
 * - Parameter validation and range checking
 * - Interrupt-driven packet reception
 * - Configuration persistence through ConfigManager
 * - Thread-safe operation with ESP32 multi-core considerations
 */
class LoRaRadio {
public:
    /**
     * @brief Construct a new LoRa Radio interface
     */
    LoRaRadio();
    
    /**
     * @brief Destroy the LoRa Radio interface and cleanup resources
     */
    ~LoRaRadio();
    
    /**
     * @brief Initialize the LoRa radio module
     *
     * Sets up SPI communication, configures the SX1262 module, and applies
     * default LoRa parameters. Must be called before using other radio functions.
     *
     * @return true if initialization successful
     * @return false if initialization failed (SPI, module, or radio issues)
     */
    bool begin();
    
    /**
     * @brief Initialize the radio and return detailed error code
     *
     * Similar to begin() but returns RadioLib error codes for detailed diagnostics.
     *
     * @return RadioLib error code (0 = success, negative = error)
     */
    int beginWithState();
    
    /**
     * @brief Check if the radio is properly initialized and ready for use
     *
     * @return true if radio, SPI, and module are all initialized
     * @return false if any component is not ready
     */
    bool isInitialized() const { return radio != nullptr && spi != nullptr && module != nullptr; }
    
    /**
     * @brief Manually cleanup radio resources
     *
     * Normally called automatically by destructor, but can be called earlier
     * if needed. Safe to call multiple times.
     */
    void cleanup();
    
    /**
     * @brief Transmit data over LoRa
     *
     * Sends a packet containing the provided data. Transmission is asynchronous
     * and this method returns immediately. Use isTransmitting() to check status.
     *
     * @param data Pointer to the data buffer to transmit
     * @param length Number of bytes to transmit (must be <= LORA_BUFFER_SIZE)
     * @return true if transmission started successfully
     * @return false if radio not ready, invalid parameters, or transmission failed
     */
    bool transmit(const uint8_t* data, size_t length);
    
    /**
     * @brief Check for and retrieve received LoRa packets
     *
     * Non-blocking check for received packets. If a packet is available,
     * copies it to the provided buffer and returns the length.
     *
     * @param buffer Buffer to store received packet data
     * @param length Pointer to store the received packet length
     * @return true if a packet was received and copied to buffer
     * @return false if no packet available or reception error
     */
    bool receive(uint8_t* buffer, size_t* length);
    
    /**
     * @brief Set the LoRa carrier frequency
     *
     * Changes the operating frequency. Valid range depends on regional regulations
     * and SX1262 capabilities (typically 150-960 MHz).
     *
     * @param freq Frequency in MHz (e.g., 433.0, 868.0, 915.0)
     */
    void setFrequency(float freq);
    
    /**
     * @brief Set the LoRa signal bandwidth
     *
     * Affects data rate and receiver sensitivity. Wider bandwidth allows
     * higher data rates but reduces sensitivity.
     *
     * @param bw Bandwidth in kHz (typically 125.0, 250.0, or 500.0)
     */
    void setBandwidth(float bw);
    
    /**
     * @brief Set the LoRa spreading factor
     *
     * Controls the spreading factor (7-12). Higher values increase range
     * and robustness but decrease data rate.
     *
     * @param sf Spreading factor (7-12)
     */
    void setSpreadingFactor(uint8_t sf);
    
    /**
     * @brief Set the LoRa forward error correction coding rate
     *
     * Controls the coding rate (5-8). Higher values provide better error
     * correction but reduce data rate. Format is 4/CR (e.g., 5 = 4/5).
     *
     * @param cr Coding rate (5-8)
     */
    void setCodingRate(uint8_t cr);
    
    /**
     * @brief Set the LoRa synchronization word
     *
     * 16-bit sync word for network identification. Allows multiple networks
     * to coexist on the same frequency without interference.
     *
     * @param sw 16-bit synchronization word
     */
    void setSyncWord(uint16_t sw);
    
    /**
     * @brief Set the LoRa transmit output power
     *
     * Controls transmit power in dBm. Valid range depends on module and
     * regional regulations (typically -9 to +22 dBm for SX1262).
     *
     * @param power Output power in dBm
     */
    void setOutputPower(int8_t power);
    
    /**
     * @brief Get the current carrier frequency
     *
     * @return Current frequency in MHz
     */
    float getFrequency() const { return frequency; }
    
    /**
     * @brief Get the current signal bandwidth
     *
     * @return Current bandwidth in kHz
     */
    float getBandwidth() const { return bandwidth; }
    
    /**
     * @brief Get the current spreading factor
     *
     * @return Current spreading factor (7-12)
     */
    uint8_t getSpreadingFactor() const { return spreadingFactor; }
    
    /**
     * @brief Get the current coding rate
     *
     * @return Current coding rate (5-8)
     */
    uint8_t getCodingRate() const { return codingRate; }
    
    /**
     * @brief Get the current output power
     *
     * @return Current output power in dBm
     */
    int8_t getOutputPower() const { return outputPower; }
    
    /**
     * @brief Get the current synchronization word
     *
     * @return Current 16-bit synchronization word
     */
    uint16_t getSyncWord() const { return syncWord; }
    
    /**
     * @brief Get the current radio configuration as a struct
     *
     * Fills the provided LoRaConfig struct with current radio parameters.
     *
     * @param config Reference to LoRaConfig struct to fill
     */
    void getCurrentConfig(LoRaConfig& config);
    
    /**
     * @brief Apply configuration from a LoRaConfig struct
     *
     * Updates all radio parameters from the provided configuration.
     * Does not automatically reconfigure the radio hardware.
     *
     * @param config Reference to LoRaConfig struct containing new parameters
     * @return true if all parameters were valid and applied
     * @return false if any parameter was invalid
     */
    bool applyConfig(const LoRaConfig& config);
    
    /**
     * @brief Apply all current parameters to the radio hardware
     *
     * Reconfigures the SX1262 module with current parameter values.
     * Must be called after changing parameters to take effect.
     * This is a potentially time-consuming operation.
     */
    void reconfigure();
    
    /**
     * @brief Check if the radio is currently transmitting
     *
     * @return true if a transmission is in progress
     * @return false if radio is idle or receiving
     */
    bool isTransmitting();
    
    /**
     * @brief Get the current Received Signal Strength Indicator
     *
     * Returns RSSI of the last received packet or current signal level.
     *
     * @return RSSI value in dBm (negative values, e.g., -80)
     */
    int16_t getRSSI();
    
    /**
     * @brief Get the current Signal-to-Noise Ratio
     *
     * Returns SNR of the last received packet.
     *
     * @return SNR value in dB
     */
    float getSNR();
    
private:
    SX1262* radio;
    SPIClass* spi;
    Module* module;
    
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint16_t syncWord;  // Changed to uint16_t for SX126x 2-byte sync word
    int8_t outputPower;
    
    bool transmitting;
    unsigned long lastTransmitTime;  // Track when we last transmitted
    volatile bool packetReceived;    // Flag set by interrupt when packet arrives
    
    /**
     * @brief Calculate SX1262 power setting from desired output power for V4 boards
     *
     * For Heltec V4 boards with non-linear PA, this function maps the desired
     * output power to the appropriate SX1262 input power using a gain lookup table.
     *
     * @param desiredOutputPower Desired output power in dBm (7-28 for V4)
     * @return SX1262 power setting in dBm (-9 to +22)
     */
    int8_t calculateSX1262Power(int8_t desiredOutputPower);
    
    static void IRAM_ATTR onDio1Action();
    static LoRaRadio* instance;
    
    void handleInterrupt();
};

#endif // RADIO_H
