/**
 * @file LoRaRadio.cpp
 * @brief LoRa radio implementation using RadioLib for SX1262
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "LoRaRadio.h"

LoRaRadio::LoRaRadio() : 
    radio(nullptr),
    spi(nullptr),
    currentFrequency(DEFAULT_FREQUENCY),
    currentTxPower(DEFAULT_TX_POWER),
    currentBandwidth(DEFAULT_BANDWIDTH),
    currentSpreadingFactor(DEFAULT_SPREADING_FACTOR),
    currentCodingRate(DEFAULT_CODING_RATE),
    lastRSSI(0.0),
    lastSNR(0.0),
    initialized(false)
{
}

bool LoRaRadio::begin() {
    Serial.println("Initializing LoRa radio...");
    
    // Initialize PA pins first (critical for proper operation)
    initializePAPins();
    
    // Wait for PA pins to stabilize
    delay(100);
    
    // Initialize SPI explicitly
    Serial.println("Initializing SPI...");
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
    spi = &SPI;
    
    // Wait for SPI initialization
    delay(50);
    
    // Create SX1262 instance with pin configuration
    Serial.printf("Creating SX1262 instance: SS=%d, DIO0=%d, RST=%d, BUSY=%d\n", 
                  LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN);
    radio = new SX1262(new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN, *spi));
    
    // Wait before initialization
    delay(100);
    
    // Initialize the radio
    Serial.print("SX1262 initialization... ");
    int state = radio->begin(currentFrequency, currentBandwidth, currentSpreadingFactor, currentCodingRate, 0x12, currentTxPower);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("success!");
        initialized = true;
        
        // Print configuration
        Serial.printf("Frequency: %.1f MHz\n", currentFrequency);
        Serial.printf("Bandwidth: %.1f kHz\n", currentBandwidth);
        Serial.printf("Spreading Factor: %d\n", currentSpreadingFactor);
        Serial.printf("Coding Rate: 4/%d\n", currentCodingRate);
        Serial.printf("TX Power: %d dBm\n", currentTxPower);
        
        return true;
    } else {
        Serial.print("failed, code ");
        Serial.println(state);
        return false;
    }
}

void LoRaRadio::initializePAPins() {
    Serial.println("Initializing Power Amplifier pins...");
    
    // Initialize PA pins exactly as shown in Heltec factory firmware
    pinMode(LORA_PA_POWER_PIN, ANALOG);     // PA power control
    pinMode(LORA_PA_EN_PIN, OUTPUT);        // PA enable
    pinMode(LORA_PA_TX_EN_PIN, OUTPUT);     // PA TX enable
    
    // Enable PA for transmission
    digitalWrite(LORA_PA_EN_PIN, HIGH);
    digitalWrite(LORA_PA_TX_EN_PIN, HIGH);
    
    Serial.printf("PA pins initialized: Power=%d, EN=%d, TX_EN=%d\n", 
                  LORA_PA_POWER_PIN, LORA_PA_EN_PIN, LORA_PA_TX_EN_PIN);
}

bool LoRaRadio::setFrequency(float frequency) {
    if (!initialized || !validateFrequency(frequency)) {
        return false;
    }
    
    int state = radio->setFrequency(frequency);
    if (state == RADIOLIB_ERR_NONE) {
        currentFrequency = frequency;
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Frequency set to %.1f MHz\n", frequency);
        #endif
        return true;
    } else {
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Failed to set frequency, code %d\n", state);
        #endif
        return false;
    }
}

bool LoRaRadio::setTxPower(int8_t power) {
    if (!initialized) return false;
    
    // Limit power for amateur radio compliance
    if (power > 20) power = 20;  // Max 20 dBm for SX1262
    if (power < -9) power = -9;  // Min power
    
    int state = radio->setOutputPower(power);
    if (state == RADIOLIB_ERR_NONE) {
        currentTxPower = power;
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("TX Power set to %d dBm\n", power);
        #endif
        return true;
    } else {
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Failed to set TX power, code %d\n", state);
        #endif
        return false;
    }
}

bool LoRaRadio::setBandwidth(float bandwidth) {
    if (!initialized) return false;
    
    int state = radio->setBandwidth(bandwidth);
    if (state == RADIOLIB_ERR_NONE) {
        currentBandwidth = bandwidth;
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Bandwidth set to %.1f kHz\n", bandwidth);
        #endif
        return true;
    } else {
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Failed to set bandwidth, code %d\n", state);
        #endif
        return false;
    }
}

bool LoRaRadio::setSpreadingFactor(uint8_t sf) {
    if (!initialized || sf < 5 || sf > 12) return false;
    
    int state = radio->setSpreadingFactor(sf);
    if (state == RADIOLIB_ERR_NONE) {
        currentSpreadingFactor = sf;
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Spreading Factor set to %d\n", sf);
        #endif
        return true;
    } else {
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Failed to set spreading factor, code %d\n", state);
        #endif
        return false;
    }
}

bool LoRaRadio::setCodingRate(uint8_t cr) {
    if (!initialized || cr < 5 || cr > 8) return false;
    
    int state = radio->setCodingRate(cr);
    if (state == RADIOLIB_ERR_NONE) {
        currentCodingRate = cr;
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Coding Rate set to 4/%d\n", cr);
        #endif
        return true;
    } else {
        #if DEBUG_RADIO_VERBOSE
        Serial.printf("Failed to set coding rate, code %d\n", state);
        #endif
        return false;
    }
}

int LoRaRadio::transmit(const uint8_t* data, size_t length) {
    if (!initialized) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    
    Serial.printf("Transmitting %d bytes...\n", length);
    int state = radio->transmit(const_cast<uint8_t*>(data), length);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Transmission successful!");
    } else {
        Serial.printf("Transmission failed, code %d\n", state);
    }
    
    return state;
}

int LoRaRadio::transmit(const String& str) {
    return transmit((const uint8_t*)str.c_str(), str.length());
}

int LoRaRadio::receive(uint8_t* data, size_t maxLength) {
    if (!initialized) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    
    int state = radio->receive(data, maxLength);
    
    if (state > 0) {
        // Successful reception
        lastRSSI = radio->getRSSI();
        lastSNR = radio->getSNR();
        Serial.printf("Received %d bytes, RSSI: %.1f dBm, SNR: %.1f dB\n", 
                      state, lastRSSI, lastSNR);
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        // Timeout - not necessarily an error (normal when no radio activity)
        // Serial.println("RX timeout");  // Commented out to reduce console spam
    } else {
        Serial.printf("Reception failed, code %d\n", state);
    }
    
    return state;
}

int LoRaRadio::startReceive() {
    if (!initialized) return RADIOLIB_ERR_CHIP_NOT_FOUND;
    
    // For RadioLib, we don't need to explicitly start receive mode
    // The receive() method handles this automatically
    Serial.println("Ready for receive operations");
    return RADIOLIB_ERR_NONE;
}

bool LoRaRadio::isReceiveComplete() {
    if (!initialized) return false;
    // RadioLib doesn't provide isReceiveComplete, we'll use polling in receive()
    return false;
}

float LoRaRadio::getLastRSSI() {
    return lastRSSI;
}

float LoRaRadio::getLastSNR() {
    return lastSNR;
}

bool LoRaRadio::isTransmitting() {
    if (!initialized) return false;
    // RadioLib doesn't have isTransmitting, we'll track state internally
    // For now, assume transmission is complete immediately
    return false;
}

void LoRaRadio::sleep() {
    if (initialized) {
        radio->sleep();
        Serial.println("Radio entering sleep mode");
    }
}

void LoRaRadio::standby() {
    if (initialized) {
        radio->standby();
        Serial.println("Radio in standby mode");
    }
}

bool LoRaRadio::validateFrequency(float frequency) {
    // Basic amateur radio frequency validation
    // This is a simplified check - adjust for your local regulations
    
    // 70cm band: 420-450 MHz (varies by region)
    if (frequency >= 420.0 && frequency <= 450.0) return true;
    
    // 33cm band: 902-928 MHz (US)
    if (frequency >= 902.0 && frequency <= 928.0) return true;
    
    // Add other amateur bands as needed
    
    Serial.printf("Warning: Frequency %.1f MHz may not be in amateur band\n", frequency);
    return true; // Allow for testing, but warn
}