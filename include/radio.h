#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <RadioLib.h>
#include "config.h"
#include "config_manager.h"

class LoRaRadio {
public:
    LoRaRadio();
    
    // Initialize the radio
    bool begin();
    int beginWithState();  // Returns RadioLib error code instead of bool
    
    // Transmit data
    bool transmit(const uint8_t* data, size_t length);
    
    // Check for received data
    bool receive(uint8_t* buffer, size_t* length);
    
    // Set radio parameters
    void setFrequency(float freq);
    void setBandwidth(float bw);
    void setSpreadingFactor(uint8_t sf);
    void setCodingRate(uint8_t cr);
    void setSyncWord(uint16_t sw);  // Changed to uint16_t for SX126x 2-byte sync word
    void setOutputPower(int8_t power);
    
    // Get current parameters
    float getFrequency() const { return frequency; }
    float getBandwidth() const { return bandwidth; }
    uint8_t getSpreadingFactor() const { return spreadingFactor; }
    uint8_t getCodingRate() const { return codingRate; }
    int8_t getOutputPower() const { return outputPower; }
    uint16_t getSyncWord() const { return syncWord; }  // Changed to uint16_t
    
    // Get current configuration as struct
    void getCurrentConfig(LoRaConfig& config);
    
    // Apply configuration from struct
    bool applyConfig(const LoRaConfig& config);
    
    // Apply all current parameters to radio
    void reconfigure();
    
    // Get radio status
    bool isTransmitting();
    int16_t getRSSI();
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
    
    static void IRAM_ATTR onDio1Action();
    static LoRaRadio* instance;
    
    void handleInterrupt();
};

#endif // RADIO_H
