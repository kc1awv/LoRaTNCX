#include "Radio.h"
#include "Config.h"

RadioHAL *RadioHAL::instance = nullptr;

bool RadioHAL::begin(RxCB cb)
{
    onRx = cb;
    instance = this;

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Initializing LoRa radio...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Pin configuration: NSS=%d, SCLK=%d, MOSI=%d, MISO=%d, RST=%d, BUSY=%d, DIO1=%d\n",
                  LORA_NSS, LORA_SCLK, LORA_MOSI, LORA_MISO, LORA_RST, LORA_BUSY, LORA_DIO1);
    #endif

    testSpiCommunication();

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting SPI interface...");
    #endif
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI);

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Creating Module and SX1262 objects...");
    #endif
    mod = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0));
    lora = new SX1262(mod);

    if (!mod || !lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] Failed to create radio objects!");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Radio objects created successfully");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Initializing V4 Power Amplifier control pins...");
    #endif
    pinMode(LORA_PA_POWER, ANALOG);
    pinMode(LORA_PA_EN, OUTPUT);
    pinMode(LORA_PA_TX_EN, OUTPUT);

    digitalWrite(LORA_PA_EN, HIGH);
    digitalWrite(LORA_PA_TX_EN, HIGH);

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] PA Control - PA_EN (pin %d) = HIGH, PA_TX_EN (pin %d) = HIGH\n",
                  LORA_PA_EN, LORA_PA_TX_EN);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] [OK] Power Amplifier enabled - RF output should now work!");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Applying initial configuration...");
    #endif
    if (!applyRadioConfig())
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] Failed to apply initial configuration!");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Setting up CRC and interrupt handler...");
    #endif
    int crcResult = lora->setCRC(true);
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] CRC enable result: %d\n", crcResult);
    #endif
    lora->setDio1Action(irqHandler);

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting receive mode...");
    #endif
    int rxResult = lora->startReceive();
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Start receive result: %d\n", rxResult);
    #endif
    if (rxResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [OK] Radio initialization completed successfully!");
        #endif
        return true;
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] [FAIL] Failed to start receive mode (error: %d)\n", rxResult);
        #endif
        return false;
    }
}

void RadioHAL::testSpiCommunication()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] [TEST] Testing SPI communication...");
    #endif
    pinMode(LORA_NSS, OUTPUT);
    pinMode(LORA_SCLK, OUTPUT);
    pinMode(LORA_MOSI, OUTPUT);
    pinMode(LORA_MISO, INPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(LORA_BUSY, INPUT);
    pinMode(LORA_DIO1, INPUT);

    digitalWrite(LORA_NSS, HIGH);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Pin states - NSS: %s, RST: %s, BUSY: %s, DIO1: %s\n",
                  digitalRead(LORA_NSS) ? "HIGH" : "LOW",
                  digitalRead(LORA_RST) ? "HIGH" : "LOW",
                  digitalRead(LORA_BUSY) ? "HIGH" : "LOW",
                  digitalRead(LORA_DIO1) ? "HIGH" : "LOW");
    #endif

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] SPI pin test completed");
    #endif
}

bool RadioHAL::send(const uint8_t *buf, size_t len)
{
    if (!lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] ERROR: LoRa object is NULL!");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Starting transmission of %d bytes\n", len);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Current config: %.3f MHz, %d dBm, SF%d, BW%.1f kHz, CR4/%d\n",
                  frequency, txPower, spreadingFactor, bandwidth, codingRate);
    #endif

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Checking radio state before TX...\n");
    #endif
    if (persist < 255)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] Performing CSMA (persistence: %d/255 = %d%%)\n",
                      persist, (persist * 100) / 255);
        #endif
        performCSMA();
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] CSMA disabled (persistence = 255)");
        #endif
    }

    if (txDelay > 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] Applying TX delay: %d ms\n", txDelay * 10);
        #endif
        delay(txDelay * 10);
    }

    digitalWrite(LORA_PA_EN, HIGH);
    digitalWrite(LORA_PA_TX_EN, HIGH);
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] V4 PA enabled for transmission");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Calling lora->transmit()...");
    #endif
    unsigned long txStart = millis();
    int st = lora->transmit((uint8_t *)buf, len);
    unsigned long txTime = millis() - txStart;

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Transmit completed in %lu ms, result: %d\n", txTime, st);
    #endif
    switch (st)
    {
    case RADIOLIB_ERR_NONE:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [OK] RADIOLIB_ERR_NONE: Transmission successful!");
        #endif
        break;
    case RADIOLIB_ERR_UNKNOWN:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_UNKNOWN");
        #endif
        break;
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_CHIP_NOT_FOUND: Radio chip not responding!");
        #endif
        break;
    case RADIOLIB_ERR_PACKET_TOO_LONG:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_PACKET_TOO_LONG");
        #endif
        break;
    case RADIOLIB_ERR_TX_TIMEOUT:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_TX_TIMEOUT");
        #endif
        break;
    case RADIOLIB_ERR_RX_TIMEOUT:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_RX_TIMEOUT");
        #endif
        break;
    case RADIOLIB_ERR_CRC_MISMATCH:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_CRC_MISMATCH");
        #endif
        break;
    case RADIOLIB_ERR_INVALID_BANDWIDTH:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_BANDWIDTH");
        #endif
        break;
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_SPREADING_FACTOR");
        #endif
        break;
    case RADIOLIB_ERR_INVALID_CODING_RATE:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_CODING_RATE");
        #endif
        break;
    case RADIOLIB_ERR_INVALID_FREQUENCY:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_FREQUENCY");
        #endif
        break;
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_OUTPUT_POWER");
        #endif
        break;
    default:
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] [FAIL] Unknown error code: %d\n", st);
        #endif
        break;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Restarting receive mode...");
    #endif
    int rxResult = lora->startReceive();
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Start receive result: %d\n", rxResult);
    #endif
    bool success = (st == RADIOLIB_ERR_NONE);
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Final result: %s\n", success ? "SUCCESS" : "FAILED");
    #endif
    return success;
}

void RadioHAL::irqHandler()
{
    if (!instance || !instance->lora || !instance->onRx)
        return;

    size_t len = instance->lora->getPacketLength();
    if (len == 0 || len > 512)
        return;

    static uint8_t buf[512];
    int st = instance->lora->readData(buf, len);
    if (st == RADIOLIB_ERR_NONE)
    {
        instance->lastRxTime = millis();
        instance->channelBusy = true;

        instance->onRx(buf, len, instance->lora->getRSSI(), instance->lora->getSNR());
    }
}

void RadioHAL::poll()
{
    if (channelBusy && (millis() - lastRxTime) > RX_TIMEOUT_MS)
    {
        channelBusy = false;
    }
}

bool RadioHAL::setTxDelay(uint8_t delay_10ms)
{
    txDelay = delay_10ms;
    return true;
}

bool RadioHAL::setPersist(uint8_t persist_val)
{
    persist = persist_val;
    return true;
}

bool RadioHAL::setSlotTime(uint8_t slot_10ms)
{
    slotTime = slot_10ms;
    return true;
}

bool RadioHAL::setFrequency(float freq_mhz)
{
    if (!lora || freq_mhz < 150.0 || freq_mhz > 960.0)
        return false;

    frequency = freq_mhz;
    int state = lora->setFrequency(frequency);
    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::setTxPower(int8_t power_dbm)
{
    if (!lora || power_dbm < -9 || power_dbm > 22)
        return false;

    txPower = power_dbm;
    int state = lora->setOutputPower(txPower);
    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::setBandwidth(float bw_khz)
{
    if (!lora)
        return false;

    bandwidth = bw_khz;
    int state = lora->setBandwidth(bandwidth);
    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::setSpreadingFactor(uint8_t sf)
{
    if (!lora || sf < 7 || sf > 12)
        return false;

    spreadingFactor = sf;
    int state = lora->setSpreadingFactor(spreadingFactor);
    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::setCodingRate(uint8_t cr)
{
    if (!lora || cr < 5 || cr > 8)
        return false;

    codingRate = cr;
    int state = lora->setCodingRate(codingRate);
    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::applyRadioConfig()
{
    if (!lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] ERROR: LoRa object is NULL in applyRadioConfig()");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Applying configuration to radio...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Parameters: freq=%.3f MHz, bw=%.1f kHz, sf=%d, cr=4/%d, sync=0x%02X, power=%d dBm, preamble=%d\n",
                  frequency, bandwidth, spreadingFactor, codingRate, syncWord, txPower, preambleLength);
    #endif

    int state = lora->begin(frequency, bandwidth, spreadingFactor,
                            codingRate, syncWord, txPower, preambleLength);

    if (state == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] [OK] Radio configuration applied successfully");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Verifying applied configuration...");
        #endif
        testRadioHealth();
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] [FAIL] Radio configuration failed with error: %d\n", state);
        #endif
        switch (state)
        {
        case RADIOLIB_ERR_INVALID_FREQUENCY:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Invalid frequency");
            #endif
            break;
        case RADIOLIB_ERR_INVALID_BANDWIDTH:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Invalid bandwidth");
            #endif
            break;
        case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Invalid spreading factor");
            #endif
            break;
        case RADIOLIB_ERR_INVALID_CODING_RATE:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Invalid coding rate");
            #endif
            break;
        case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Invalid output power");
            #endif
            break;
        case RADIOLIB_ERR_CHIP_NOT_FOUND:
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] ERROR: Radio chip not found - check SPI connections!");
            #endif
            break;
        default:
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[RADIO] ERROR: Unknown configuration error: %d\n", state);
            #endif
            break;
        }
    }

    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::testRadioHealth()
{
    if (!lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Health check FAILED: LoRa object is NULL");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting radio health check...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.print("[RADIO] Test 1 - Standby mode test: ");
    #endif
    int standbyResult = lora->standby();
    if (standbyResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[PASS] - Radio responds to commands");
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[FAIL] (error: %d) - Radio not responding to commands\n", standbyResult);
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.print("[RADIO] Test 2 - Frequency setting test: ");
    #endif
    int freqResult = lora->setFrequency(frequency);
    if (freqResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[PASS] (%.3f MHz)\n", frequency);
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[FAIL] (error: %d)\n", freqResult);
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.print("[RADIO] Test 3 - Power setting test: ");
    #endif
    int powerResult = lora->setOutputPower(txPower);
    if (powerResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[PASS] (%d dBm)\n", txPower);
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[FAIL] (error: %d)\n", powerResult);
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.print("[RADIO] Test 4 - Small packet transmission test: ");
    #endif
    uint8_t testData[] = {0xAA, 0x55, 0xAA, 0x55};
    unsigned long txStart = millis();
    int testResult = lora->transmit(testData, 4);
    unsigned long txTime = millis() - txStart;

    if (testResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[PASS] (completed in %lu ms)\n", txTime);
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[FAIL] (error: %d, took %lu ms)\n", testResult, txTime);
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] TX test failed - possible causes:");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] - SPI communication issue");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] - Radio module power issue");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] - Invalid configuration");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] - Hardware fault");
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.print("[RADIO] Test 5 - Return to receive mode: ");
    #endif
    int rxResult = lora->startReceive();
    if (rxResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[PASS]");
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[FAIL] (error: %d)\n", rxResult);
        #endif
        return false;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Radio health check PASSED - hardware is responding correctly");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] If you're not seeing RF output, check:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] - Antenna connection");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] - SDR frequency and settings");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] - RF shielding/interference");
    #endif
    return true;
}

bool RadioHAL::checkChannelClear()
{
    return !channelBusy;
}

void RadioHAL::performCSMA()
{
    while (true)
    {
        if (checkChannelClear())
        {
            uint8_t random_val = random(256);
            if (random_val <= persist)
            {
                break;
            }
            else
            {
                delay(slotTime * 10);
            }
        }
        else
        {
            delay(slotTime * 10);
        }
    }
}

void RadioHAL::loadConfig()
{
    RadioConfig &cfg = config.getRadioConfig();

    frequency = cfg.frequency;
    bandwidth = cfg.bandwidth;
    spreadingFactor = cfg.spreadingFactor;
    codingRate = cfg.codingRate;
    txPower = cfg.txPower;
    txDelay = cfg.txDelay;
    persist = cfg.persist;
    slotTime = cfg.slotTime;

    if (lora)
    {
        applyRadioConfig();
    }
}

void RadioHAL::runTransmissionTest()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting comprehensive transmission test...");
    #endif
    if (!lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Test FAILED: LoRa object is NULL");
        #endif
        return;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Test 1: Current radio status");
    #endif
    testRadioHealth();

    #ifndef KISS_SERIAL_MODE
    Serial.println("\n[RADIO] Test 2: Test packets of various sizes");
    #endif
    uint8_t testPackets[][32] = {
        {0xAA},
        {0xAA, 0x55},
        {0xAA, 0x55, 0xAA, 0x55},
        {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE},
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}};

    size_t testSizes[] = {1, 2, 4, 8, 16};

    for (int i = 0; i < 5; i++)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("\n[RADIO] Testing %d byte packet...\n", testSizes[i]);
        #endif
        uint8_t originalPersist = persist;
        uint8_t originalTxDelay = txDelay;
        persist = 255;
        txDelay = 0;

        unsigned long start = millis();
        int result = lora->transmit(testPackets[i], testSizes[i]);
        unsigned long duration = millis() - start;

        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] Result: %d, Duration: %lu ms\n", result, duration);
        #endif
        if (result == RADIOLIB_ERR_NONE)
        {
            #ifndef KISS_SERIAL_MODE
            Serial.println("[RADIO] SUCCESS");
            #endif
        }
        else
        {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[RADIO] [FAIL] TX FAILED (error %d)\n", result);
            #endif
        }

        persist = originalPersist;
        txDelay = originalTxDelay;

        delay(1000);
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("\n[RADIO] Test 3: Checking for any received packets");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Restarting receive mode...");
    #endif
    int rxResult = lora->startReceive();
    if (rxResult == RADIOLIB_ERR_NONE)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Receive mode started successfully");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Monitor your SDR for the test transmissions above");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] Test packets should appear as brief bursts on your configured frequency");
        #endif
    }
    else
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[RADIO] [FAIL] Failed to restart receive mode (error: %d)\n", rxResult);
        #endif
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("\n[RADIO] Transmission test completed");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Check SDR for actual RF output during the test");
    #endif
}

void RadioHAL::checkHardwarePins()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] [TEST] Checking hardware pin configuration...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Current pin definitions:\n");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   NSS (CS):   %d\n", LORA_NSS);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   SCLK:       %d\n", LORA_SCLK);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   MOSI:       %d\n", LORA_MOSI);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   MISO:       %d\n", LORA_MISO);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   RST:        %d\n", LORA_RST);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   BUSY:       %d\n", LORA_BUSY);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   DIO1:       %d\n", LORA_DIO1);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   PA_POWER:   %d\n", LORA_PA_POWER);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   PA_EN:      %d\n", LORA_PA_EN);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   PA_TX_EN:   %d\n", LORA_PA_TX_EN);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Heltec WiFi LoRa 32 V4 pin configuration");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Official Heltec V4 LoRa pins should be:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   NSS (CS):   8");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   SCK:        9");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   MOSI:      10");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   MISO:      11");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   RST:       12");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   BUSY:      13");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO]   DIO1:      14");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Current pin states:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   NSS:  %s\n", digitalRead(LORA_NSS) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   RST:  %s\n", digitalRead(LORA_RST) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   BUSY: %s\n", digitalRead(LORA_BUSY) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   DIO1: %s\n", digitalRead(LORA_DIO1) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   PA_EN: %s\n", digitalRead(LORA_PA_EN) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO]   PA_TX_EN: %s\n", digitalRead(LORA_PA_TX_EN) ? "HIGH" : "LOW");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Testing pin manipulation...");
    #endif
    digitalWrite(LORA_NSS, LOW);
    delay(10);
    bool nssLow = digitalRead(LORA_NSS) == LOW;
    digitalWrite(LORA_NSS, HIGH);
    delay(10);
    bool nssHigh = digitalRead(LORA_NSS) == HIGH;

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] NSS control test: %s\n", (nssLow && nssHigh) ? "[PASS]" : "[FAIL]");
    #endif
    digitalWrite(LORA_RST, LOW);
    delay(10);
    bool rstLow = digitalRead(LORA_RST) == LOW;
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    bool rstHigh = digitalRead(LORA_RST) == HIGH;

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] RST control test: %s\n", (rstLow && rstHigh) ? "[PASS]" : "[FAIL]");
    #endif
}

void RadioHAL::testContinuousTransmission()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting continuous transmission test...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] This will transmit continuously for 30 seconds");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Monitor your SDR during this time!");
    #endif
    if (!lora)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[RADIO] ERROR: LoRa not initialized");
        #endif
        return;
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Setting maximum power (22 dBm) for test...");
    #endif
    lora->setOutputPower(22);

    uint8_t testPattern[] = {
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Starting continuous transmission...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Frequency: %.3f MHz\n", frequency);
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Power: 22 dBm (max)\n");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Look for signals on your SDR NOW!\n");
    #endif
    unsigned long startTime = millis();
    int transmissionCount = 0;

    while (millis() - startTime < 30000)
    {
        int result = lora->transmit(testPattern, sizeof(testPattern));
        transmissionCount++;

        if (result == RADIOLIB_ERR_NONE)
        {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[RADIO] TX #%d: SUCCESS\n", transmissionCount);
            #endif
        }
        else
        {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[RADIO] TX #%d: FAILED (error %d)\n", transmissionCount, result);
            #endif
        }

        delay(500);

        if (Serial.available())
        {
            char c = Serial.read();
            if (c == 'q' || c == 'Q')
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("[RADIO] Test aborted by user");
                #endif
                break;
            }
        }
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Continuous transmission test completed");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Total transmissions: %d\n", transmissionCount);
    #endif
    lora->setOutputPower(txPower);
    lora->startReceive();

    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] If you saw NO activity on SDR during this test:");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] 1. Check antenna connection");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] 2. Verify SDR frequency setting");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] 3. Pin configuration may be wrong");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] 4. Hardware fault possible");
    #endif
}
