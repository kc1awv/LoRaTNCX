#include "radio.h"
#include "board_config.h"

LoRaRadio* LoRaRadio::instance = nullptr;

// PA gain values for V4 board (GC1109 power amplifier)
// Index corresponds to output power level (7-28 dBm)
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
static const int8_t paGainValues[PA_GAIN_POINTS] = PA_GAIN_VALUES;
#endif

// Calculate SX1262 power setting from desired output power for V4 boards
int8_t LoRaRadio::calculateSX1262Power(int8_t desiredOutputPower) {
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
    // For V4 boards with non-linear PA, calculate SX1262 power from gain table
    if (desiredOutputPower >= 7 && desiredOutputPower <= PA_MAX_OUTPUT) {
        // Map output power to gain table index (7dBm = index 0, 28dBm = index 21)
        uint8_t gainIndex = desiredOutputPower - 7;
        if (gainIndex < PA_GAIN_POINTS) {
            int8_t gain = paGainValues[gainIndex];
            int8_t sx1262Power = desiredOutputPower - gain;
            
            // Ensure SX1262 power stays within valid range (-9 to +22 dBm)
            if (sx1262Power < -9) {
                sx1262Power = -9;
            } else if (sx1262Power > 22) {
                sx1262Power = 22;
            }
            
            return sx1262Power;
        }
    }
    // Fall back to direct power for out-of-range values
    return desiredOutputPower;
#else
    // For V3 and other boards, use power directly
    return desiredOutputPower;
#endif
}

LoRaRadio::LoRaRadio() 
    : radio(nullptr), spi(nullptr), module(nullptr),
      frequency(LORA_FREQUENCY), bandwidth(LORA_BANDWIDTH),
      spreadingFactor(LORA_SPREADING), codingRate(LORA_CODINGRATE),
      syncWord(LORA_SYNCWORD), outputPower(LORA_POWER),
      transmitting(false), lastTransmitTime(0), packetReceived(false) {
    instance = this;
}

LoRaRadio::~LoRaRadio() {
    cleanup();
}

void LoRaRadio::cleanup() {
    // Clean up in reverse order of allocation
    if (radio) {
        delete radio;
        radio = nullptr;
    }
    if (module) {
        delete module;
        module = nullptr;
    }
    if (spi) {
        delete spi;
        spi = nullptr;
    }
}

bool LoRaRadio::begin() {
    return (beginWithState() == RADIOLIB_ERR_NONE);
}

int LoRaRadio::beginWithState() {
    // Clean up any existing allocations first
    cleanup();
    
    // Create SPI instance
    spi = new SPIClass(HSPI);
    if (!spi) {
        return RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED;
    }
    spi->begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    
    // Create module
    module = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, *spi);
    if (!module) {
        cleanup();  // Clean up SPI
        return RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED;
    }
    
    // Create radio instance
    radio = new SX1262(module);
    if (!radio) {
        cleanup();  // Clean up SPI and module
        return RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED;
    }
    
    // Try with more conservative parameters first
    int state = radio->begin();
    
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    // Now configure parameters one by one
    state = radio->setFrequency(frequency);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setBandwidth(bandwidth);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setSpreadingFactor(spreadingFactor);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setCodingRate(codingRate);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setSyncWord(syncWord);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setOutputPower(outputPower);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    state = radio->setPreambleLength(LORA_PREAMBLE);
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    // Set up interrupt for receive
    radio->setPacketReceivedAction(onDio1Action);
    
    // Start receiving
    state = radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        cleanup();  // Clean up all allocations
        return state;
    }
    
    return RADIOLIB_ERR_NONE;
}

bool LoRaRadio::transmit(const uint8_t* data, size_t length) {
    if (!isInitialized() || transmitting) {
        return false; // Not initialized or already transmitting
    }
    
    // Stop receiving first to prevent race conditions
    radio->standby();
    
    transmitting = true;
    
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
    if (!isInitialized()) {
        return false; // Not initialized
    }
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
    
    // Calculate the actual SX1262 power setting (handles V4 PA gain)
    int8_t sx1262Power = calculateSX1262Power(power);
    
    radio->setOutputPower(sx1262Power);
}

void LoRaRadio::reconfigure() {
    if (!isInitialized()) {
        return; // Not initialized
    }
    // Stop receiving
    radio->standby();
    
    // Apply all parameters
    radio->setFrequency(frequency);
    radio->setBandwidth(bandwidth);
    radio->setSpreadingFactor(spreadingFactor);
    radio->setCodingRate(codingRate);
    
    // Use calculated SX1262 power (handles V4 PA gain)
    int8_t sx1262Power = calculateSX1262Power(outputPower);
    radio->setOutputPower(sx1262Power);
    
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
    if (config.power < RADIO_POWER_MIN || config.power > RADIO_POWER_MAX) {
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
    if (!isInitialized()) {
        return 0; // Not initialized
    }
    return radio->getRSSI();
}

float LoRaRadio::getSNR() {
    if (!isInitialized()) {
        return 0.0f; // Not initialized
    }
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
