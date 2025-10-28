/**
 * @file LoRaRadio.h
 * @brief LoRa radio interface using RadioLib for SX1262
 * @author LoRaTNCX Project
 * @date October 28, 2025
 * 
 * This file provides a wrapper around RadioLib for the SX1262 chip
 * on the Heltec WiFi LoRa 32 V4 board, configured for amateur radio use.
 */

#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <Arduino.h>
#include <RadioLib.h>
#include "HardwareConfig.h"

class LoRaRadio {
public:
    LoRaRadio();
    
    // Initialization and configuration
    bool begin();
    bool setFrequency(float frequency);
    bool setTxPower(int8_t power);
    bool setBandwidth(float bandwidth);
    bool setSpreadingFactor(uint8_t sf);
    bool setCodingRate(uint8_t cr);
    
    // Transmission and reception
    int transmit(const uint8_t* data, size_t length);
    int transmit(const String& str);
    int receive(uint8_t* data, size_t maxLength);
    int startReceive();
    bool isReceiveComplete();
    
    // Status and information
    float getLastRSSI();
    float getLastSNR();
    bool isTransmitting();
    
    // Power management
    void sleep();
    void standby();
    
private:
    SX1262* radio;
    SPIClass* spi;
    
    // Configuration values
    float currentFrequency;
    int8_t currentTxPower;
    float currentBandwidth;
    uint8_t currentSpreadingFactor;
    uint8_t currentCodingRate;
    
    // Status values
    float lastRSSI;
    float lastSNR;
    bool initialized;
    
    // Private methods
    void initializePAPins();
    bool validateFrequency(float frequency);
};

#endif // LORA_RADIO_H