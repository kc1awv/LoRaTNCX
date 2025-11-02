#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include "FrequencyBands.h"

// Pin definitions for Heltec WiFi LoRa 32 boards
// Both V3 and V4 use SX1262 LoRa chip with similar pin configuration

// Common LoRa pins for both V3 and V4
#define LORA_NSS 8   // SPI Chip Select
#define LORA_DIO1 14 // DIO1 pin (was DIO0 in older terminology)
#define LORA_NRST 12 // Reset pin
#define LORA_BUSY 13 // Busy pin
#define LORA_SCK 9   // SPI Clock
#define LORA_MISO 11 // SPI MISO
#define LORA_MOSI 10 // SPI MOSI

// V4 specific PA (Power Amplifier) control pins
#ifdef ARDUINO_heltec_wifi_lora_32_V4
#define LORA_PA_EN 36    // PA Enable
#define LORA_PA_TX_EN 37 // PA TX Enable
#define LORA_PA_POWER 38 // PA Power control
#endif

// Hardware capabilities are now handled by FrequencyBandManager

// Default LoRa parameters (RadioLib compatible)
#define LORA_BANDWIDTH_DEFAULT 125.0    // 125 kHz
#define LORA_SPREADING_FACTOR_DEFAULT 7 // SF7
#define LORA_CODINGRATE_DEFAULT 5       // CR 4/5 (RadioLib uses denominator)
#define LORA_PREAMBLE_LENGTH_DEFAULT 8  // 8 symbols
#define LORA_SYNC_WORD_DEFAULT 0x12     // LoRaWAN public sync word

// Power limits for different hardware versions
#ifdef ARDUINO_heltec_wifi_lora_32_V3
#define LORA_MAX_TX_POWER 22     // V3 maximum power in dBm
#define LORA_DEFAULT_TX_POWER 10 // V3 default power in dBm
#elif defined(ARDUINO_heltec_wifi_lora_32_V4)
#define LORA_MAX_TX_POWER 22     // V4 maximum power in dBm (22dBm without PA)
#define LORA_DEFAULT_TX_POWER 14 // V4 default power in dBm
#else
#define LORA_MAX_TX_POWER 14     // Safe default
#define LORA_DEFAULT_TX_POWER 10 // Safe default
#endif

// Buffer size for LoRa packets
#define LORA_BUFFER_SIZE 255

typedef enum
{
    LORA_STATE_IDLE,
    LORA_STATE_TX,
    LORA_STATE_RX,
    LORA_STATE_SLEEP
} lora_state_t;

typedef struct
{
    float frequency; // MHz
    int8_t txPower;  // dBm
    float bandwidth; // kHz
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint16_t preambleLength;
    uint8_t syncWord;
    bool crcEnabled;
} lora_config_t;

class LoRaRadio
{
private:
    SX1262 radio;
    lora_config_t config;
    lora_state_t currentState;
    
    // Frequency band management
    FrequencyBandManager* bandManager;

    // Callback function pointers
    void (*onTxDoneCallback)(void);
    void (*onTxTimeoutCallback)(void);
    void (*onRxDoneCallback)(uint8_t *payload, uint16_t size, int16_t rssi, float snr);
    void (*onRxTimeoutCallback)(void);
    void (*onRxErrorCallback)(void);

    // Statistics
    uint32_t txCount;
    uint32_t rxCount;
    int16_t lastRssi;
    float lastSnr;

    // Hardware-specific initialization
    void initializeHardware();
    void configurePowerAmplifier(bool enable);

    // Interrupt flag
    volatile bool transmitFlag;
    volatile bool receiveFlag;
    volatile bool enableInterrupt;

    // Static callback wrapper for RadioLib
    static void setFlag(void);
    static LoRaRadio *instance;

public:
    LoRaRadio();
    ~LoRaRadio();

    // Initialization and configuration
    bool begin();
    bool begin(float frequency);
    bool begin(float frequency, int8_t txPower);

    // Configuration methods
    int setFrequency(float frequency);
    int setTxPower(int8_t power);
    int setBandwidth(float bandwidth);
    int setSpreadingFactor(uint8_t spreadingFactor);
    int setCodingRate(uint8_t codingRate);
    int setPreambleLength(uint16_t length);
    int setSyncWord(uint8_t syncWord);
    int setCRC(bool enable);
    
    // Frequency band methods
    bool selectBand(const String& bandId);
    bool setFrequencyWithBand(float frequency);
    FrequencyBandManager* getBandManager() { return bandManager; }

    // Get current configuration
    lora_config_t getConfig() const { return config; }
    float getFrequency() const { return config.frequency; }
    int8_t getTxPower() const { return config.txPower; }
    lora_state_t getState() const { return currentState; }

    // Statistics
    uint32_t getTxCount() const { return txCount; }
    uint32_t getRxCount() const { return rxCount; }
    int16_t getLastRssi() const { return lastRssi; }
    float getLastSnr() const { return lastSnr; }

    // Communication methods
    int send(const uint8_t *payload, uint8_t length);
    int send(const String &message);
    int startReceive();
    int startReceive(uint32_t timeout);
    int sleep();
    int standby();

    // Callback registration
    void onTxDone(void (*callback)(void));
    void onTxTimeout(void (*callback)(void));
    void onRxDone(void (*callback)(uint8_t *payload, uint16_t size, int16_t rssi, float snr));
    void onRxTimeout(void (*callback)(void));
    void onRxError(void (*callback)(void));

    // Utility methods
    bool isFrequencyValid(float frequency);
    bool isTxPowerValid(int8_t power);
    void printConfiguration();
    void printStatistics();
    void printAvailableBands();
    void printCurrentBand();

    // Process radio events (call this in main loop)
    void handle();

    // Get raw RadioLib object for advanced usage
    SX1262 &getRadio() { return radio; }
};