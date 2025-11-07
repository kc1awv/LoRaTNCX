#include "radio.h"
#include "board_config.h"

LoRaRadio* LoRaRadio::instance = nullptr;

LoRaRadio::LoRaRadio() 
    : radio(nullptr), spi(nullptr), module(nullptr),
      frequency(LORA_FREQUENCY), bandwidth(LORA_BANDWIDTH),
      spreadingFactor(LORA_SPREADING), codingRate(LORA_CODINGRATE),
      syncWord(LORA_SYNCWORD), outputPower(LORA_POWER),
      transmitting(false), lastTransmitTime(0), packetReceived(false) {
    instance = this;
}

bool LoRaRadio::begin() {
    return (beginWithState() == RADIOLIB_ERR_NONE);
}

int LoRaRadio::beginWithState() {
    // Create SPI instance
    spi = new SPIClass(HSPI);
    spi->begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    
    // Create module
    module = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, *spi);
    
    // Create radio instance
    radio = new SX1262(module);
    
    // Try with more conservative parameters first
    int state = radio->begin();
    
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    // Now configure parameters one by one
    state = radio->setFrequency(frequency);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setBandwidth(bandwidth);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setSpreadingFactor(spreadingFactor);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setCodingRate(codingRate);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setSyncWord(syncWord);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setOutputPower(outputPower);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    state = radio->setPreambleLength(LORA_PREAMBLE);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    // Set up interrupt for receive
    radio->setPacketReceivedAction(onDio1Action);
    
    // Start receiving
    state = radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    
    return RADIOLIB_ERR_NONE;
}

bool LoRaRadio::transmit(const uint8_t* data, size_t length) {
    if (transmitting) {
        return false; // Already transmitting
    }
    
    transmitting = true;
    
    // Stop receiving
    radio->standby();
    
    int state = radio->transmit(const_cast<uint8_t*>(data), length);
    
    transmitting = false;
    
    if (state == RADIOLIB_ERR_NONE) {
        // Record transmit time for deaf period
        lastTransmitTime = millis();
        // Resume receiving
        radio->startReceive();
        return true;
    } else {
        // Resume receiving
        radio->startReceive();
        return false;
    }
}

bool LoRaRadio::receive(uint8_t* buffer, size_t* length) {
    // Deaf period after transmit to avoid hearing our own transmission
    // Configurable via DEAF_PERIOD_MS in config.h (set to 0 to disable)
    #if DEAF_PERIOD_MS > 0
    if (millis() - lastTransmitTime < DEAF_PERIOD_MS) {
        // Clear the flag even during deaf period
        if (packetReceived) {
            packetReceived = false;
            radio->startReceive();  // Restart RX to clear buffer
        }
        return false;  // Still in deaf period, don't process receives
    }
    #endif
    
    // Only read data if the interrupt flag says we have a NEW packet
    if (!packetReceived) {
        return false;  // No new packet
    }
    
    // Clear the flag first
    packetReceived = false;
    
    // Read the packet
    int state = radio->readData(buffer, LORA_BUFFER_SIZE);
    
    if (state == RADIOLIB_ERR_NONE) {
        *length = radio->getPacketLength();
        
        // Restart receive mode to listen for next packet
        radio->startReceive();
        
        return true;
    } else {
        // Error reading - restart receive anyway
        radio->startReceive();
        return false;
    }
}

void LoRaRadio::setFrequency(float freq) {
    frequency = freq;
    radio->setFrequency(freq);
}

void LoRaRadio::setBandwidth(float bw) {
    bandwidth = bw;
    radio->setBandwidth(bw);
}

void LoRaRadio::setSpreadingFactor(uint8_t sf) {
    spreadingFactor = sf;
    radio->setSpreadingFactor(sf);
}

void LoRaRadio::setCodingRate(uint8_t cr) {
    codingRate = cr;
    radio->setCodingRate(cr);
}

void LoRaRadio::setSyncWord(uint16_t sw) {
    syncWord = sw;
    // SX126x uses 2-byte sync word
    radio->setSyncWord(sw);
}

void LoRaRadio::setOutputPower(int8_t power) {
    outputPower = power;
    radio->setOutputPower(power);
}

void LoRaRadio::reconfigure() {
    // Stop receiving
    radio->standby();
    
    // Apply all parameters
    radio->setFrequency(frequency);
    radio->setBandwidth(bandwidth);
    radio->setSpreadingFactor(spreadingFactor);
    radio->setCodingRate(codingRate);
    radio->setOutputPower(outputPower);
    
    // Resume receiving
    radio->startReceive();
}

void LoRaRadio::getCurrentConfig(LoRaConfig& config) {
    config.frequency = frequency;
    config.bandwidth = bandwidth;
    config.spreading = spreadingFactor;
    config.codingRate = codingRate;
    config.power = outputPower;
    config.syncWord = syncWord;
    config.preamble = LORA_PREAMBLE;
    config.magic = 0xCAFEBABE;  // Match ConfigManager magic
}

bool LoRaRadio::applyConfig(const LoRaConfig& config) {
    // Validate parameters
    if (config.frequency < 150.0 || config.frequency > 960.0) {
        return false;
    }
    if (config.spreading < 7 || config.spreading > 12) {
        return false;
    }
    if (config.codingRate < 5 || config.codingRate > 8) {
        return false;
    }
    if (config.power < -9 || config.power > 22) {
        return false;
    }
    
    // Apply parameters
    frequency = config.frequency;
    bandwidth = config.bandwidth;
    spreadingFactor = config.spreading;
    codingRate = config.codingRate;
    outputPower = config.power;
    syncWord = config.syncWord;
    
    // Reconfigure radio
    reconfigure();
    
    return true;
}

bool LoRaRadio::isTransmitting() {
    return transmitting;
}

int16_t LoRaRadio::getRSSI() {
    return radio->getRSSI();
}

float LoRaRadio::getSNR() {
    return radio->getSNR();
}

void IRAM_ATTR LoRaRadio::onDio1Action() {
    if (instance) {
        instance->handleInterrupt();
    }
}

void LoRaRadio::handleInterrupt() {
    // Interrupt handling - packet received
    // Set flag so main loop knows to read the packet
    packetReceived = true;
}
