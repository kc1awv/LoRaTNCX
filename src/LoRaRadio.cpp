/**
 * @file LoRaRadio.cpp
 * @brief LoRa radio interface implementation using proven communication methods
 * @author LoRaTNCX Project
 * @date October 28, 2025
 */

#include "LoRaRadio.h"

LoRaRadio::LoRaRadio()
{
    radio = nullptr;
    initialized = false;
    txCount = 0;
    rxCount = 0;
    lastRSSI = 0;
    lastSNR = 0;
}

bool LoRaRadio::begin()
{
    Serial.println("Initializing LoRa radio...");

    // Initialize SPI
    initializeSPI();

    // Initialize PA control using proven methods
    initializePAControl();

    // Initialize radio instance - using same pins as working ping/pong
    radio = new SX1262(new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN));

    // Configure radio with proven settings
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
        Serial.println("✓ LoRa radio initialized successfully!");

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
        Serial.printf("  Frequency: %.1f MHz\n", LORA_FREQUENCY);
        Serial.printf("  Bandwidth: %.1f kHz\n", LORA_BANDWIDTH);
        Serial.printf("  Spreading Factor: %d\n", LORA_SPREADING_FACTOR);
        Serial.printf("  Coding Rate: 4/%d\n", LORA_CODING_RATE);
        Serial.printf("  Output Power: %d dBm\n", LORA_OUTPUT_POWER);
        Serial.printf("  Sync Word: 0x%02X\n", LORA_SYNC_WORD);
    }
    else
    {
        Serial.print("✗ LoRa radio initialization failed! Error: ");
        Serial.println(state);
        return false;
    }

    return initialized;
}

bool LoRaRadio::begin(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate)
{
    Serial.println("Initializing LoRa radio with custom configuration...");

    // Initialize SPI
    initializeSPI();

    // Initialize PA control using proven methods
    initializePAControl();

    // Initialize radio instance - using same pins as working ping/pong
    radio = new SX1262(new Module(LORA_SS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_BUSY_PIN));

    // Configure radio with custom settings
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
        Serial.println("✓ LoRa radio initialized successfully!");

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
        Serial.printf("  Frequency: %.1f MHz\n", frequency);
        Serial.printf("  Bandwidth: %.1f kHz\n", bandwidth);
        Serial.printf("  Spreading Factor: %d\n", spreadingFactor);
        Serial.printf("  Coding Rate: 4/%d\n", codingRate);
        Serial.printf("  Output Power: %d dBm\n", LORA_OUTPUT_POWER);
        Serial.printf("  Sync Word: 0x%02X\n", LORA_SYNC_WORD);
    }
    else
    {
        Serial.print("✗ LoRa radio initialization failed! Error: ");
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
    Serial.println("Configuring PA control pins (using proven method)...");

    // PA Power Control (LORA_PA_POWER = 7) - ANALOG mode (factory firmware insight)
    pinMode(LORA_PA_POWER_PIN, ANALOG);
    Serial.printf("  PA_POWER (pin %d): ANALOG mode (factory style)\n", LORA_PA_POWER_PIN);

    // PA Enable (LORA_PA_EN = 2) - Keep enabled
    pinMode(LORA_PA_EN_PIN, OUTPUT);
    digitalWrite(LORA_PA_EN_PIN, HIGH);
    Serial.printf("  PA_EN (pin %d): HIGH\n", LORA_PA_EN_PIN);

    // PA TX Enable (LORA_PA_TX_EN = 46) - Start in receive mode
    pinMode(LORA_PA_TX_EN_PIN, OUTPUT);
    digitalWrite(LORA_PA_TX_EN_PIN, LOW); // LOW for receive mode
    Serial.printf("  PA_TX_EN (pin %d): LOW (RX mode)\n", LORA_PA_TX_EN_PIN);

    Serial.println("PA configured: Power/Enable HIGH, TX_EN LOW for receive mode");

    // Power stabilization delay
    delay(200);
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
    Serial.println("Initializing SPI...");
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_SS_PIN);
    Serial.println("✓ SPI initialized");
}