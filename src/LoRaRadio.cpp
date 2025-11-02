#include "LoRaRadio.h"
#include "FrequencyBands.h"

// Static instance pointer for callbacks
LoRaRadio *LoRaRadio::instance = nullptr;

// Static callback wrapper for RadioLib interrupts
void LoRaRadio::setFlag(void)
{
    if (instance)
    {
        // Set the appropriate flag based on current state
        if (instance->currentState == LORA_STATE_TX)
        {
            instance->transmitFlag = true;
        }
        else if (instance->currentState == LORA_STATE_RX)
        {
            instance->receiveFlag = true;
        }
    }
}

LoRaRadio::LoRaRadio() : radio(new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY))
{
    // Set this as the singleton instance
    instance = this;

    // Initialize frequency band manager
    bandManager = new FrequencyBandManager();

    // Initialize default configuration based on band manager
    if (bandManager->getCurrentBand()) {
        config.frequency = bandManager->getCurrentFrequency();
    } else {
        // Set safe default band (North American ISM)
        config.frequency = 915.0; // Default to 915 MHz (902-928 MHz range)
        bandManager->selectBand(BAND_ISM_902_928);
    }

    config.txPower = LORA_DEFAULT_TX_POWER;
    config.bandwidth = LORA_BANDWIDTH_DEFAULT;
    config.spreadingFactor = LORA_SPREADING_FACTOR_DEFAULT;
    config.codingRate = LORA_CODINGRATE_DEFAULT;
    config.preambleLength = LORA_PREAMBLE_LENGTH_DEFAULT;
    config.syncWord = LORA_SYNC_WORD_DEFAULT;
    config.crcEnabled = true;

    // Initialize state and callbacks
    currentState = LORA_STATE_IDLE;
    onTxDoneCallback = nullptr;
    onTxTimeoutCallback = nullptr;
    onRxDoneCallback = nullptr;
    onRxTimeoutCallback = nullptr;
    onRxErrorCallback = nullptr;

    // Initialize statistics
    txCount = 0;
    rxCount = 0;
    lastRssi = 0;
    lastSnr = 0.0;

    // Initialize interrupt flags
    transmitFlag = false;
    receiveFlag = false;
    enableInterrupt = true;
}

LoRaRadio::~LoRaRadio()
{
    if (instance == this)
    {
        instance = nullptr;
    }
    
    delete bandManager;
}

void LoRaRadio::initializeHardware()
{
    // Initialize SPI
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

#ifdef ARDUINO_heltec_wifi_lora_32_V4
    // V4 specific: Configure Power Amplifier pins
    pinMode(LORA_PA_EN, OUTPUT);
    pinMode(LORA_PA_TX_EN, OUTPUT);

    // Start with PA disabled
    digitalWrite(LORA_PA_EN, LOW);
    digitalWrite(LORA_PA_TX_EN, LOW);
#endif
}

void LoRaRadio::configurePowerAmplifier(bool enable)
{
#ifdef ARDUINO_heltec_wifi_lora_32_V4
    // V4 has PA control
    if (enable)
    {
        digitalWrite(LORA_PA_EN, HIGH);
        digitalWrite(LORA_PA_TX_EN, HIGH);
    }
    else
    {
        digitalWrite(LORA_PA_TX_EN, LOW);
        digitalWrite(LORA_PA_EN, LOW);
    }
#endif
    // V3 doesn't have PA control - power is controlled directly by the radio
}

bool LoRaRadio::begin()
{
    return begin(config.frequency, config.txPower);
}

bool LoRaRadio::begin(float frequency)
{
    return begin(frequency, config.txPower);
}

bool LoRaRadio::begin(float frequency, int8_t txPower)
{
    // Validate parameters
    if (!isFrequencyValid(frequency))
    {
        Serial.printf("[LoRa] Error: Invalid frequency: %.1f MHz\n", frequency);
        return false;
    }

    if (!isTxPowerValid(txPower))
    {
        Serial.printf("[LoRa] Error: Invalid TX power: %d dBm\n", txPower);
        return false;
    }

    // Store configuration
    config.frequency = frequency;
    config.txPower = txPower;

    // Initialize hardware
    initializeHardware();

    // Initialize SX1262 radio
    int state = radio.begin(
        config.frequency,
        config.bandwidth,
        config.spreadingFactor,
        config.codingRate,
        config.syncWord,
        config.txPower,
        config.preambleLength);

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Radio initialization failed, code: %d\n", state);
        return false;
    }

    // Set CRC
    state = radio.setCRC(config.crcEnabled);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Failed to set CRC, code: %d\n", state);
        return false;
    }

    // Set interrupt action
    radio.setDio1Action(setFlag);

    currentState = LORA_STATE_IDLE;

    Serial.println("[LoRa] Radio initialized successfully");
    printConfiguration();

    return true;
}

int LoRaRadio::setFrequency(float frequency)
{
    if (!isFrequencyValid(frequency))
    {
        Serial.printf("[LoRa] Invalid frequency: %.1f MHz\n", frequency);
        return RADIOLIB_ERR_INVALID_FREQUENCY;
    }

    config.frequency = frequency;
    int state = radio.setFrequency(frequency);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Frequency set to %.1f MHz\n", frequency);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set frequency, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setTxPower(int8_t power)
{
    if (!isTxPowerValid(power))
    {
        Serial.printf("[LoRa] Invalid TX power: %d dBm (max: %d dBm)\n", power, LORA_MAX_TX_POWER);
        return RADIOLIB_ERR_INVALID_OUTPUT_POWER;
    }

    config.txPower = power;
    int state = radio.setOutputPower(power);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] TX power set to %d dBm\n", power);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set TX power, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setBandwidth(float bandwidth)
{
    config.bandwidth = bandwidth;
    int state = radio.setBandwidth(bandwidth);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Bandwidth set to %.1f kHz\n", bandwidth);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set bandwidth, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setSpreadingFactor(uint8_t spreadingFactor)
{
    if (spreadingFactor < 7 || spreadingFactor > 12)
    {
        Serial.printf("[LoRa] Invalid spreading factor: %d (valid: 7-12)\n", spreadingFactor);
        return RADIOLIB_ERR_INVALID_SPREADING_FACTOR;
    }

    config.spreadingFactor = spreadingFactor;
    int state = radio.setSpreadingFactor(spreadingFactor);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Spreading factor set to SF%d\n", spreadingFactor);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set spreading factor, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setCodingRate(uint8_t codingRate)
{
    if (codingRate < 5 || codingRate > 8)
    {
        Serial.printf("[LoRa] Invalid coding rate: %d (valid: 5-8 for 4/5-4/8)\n", codingRate);
        return RADIOLIB_ERR_INVALID_CODING_RATE;
    }

    config.codingRate = codingRate;
    int state = radio.setCodingRate(codingRate);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Coding rate set to 4/%d\n", codingRate);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set coding rate, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setPreambleLength(uint16_t length)
{
    config.preambleLength = length;
    int state = radio.setPreambleLength(length);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Preamble length set to %d\n", length);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set preamble length, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setSyncWord(uint8_t syncWord)
{
    config.syncWord = syncWord;
    int state = radio.setSyncWord(syncWord);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Sync word set to 0x%02X\n", syncWord);
    }
    else
    {
        Serial.printf("[LoRa] Failed to set sync word, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::setCRC(bool enable)
{
    config.crcEnabled = enable;
    int state = radio.setCRC(enable);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] CRC %s\n", enable ? "enabled" : "disabled");
    }
    else
    {
        Serial.printf("[LoRa] Failed to set CRC, code: %d\n", state);
    }
    return state;
}

int LoRaRadio::send(const uint8_t *payload, uint8_t length)
{
    if (currentState == LORA_STATE_TX)
    {
        Serial.println("[LoRa] Warning: Already transmitting");
        return RADIOLIB_ERR_TX_TIMEOUT;
    }

    if (length > LORA_BUFFER_SIZE)
    {
        Serial.printf("[LoRa] Error: Payload too large (%d > %d)\n", length, LORA_BUFFER_SIZE);
        return RADIOLIB_ERR_PACKET_TOO_LONG;
    }

    // Enable PA for transmission (V4 only)
    configurePowerAmplifier(true);

    currentState = LORA_STATE_TX;
    transmitFlag = false;
    enableInterrupt = true;

    int state = radio.startTransmit(const_cast<uint8_t *>(payload), length);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Failed to start transmission, code: %d\n", state);
        currentState = LORA_STATE_IDLE;
        configurePowerAmplifier(false);
    }

    return state;
}

int LoRaRadio::send(const String &message)
{
    return send((const uint8_t *)message.c_str(), message.length());
}

int LoRaRadio::startReceive()
{
    return startReceive(0); // 0 = continuous receive mode
}

int LoRaRadio::startReceive(uint32_t timeout)
{
    if (currentState == LORA_STATE_RX)
    {
        Serial.println("[LoRa] Warning: Already receiving");
        return RADIOLIB_ERR_NONE;
    }

    // Disable PA for reception (V4 only)
    configurePowerAmplifier(false);

    currentState = LORA_STATE_RX;
    receiveFlag = false;
    enableInterrupt = true;

    int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] Failed to start reception, code: %d\n", state);
        currentState = LORA_STATE_IDLE;
    }

    return state;
}

int LoRaRadio::sleep()
{
    configurePowerAmplifier(false);
    currentState = LORA_STATE_SLEEP;
    return radio.sleep();
}

int LoRaRadio::standby()
{
    configurePowerAmplifier(false);
    currentState = LORA_STATE_IDLE;
    return radio.standby();
}

void LoRaRadio::onTxDone(void (*callback)(void))
{
    onTxDoneCallback = callback;
}

void LoRaRadio::onTxTimeout(void (*callback)(void))
{
    onTxTimeoutCallback = callback;
}

void LoRaRadio::onRxDone(void (*callback)(uint8_t *payload, uint16_t size, int16_t rssi, float snr))
{
    onRxDoneCallback = callback;
}

void LoRaRadio::onRxTimeout(void (*callback)(void))
{
    onRxTimeoutCallback = callback;
}

void LoRaRadio::onRxError(void (*callback)(void))
{
    onRxErrorCallback = callback;
}

bool LoRaRadio::isFrequencyValid(float frequency)
{
    // Use band manager for validation if available
    if (bandManager) {
        return bandManager->isFrequencyValid(frequency);
    }
    
    // Fallback to hardware limits and basic ISM bands
    return (frequency >= HARDWARE_MIN_FREQ && frequency <= HARDWARE_MAX_FREQ);
}

bool LoRaRadio::isTxPowerValid(int8_t power)
{
    return (power >= -9 && power <= LORA_MAX_TX_POWER);
}

void LoRaRadio::printConfiguration()
{
    Serial.println("[LoRa] Configuration:");
    Serial.printf("  Hardware: ");
#ifdef ARDUINO_heltec_wifi_lora_32_V3
    Serial.println("Heltec WiFi LoRa 32 V3");
#elif defined(ARDUINO_heltec_wifi_lora_32_V4)
    Serial.println("Heltec WiFi LoRa 32 V4");
#else
    Serial.println("Unknown");
#endif

    Serial.printf("  Frequency: %.1f MHz\n", config.frequency);
    Serial.printf("  TX Power: %d dBm (max: %d dBm)\n", config.txPower, LORA_MAX_TX_POWER);
    Serial.printf("  Bandwidth: %.1f kHz\n", config.bandwidth);
    Serial.printf("  Spreading Factor: SF%d\n", config.spreadingFactor);
    Serial.printf("  Coding Rate: 4/%d\n", config.codingRate);
    Serial.printf("  Preamble Length: %d\n", config.preambleLength);
    Serial.printf("  Sync Word: 0x%02X\n", config.syncWord);
    Serial.printf("  CRC: %s\n", config.crcEnabled ? "Enabled" : "Disabled");
}

void LoRaRadio::printStatistics()
{
    Serial.println("[LoRa] Statistics:");
    Serial.printf("  TX Count: %u\n", txCount);
    Serial.printf("  RX Count: %u\n", rxCount);
    Serial.printf("  Last RSSI: %d dBm\n", lastRssi);
    Serial.printf("  Last SNR: %.1f dB\n", lastSnr);

    const char *stateNames[] = {"IDLE", "TX", "RX", "SLEEP"};
    Serial.printf("  Current State: %s\n", stateNames[currentState]);
}

void LoRaRadio::handle()
{
    // Check if the interrupt flag was set
    if (!enableInterrupt)
    {
        return;
    }

    // Check if transmit completed
    if (transmitFlag)
    {
        transmitFlag = false;

        currentState = LORA_STATE_IDLE;
        txCount++;

        // Disable PA after transmission
        configurePowerAmplifier(false);

        if (onTxDoneCallback)
        {
            onTxDoneCallback();
        }

        // Automatically start receiving again
        startReceive();
        return;
    }

    // Check if receive completed
    if (receiveFlag)
    {
        receiveFlag = false;

        currentState = LORA_STATE_IDLE;
        rxCount++;

        // Read received data
        uint8_t buffer[LORA_BUFFER_SIZE];
        int state = radio.readData(buffer, 0);

        if (state == RADIOLIB_ERR_NONE)
        {
            // Get packet info
            lastRssi = radio.getRSSI();
            lastSnr = radio.getSNR();

            if (onRxDoneCallback)
            {
                onRxDoneCallback(buffer, radio.getPacketLength(), lastRssi, lastSnr);
            }
        }
        else if (onRxErrorCallback)
        {
            onRxErrorCallback();
        }

        // Start receiving again
        startReceive();
        return;
    }
}

bool LoRaRadio::selectBand(const String& bandId)
{
    if (bandManager && bandManager->selectBand(bandId)) {
        config.frequency = bandManager->getCurrentFrequency();
        
        // Apply the new frequency to the radio hardware
        int state = radio.setFrequency(config.frequency);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("[LoRa] Selected band: %s @ %.3f MHz\n", 
                         bandId.c_str(), config.frequency);
            return true;
        } else {
            Serial.printf("[LoRa] Failed to apply frequency for band %s, error: %d\n", 
                         bandId.c_str(), state);
            return false;
        }
    }
    
    Serial.printf("[LoRa] Failed to select band: %s\n", bandId.c_str());
    return false;
}

bool LoRaRadio::setFrequencyWithBand(float frequency)
{
    if (bandManager && bandManager->setFrequency(frequency)) {
        config.frequency = frequency;
        
        // Apply the new frequency to the radio hardware
        int state = radio.setFrequency(frequency);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("[LoRa] Frequency set to %.3f MHz\n", frequency);
            return true;
        } else {
            Serial.printf("[LoRa] Failed to apply frequency %.3f MHz, error: %d\n", 
                         frequency, state);
            return false;
        }
    }
    
    Serial.printf("[LoRa] Frequency %.3f MHz not allowed by current band configuration\n", frequency);
    return false;
}

void LoRaRadio::printAvailableBands()
{
    if (bandManager) {
        bandManager->printAvailableBands();
    } else {
        Serial.println("[LoRa] Band manager not initialized");
    }
}

void LoRaRadio::printCurrentBand()
{
    if (bandManager) {
        bandManager->printCurrentConfiguration();
    } else {
        Serial.println("[LoRa] Band manager not initialized");
    }
}