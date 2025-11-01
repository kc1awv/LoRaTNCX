/**
 * @file LoRaRadio.cpp
 * @brief LoRa radio interface implementation using proven communication methods
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "LoRaRadio.h"
#include "SystemLogger.h"
#include <math.h>

LoRaRadio::LoRaRadio()
{
    radio = nullptr;
    initialized = false;
    spiInitialized = false;
    paInitialized = false;
    txCount = 0;
    rxCount = 0;
    lastRSSI = 0;
    lastSNR = 0;

    // Initialize current parameters with defaults
    currentFrequency = LORA_FREQUENCY;
    currentTxPower = LORA_OUTPUT_POWER;
    currentSpreadingFactor = LORA_SPREADING_FACTOR;
    currentBandwidth = LORA_BANDWIDTH;
    currentCodingRate = LORA_CODING_RATE;
    currentSyncWord = LORA_SYNC_WORD;
}

bool LoRaRadio::begin()
{
    const bool reconfiguring = initialized;
    LOG_LORA_INFO(reconfiguring ? "Reconfiguring LoRa radio with default configuration..."
                                 : "Initializing LoRa radio...");

    // Initialize SPI and PA control only once
    if (!spiInitialized)
    {
        initializeSPI();
    }

    if (!paInitialized)
    {
        initializePAControl();
    }

    // Initialize radio instance - using same pins as working ping/pong
    if (radio == nullptr)
    {
        radio = new SX1262(new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN));
    }

    // Configure radio with proven settings
    initialized = false;
    int state = radio->begin(
        LORA_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        LORA_OUTPUT_POWER,
        LORA_PREAMBLE_LENGTH);

    if (state == RADIOLIB_ERR_NONE)
    {
        // Store current parameters
        currentFrequency = LORA_FREQUENCY;
        currentBandwidth = LORA_BANDWIDTH;
        currentSpreadingFactor = LORA_SPREADING_FACTOR;
        currentCodingRate = LORA_CODING_RATE;
        currentSyncWord = LORA_SYNC_WORD;
        currentTxPower = LORA_OUTPUT_POWER;

        Serial.println(reconfiguring ? "✓ LoRa radio reconfigured successfully!"
                                     : "✓ LoRa radio initialized successfully!");

        // Set PA to receive mode after initialization
        setPA(false);

        // Start in receive mode
        int rxState = radio->startReceive();
        if (rxState == RADIOLIB_ERR_NONE)
        {
            Serial.println("✓ Radio started in receive mode");
            initialized = true;
        }
        else
        {
            Serial.print("✗ Failed to start receive mode, error: ");
            Serial.println(rxState);
            return false;
        }

        // Print configuration
        Serial.println("LoRa Configuration:");
        Serial.printf("  Frequency: %.1f MHz\r\n", currentFrequency);
        Serial.printf("  Bandwidth: %.1f kHz\r\n", currentBandwidth);
        Serial.printf("  Spreading Factor: %d\r\n", currentSpreadingFactor);
        Serial.printf("  Coding Rate: 4/%d\r\n", currentCodingRate);
        Serial.printf("  Output Power: %d dBm\r\n", currentTxPower);
        Serial.printf("  Sync Word: 0x%02X\r\n", currentSyncWord);
    }
    else
    {
        Serial.print(reconfiguring ? "✗ LoRa radio reconfiguration failed! Error: "
                                   : "✗ LoRa radio initialization failed! Error: ");
        Serial.println(state);
        return false;
    }

    return initialized;
}

bool LoRaRadio::begin(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate)
{
    const bool reconfiguring = initialized;
    Serial.println(reconfiguring ? "Reconfiguring LoRa radio with custom configuration..."
                                 : "Initializing LoRa radio with custom configuration...");

    // Initialize SPI and PA control only once
    if (!spiInitialized)
    {
        initializeSPI();
    }

    if (!paInitialized)
    {
        initializePAControl();
    }

    // Initialize radio instance - using same pins as working ping/pong
    if (radio == nullptr)
    {
        radio = new SX1262(new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN));
    }

    // Configure radio with custom settings
    initialized = false;
    int state = radio->begin(
        frequency,
        bandwidth,
        spreadingFactor,
        codingRate,
        LORA_SYNC_WORD,
        LORA_OUTPUT_POWER,
        LORA_PREAMBLE_LENGTH);

    if (state == RADIOLIB_ERR_NONE)
    {
        // Store current parameters
        currentFrequency = frequency;
        currentBandwidth = bandwidth;
        currentSpreadingFactor = spreadingFactor;
        currentCodingRate = codingRate;
        currentSyncWord = LORA_SYNC_WORD;
        currentTxPower = LORA_OUTPUT_POWER;

        Serial.println(reconfiguring ? "✓ LoRa radio reconfigured successfully!"
                                     : "✓ LoRa radio initialized successfully!");

        // Set PA to receive mode after initialization
        setPA(false);

        // Start in receive mode
        int rxState = radio->startReceive();
        if (rxState == RADIOLIB_ERR_NONE)
        {
            Serial.println("✓ Radio started in receive mode");
            initialized = true;
        }
        else
        {
            Serial.print("✗ Failed to start receive mode, error: ");
            Serial.println(rxState);
            return false;
        }

        // Print configuration
        Serial.println("LoRa Configuration:");
        Serial.printf("  Frequency: %.1f MHz\r\n", currentFrequency);
        Serial.printf("  Bandwidth: %.1f kHz\r\n", currentBandwidth);
        Serial.printf("  Spreading Factor: %d\r\n", currentSpreadingFactor);
        Serial.printf("  Coding Rate: 4/%d\r\n", currentCodingRate);
        Serial.printf("  Output Power: %d dBm\r\n", currentTxPower);
        Serial.printf("  Sync Word: 0x%02X\r\n", currentSyncWord);
    }
    else
    {
        Serial.print(reconfiguring ? "✗ LoRa radio reconfiguration failed! Error: "
                                   : "✗ LoRa radio initialization failed! Error: ");
        Serial.println(state);
        return false;
    }

    return initialized;
}

bool LoRaRadio::transmit(const uint8_t *data, size_t length)
{
    if (!initialized)
    {
        return false;
    }

    // Set PA to transmit mode with proven timing
    setPA(true);

    // Transmit data
    int state = radio->transmit(const_cast<uint8_t *>(data), length);

    // Set PA back to receive mode with proven timing
    setPA(false);

    // Restart receive mode
    radio->startReceive();

    if (state == RADIOLIB_ERR_NONE)
    {
        txCount++;
        return true;
    }
    else
    {
        Serial.print("Transmission failed, error: ");
        Serial.println(state);
        return false;
    }
}

bool LoRaRadio::transmit(const String &message)
{
    if (!initialized)
    {
        return false;
    }

    // Set PA to transmit mode with proven timing
    setPA(true);

    // Transmit string - create a non-const copy for RadioLib
    String msgCopy = message;
    int state = radio->transmit(msgCopy);

    // Set PA back to receive mode with proven timing
    setPA(false);

    // Restart receive mode
    radio->startReceive();

    if (state == RADIOLIB_ERR_NONE)
    {
        txCount++;
        return true;
    }
    else
    {
        Serial.print("Transmission failed, error: ");
        Serial.println(state);
        return false;
    }
}

bool LoRaRadio::available()
{
    if (!initialized)
    {
        return false;
    }

    // Check if packet received
    return (radio->getIrqStatus() & RADIOLIB_SX126X_IRQ_RX_DONE);
}

size_t LoRaRadio::receive(uint8_t *buffer, size_t maxLength)
{
    if (!initialized || !available())
    {
        return 0;
    }

    // Use String-based readData (proven to work in ping/pong)
    String str;
    int state = radio->readData(str);

    size_t length = 0;

    if (state == RADIOLIB_ERR_NONE && str.length() > 0)
    {
        // Copy string data to buffer
        length = min((size_t)str.length(), maxLength);
        memcpy(buffer, str.c_str(), length);

        rxCount++;
        lastRSSI = radio->getRSSI();
        lastSNR = radio->getSNR();

        // Restart receive mode
        radio->startReceive();
    }
    else if (state != RADIOLIB_ERR_RX_TIMEOUT)
    {
        Serial.print("Receive error: ");
        Serial.println(state);

        // Restart receive mode on error
        radio->startReceive();
    }

    return length;
}

bool LoRaRadio::receive(String &message)
{
    if (!initialized || !available())
    {
        return false;
    }

    int state = radio->readData(message);

    if (state == RADIOLIB_ERR_NONE)
    {
        rxCount++;
        lastRSSI = radio->getRSSI();
        lastSNR = radio->getSNR();

        // Restart receive mode
        radio->startReceive();
        return true;
    }
    else if (state != RADIOLIB_ERR_RX_TIMEOUT)
    {
        Serial.print("Receive error: ");
        Serial.println(state);

        // Restart receive mode on error
        radio->startReceive();
    }

    return false;
}

float LoRaRadio::getRSSI()
{
    return lastRSSI;
}

float LoRaRadio::getSNR()
{
    return lastSNR;
}

String LoRaRadio::getStatus()
{
    String status = "LoRa Radio Status:\n";
    status += "  Initialized: " + String(initialized ? "Yes" : "No") + "\n";
    status += "  TX Count: " + String(txCount) + "\n";
    status += "  RX Count: " + String(rxCount) + "\n";
    status += "  Last RSSI: " + String(lastRSSI, 1) + " dBm\n";
    status += "  Last SNR: " + String(lastSNR, 1) + " dB";
    return status;
}

void LoRaRadio::initializePAControl()
{
    LOG_BOOT_INFO("Configuring PA control pins (using proven method)...");

    // PA Power Control (LORA_PA_POWER = 7) - ANALOG mode (factory firmware insight)
    pinMode(LORA_PA_POWER_PIN, ANALOG);
    LOG_DEBUG("  PA_POWER (pin " + String(LORA_PA_POWER_PIN) + "): ANALOG mode (factory style)");

    // PA Enable (LORA_PA_EN = 2) - Keep enabled
    pinMode(LORA_PA_EN_PIN, OUTPUT);
    digitalWrite(LORA_PA_EN_PIN, HIGH);
    LOG_DEBUG("  PA_EN (pin " + String(LORA_PA_EN_PIN) + "): HIGH");

    // PA TX Enable (LORA_PA_TX_EN = 46) - Start in receive mode
    pinMode(LORA_PA_TX_EN_PIN, OUTPUT);
    digitalWrite(LORA_PA_TX_EN_PIN, LOW); // LOW for receive mode
    LOG_DEBUG("  PA_TX_EN (pin " + String(LORA_PA_TX_EN_PIN) + "): LOW (RX mode)");

    LOG_BOOT_SUCCESS("PA configured: Power/Enable HIGH, TX_EN LOW for receive mode");

    // Power stabilization delay
    delay(200);
    paInitialized = true;
}

void LoRaRadio::setPA(bool transmit)
{
    if (transmit)
    {
        // Enable PA for transmission using proven method
        digitalWrite(LORA_PA_EN_PIN, HIGH);    // Keep PA enabled
        digitalWrite(LORA_PA_TX_EN_PIN, HIGH); // Enable TX mode
        analogWrite(LORA_PA_POWER_PIN, 255);   // Full power in ANALOG mode
        delay(20);                             // Critical 20ms settling delay (proven in ping/pong)
    }
    else
    {
        // Switch PA to receive mode using proven method
        digitalWrite(LORA_PA_EN_PIN, HIGH);   // Keep PA enabled
        digitalWrite(LORA_PA_TX_EN_PIN, LOW); // DISABLE TX mode for RX
        analogWrite(LORA_PA_POWER_PIN, 0);    // Reduce power for RX
        delay(20);                            // Critical 20ms settling delay (proven in ping/pong)
    }
}

void LoRaRadio::initializeSPI()
{
    LOG_BOOT_INFO("Initializing SPI...");
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
    LOG_BOOT_SUCCESS("SPI initialized");
    spiInitialized = true;
}

bool LoRaRadio::applyConfiguration(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate, int8_t txPower, uint8_t syncWord)
{
    bool modemOk = applyModemConfiguration(frequency, bandwidth, spreadingFactor, codingRate);

    bool txPowerOk = setTxPower(txPower);
    bool syncWordOk = setSyncWord(syncWord);

    return modemOk && txPowerOk && syncWordOk;
}

bool LoRaRadio::applyModemConfiguration(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate)
{
    if (initialized)
    {
        const float freqDiff = fabsf(currentFrequency - frequency);
        const float bwDiff = fabsf(currentBandwidth - bandwidth);
        if (freqDiff < 0.001f && bwDiff < 0.001f &&
            currentSpreadingFactor == spreadingFactor &&
            currentCodingRate == codingRate)
        {
            // Configuration already matches requested values
            return true;
        }
    }

    return begin(frequency, bandwidth, spreadingFactor, codingRate);
}

// Parameter setter methods - these may reinitialize the radio with new parameters
bool LoRaRadio::setFrequency(float frequency)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    return applyModemConfiguration(frequency, currentBandwidth, currentSpreadingFactor, currentCodingRate);
}

bool LoRaRadio::setTxPower(int8_t power)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    // Set power directly on radio (doesn't require full reinit)
    int state = radio->setOutputPower(power);
    if (state == RADIOLIB_ERR_NONE)
    {
        currentTxPower = power;
        return true;
    }
    return false;
}

bool LoRaRadio::setSpreadingFactor(uint8_t sf)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    return applyModemConfiguration(currentFrequency, currentBandwidth, sf, currentCodingRate);
}

bool LoRaRadio::setBandwidth(float bw)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    return applyModemConfiguration(currentFrequency, bw, currentSpreadingFactor, currentCodingRate);
}

bool LoRaRadio::setCodingRate(uint8_t cr)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    return applyModemConfiguration(currentFrequency, currentBandwidth, currentSpreadingFactor, cr);
}

bool LoRaRadio::setSyncWord(uint8_t syncWord)
{
    if (!initialized || radio == nullptr)
    {
        return false;
    }

    // Set sync word directly on radio (doesn't require full reinit)
    int state = radio->setSyncWord(syncWord);
    if (state == RADIOLIB_ERR_NONE)
    {
        currentSyncWord = syncWord;
        return true;
    }
    return false;
}

// Parameter getter methods
float LoRaRadio::getFrequency() const
{
    return currentFrequency;
}

int8_t LoRaRadio::getTxPower() const
{
    return currentTxPower;
}

uint8_t LoRaRadio::getSpreadingFactor() const
{
    return currentSpreadingFactor;
}

float LoRaRadio::getBandwidth() const
{
    return currentBandwidth;
}

uint8_t LoRaRadio::getCodingRate() const
{
    return currentCodingRate;
}

uint8_t LoRaRadio::getSyncWord() const
{
    return currentSyncWord;
}