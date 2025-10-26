#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <functional>

// Heltec WiFi LoRa 32 V4 pin definitions
static const int8_t LORA_NSS = 8;       // CS
static const int8_t LORA_SCLK = 9;      // SCK
static const int8_t LORA_MOSI = 10;     // MOSI
static const int8_t LORA_MISO = 11;     // MISO
static const int8_t LORA_RST = 12;      // RST
static const int8_t LORA_BUSY = 13;     // BUSY
static const int8_t LORA_DIO1 = 14;     // DIO1
static const int8_t LORA_PA_POWER = 7;  // PA Power control
static const int8_t LORA_PA_EN = 2;     // PA Enable
static const int8_t LORA_PA_TX_EN = 46; // PA TX Enable

class RadioHAL
{
public:
    using RxCB = std::function<void(const uint8_t *, size_t, int16_t, float)>;
    
    bool begin(RxCB cb);
    bool send(const uint8_t *buf, size_t len);
    void poll();
    
    // Configuration loading
    void loadConfig();
    
    // KISS parameter configuration
    bool setTxDelay(uint8_t delay_10ms);        // TX delay in 10ms units
    bool setPersist(uint8_t persist);           // Persistence parameter (0-255)
    bool setSlotTime(uint8_t slot_10ms);        // Slot time in 10ms units
    bool setFrequency(float freq_mhz);          // Set frequency in MHz
    bool setTxPower(int8_t power_dbm);          // Set TX power in dBm
    bool setBandwidth(float bw_khz);            // Set bandwidth in kHz
    bool setSpreadingFactor(uint8_t sf);        // Set spreading factor (7-12)
    bool setCodingRate(uint8_t cr);             // Set coding rate (5-8)
    
    // Get current configuration
    float getFrequency() const { return frequency; }
    int8_t getTxPower() const { return txPower; }
    float getBandwidth() const { return bandwidth; }
    uint8_t getSpreadingFactor() const { return spreadingFactor; }
    uint8_t getCodingRate() const { return codingRate; }
    uint8_t getTxDelay() const { return txDelay; }
    uint8_t getPersist() const { return persist; }
    uint8_t getSlotTime() const { return slotTime; }
    
    // Debug and test functions
    void runTransmissionTest();
    void checkHardwarePins();
    void testContinuousTransmission();
    bool testRadioHealth();
    void testSpiCommunication();

private:
    RxCB onRx;
    Module *mod = nullptr;
    SX1262 *lora = nullptr;
    
    // Radio configuration parameters
    float frequency = 915.0;        // MHz
    float bandwidth = 125.0;        // kHz
    uint8_t spreadingFactor = 9;    // 7-12
    uint8_t codingRate = 7;         // 5-8 (4/5 to 4/8)
    uint8_t syncWord = 5;           // LoRa sync word
    int8_t txPower = 8;             // dBm
    uint16_t preambleLength = 10;   // symbols
    
    // KISS timing parameters
    uint8_t txDelay = 30;           // TX delay in 10ms units (300ms default)
    uint8_t persist = 63;           // Persistence (0-255, 63 = ~25%)
    uint8_t slotTime = 10;          // Slot time in 10ms units (100ms default)
    
    // Channel access state
    bool channelBusy = false;
    unsigned long lastRxTime = 0;
    static const unsigned long RX_TIMEOUT_MS = 1000; // Consider channel free after 1s
    
    // Helper methods
    bool applyRadioConfig();
    bool checkChannelClear();
    void performCSMA();
    
    // Static callback for RadioLib
    static void irqHandler();
    static RadioHAL* instance;
};
