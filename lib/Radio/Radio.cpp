#include "Radio.h"
#include "Config.h"

RadioHAL *RadioHAL::instance = nullptr;

bool RadioHAL::begin(RxCB cb)
{
    onRx = cb;
    instance = this;

    Serial.println("[RADIO] Initializing LoRa radio...");
    Serial.printf("[RADIO] Pin configuration: NSS=%d, SCLK=%d, MOSI=%d, MISO=%d, RST=%d, BUSY=%d, DIO1=%d\n",
                  LORA_NSS, LORA_SCLK, LORA_MOSI, LORA_MISO, LORA_RST, LORA_BUSY, LORA_DIO1);

    testSpiCommunication();

    Serial.println("[RADIO] Starting SPI interface...");
    SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI);

    Serial.println("[RADIO] Creating Module and SX1262 objects...");
    mod = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0));
    lora = new SX1262(mod);

    if (!mod || !lora)
    {
        Serial.println("[RADIO] [FAIL] Failed to create radio objects!");
        return false;
    }

    Serial.println("[RADIO] Radio objects created successfully");

    Serial.println("[RADIO] Initializing V4 Power Amplifier control pins...");
    pinMode(LORA_PA_POWER, ANALOG);
    pinMode(LORA_PA_EN, OUTPUT);
    pinMode(LORA_PA_TX_EN, OUTPUT);

    digitalWrite(LORA_PA_EN, HIGH);
    digitalWrite(LORA_PA_TX_EN, HIGH);

    Serial.printf("[RADIO] PA Control - PA_EN (pin %d) = HIGH, PA_TX_EN (pin %d) = HIGH\n",
                  LORA_PA_EN, LORA_PA_TX_EN);
    Serial.println("[RADIO] [OK] Power Amplifier enabled - RF output should now work!");

    Serial.println("[RADIO] Applying initial configuration...");
    if (!applyRadioConfig())
    {
        Serial.println("[RADIO] [FAIL] Failed to apply initial configuration!");
        return false;
    }

    Serial.println("[RADIO] Setting up CRC and interrupt handler...");
    int crcResult = lora->setCRC(true);
    Serial.printf("[RADIO] CRC enable result: %d\n", crcResult);

    lora->setDio1Action(irqHandler);

    Serial.println("[RADIO] Starting receive mode...");
    int rxResult = lora->startReceive();
    Serial.printf("[RADIO] Start receive result: %d\n", rxResult);

    if (rxResult == RADIOLIB_ERR_NONE)
    {
        Serial.println("[RADIO] [OK] Radio initialization completed successfully!");
        return true;
    }
    else
    {
        Serial.printf("[RADIO] [FAIL] Failed to start receive mode (error: %d)\n", rxResult);
        return false;
    }
}

void RadioHAL::testSpiCommunication()
{
    Serial.println("[RADIO] [TEST] Testing SPI communication...");

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

    Serial.printf("[RADIO] Pin states - NSS: %s, RST: %s, BUSY: %s, DIO1: %s\n",
                  digitalRead(LORA_NSS) ? "HIGH" : "LOW",
                  digitalRead(LORA_RST) ? "HIGH" : "LOW",
                  digitalRead(LORA_BUSY) ? "HIGH" : "LOW",
                  digitalRead(LORA_DIO1) ? "HIGH" : "LOW");

    Serial.println("[RADIO] SPI pin test completed");
}

bool RadioHAL::send(const uint8_t *buf, size_t len)
{
    if (!lora)
    {
        Serial.println("[RADIO] ERROR: LoRa object is NULL!");
        return false;
    }

    Serial.printf("[RADIO] Starting transmission of %d bytes\n", len);
    Serial.printf("[RADIO] Current config: %.3f MHz, %d dBm, SF%d, BW%.1f kHz, CR4/%d\n",
                  frequency, txPower, spreadingFactor, bandwidth, codingRate);

    Serial.printf("[RADIO] Checking radio state before TX...\n");

    if (persist < 255)
    {
        Serial.printf("[RADIO] Performing CSMA (persistence: %d/255 = %d%%)\n",
                      persist, (persist * 100) / 255);
        performCSMA();
    }
    else
    {
        Serial.println("[RADIO] CSMA disabled (persistence = 255)");
    }

    if (txDelay > 0)
    {
        Serial.printf("[RADIO] Applying TX delay: %d ms\n", txDelay * 10);
        delay(txDelay * 10);
    }

    digitalWrite(LORA_PA_EN, HIGH);
    digitalWrite(LORA_PA_TX_EN, HIGH);
    Serial.println("[RADIO] V4 PA enabled for transmission");

    Serial.println("[RADIO] Calling lora->transmit()...");
    unsigned long txStart = millis();
    int st = lora->transmit((uint8_t *)buf, len);
    unsigned long txTime = millis() - txStart;

    Serial.printf("[RADIO] Transmit completed in %lu ms, result: %d\n", txTime, st);

    switch (st)
    {
    case RADIOLIB_ERR_NONE:
        Serial.println("[RADIO] [OK] RADIOLIB_ERR_NONE: Transmission successful!");
        break;
    case RADIOLIB_ERR_UNKNOWN:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_UNKNOWN");
        break;
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_CHIP_NOT_FOUND: Radio chip not responding!");
        break;
    case RADIOLIB_ERR_PACKET_TOO_LONG:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_PACKET_TOO_LONG");
        break;
    case RADIOLIB_ERR_TX_TIMEOUT:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_TX_TIMEOUT");
        break;
    case RADIOLIB_ERR_RX_TIMEOUT:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_RX_TIMEOUT");
        break;
    case RADIOLIB_ERR_CRC_MISMATCH:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_CRC_MISMATCH");
        break;
    case RADIOLIB_ERR_INVALID_BANDWIDTH:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_BANDWIDTH");
        break;
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_SPREADING_FACTOR");
        break;
    case RADIOLIB_ERR_INVALID_CODING_RATE:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_CODING_RATE");
        break;
    case RADIOLIB_ERR_INVALID_FREQUENCY:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_FREQUENCY");
        break;
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
        Serial.println("[RADIO] [FAIL] RADIOLIB_ERR_INVALID_OUTPUT_POWER");
        break;
    default:
        Serial.printf("[RADIO] [FAIL] Unknown error code: %d\n", st);
        break;
    }

    Serial.println("[RADIO] Restarting receive mode...");
    int rxResult = lora->startReceive();
    Serial.printf("[RADIO] Start receive result: %d\n", rxResult);

    bool success = (st == RADIOLIB_ERR_NONE);
    Serial.printf("[RADIO] Final result: %s\n", success ? "SUCCESS" : "FAILED");

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
        Serial.println("[RADIO] ERROR: LoRa object is NULL in applyRadioConfig()");
        return false;
    }

    Serial.println("[RADIO] Applying configuration to radio...");
    Serial.printf("[RADIO] Parameters: freq=%.3f MHz, bw=%.1f kHz, sf=%d, cr=4/%d, sync=0x%02X, power=%d dBm, preamble=%d\n",
                  frequency, bandwidth, spreadingFactor, codingRate, syncWord, txPower, preambleLength);

    int state = lora->begin(frequency, bandwidth, spreadingFactor,
                            codingRate, syncWord, txPower, preambleLength);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("[RADIO] [OK] Radio configuration applied successfully");

        Serial.println("[RADIO] Verifying applied configuration...");
        testRadioHealth();
    }
    else
    {
        Serial.printf("[RADIO] [FAIL] Radio configuration failed with error: %d\n", state);

        switch (state)
        {
        case RADIOLIB_ERR_INVALID_FREQUENCY:
            Serial.println("[RADIO] ERROR: Invalid frequency");
            break;
        case RADIOLIB_ERR_INVALID_BANDWIDTH:
            Serial.println("[RADIO] ERROR: Invalid bandwidth");
            break;
        case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
            Serial.println("[RADIO] ERROR: Invalid spreading factor");
            break;
        case RADIOLIB_ERR_INVALID_CODING_RATE:
            Serial.println("[RADIO] ERROR: Invalid coding rate");
            break;
        case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
            Serial.println("[RADIO] ERROR: Invalid output power");
            break;
        case RADIOLIB_ERR_CHIP_NOT_FOUND:
            Serial.println("[RADIO] ERROR: Radio chip not found - check SPI connections!");
            break;
        default:
            Serial.printf("[RADIO] ERROR: Unknown configuration error: %d\n", state);
            break;
        }
    }

    return state == RADIOLIB_ERR_NONE;
}

bool RadioHAL::testRadioHealth()
{
    if (!lora)
    {
        Serial.println("[RADIO] Health check FAILED: LoRa object is NULL");
        return false;
    }

    Serial.println("[RADIO] Starting radio health check...");

    Serial.print("[RADIO] Test 1 - Standby mode test: ");
    int standbyResult = lora->standby();
    if (standbyResult == RADIOLIB_ERR_NONE)
    {
        Serial.println("[PASS] - Radio responds to commands");
    }
    else
    {
        Serial.printf("[FAIL] (error: %d) - Radio not responding to commands\n", standbyResult);
        return false;
    }

    Serial.print("[RADIO] Test 2 - Frequency setting test: ");
    int freqResult = lora->setFrequency(frequency);
    if (freqResult == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[PASS] (%.3f MHz)\n", frequency);
    }
    else
    {
        Serial.printf("[FAIL] (error: %d)\n", freqResult);
        return false;
    }

    Serial.print("[RADIO] Test 3 - Power setting test: ");
    int powerResult = lora->setOutputPower(txPower);
    if (powerResult == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[PASS] (%d dBm)\n", txPower);
    }
    else
    {
        Serial.printf("[FAIL] (error: %d)\n", powerResult);
        return false;
    }

    Serial.print("[RADIO] Test 4 - Small packet transmission test: ");
    uint8_t testData[] = {0xAA, 0x55, 0xAA, 0x55};
    unsigned long txStart = millis();
    int testResult = lora->transmit(testData, 4);
    unsigned long txTime = millis() - txStart;

    if (testResult == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[PASS] (completed in %lu ms)\n", txTime);
    }
    else
    {
        Serial.printf("[FAIL] (error: %d, took %lu ms)\n", testResult, txTime);

        Serial.println("[RADIO] TX test failed - possible causes:");
        Serial.println("[RADIO] - SPI communication issue");
        Serial.println("[RADIO] - Radio module power issue");
        Serial.println("[RADIO] - Invalid configuration");
        Serial.println("[RADIO] - Hardware fault");

        return false;
    }

    Serial.print("[RADIO] Test 5 - Return to receive mode: ");
    int rxResult = lora->startReceive();
    if (rxResult == RADIOLIB_ERR_NONE)
    {
        Serial.println("[PASS]");
    }
    else
    {
        Serial.printf("[FAIL] (error: %d)\n", rxResult);
        return false;
    }

    Serial.println("[RADIO] Radio health check PASSED - hardware is responding correctly");
    Serial.println("[RADIO] If you're not seeing RF output, check:");
    Serial.println("[RADIO] - Antenna connection");
    Serial.println("[RADIO] - SDR frequency and settings");
    Serial.println("[RADIO] - RF shielding/interference");

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
    Serial.println("[RADIO] Starting comprehensive transmission test...");

    if (!lora)
    {
        Serial.println("[RADIO] Test FAILED: LoRa object is NULL");
        return;
    }

    Serial.println("[RADIO] Test 1: Current radio status");
    testRadioHealth();

    Serial.println("\n[RADIO] Test 2: Test packets of various sizes");

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
        Serial.printf("\n[RADIO] Testing %d byte packet...\n", testSizes[i]);

        uint8_t originalPersist = persist;
        uint8_t originalTxDelay = txDelay;
        persist = 255;
        txDelay = 0;

        unsigned long start = millis();
        int result = lora->transmit(testPackets[i], testSizes[i]);
        unsigned long duration = millis() - start;

        Serial.printf("[RADIO] Result: %d, Duration: %lu ms\n", result, duration);

        if (result == RADIOLIB_ERR_NONE)
        {
            Serial.println("[RADIO] SUCCESS");
        }
        else
        {
            Serial.printf("[RADIO] [FAIL] TX FAILED (error %d)\n", result);
        }

        persist = originalPersist;
        txDelay = originalTxDelay;

        delay(1000);
    }

    Serial.println("\n[RADIO] Test 3: Checking for any received packets");
    Serial.println("[RADIO] Restarting receive mode...");

    int rxResult = lora->startReceive();
    if (rxResult == RADIOLIB_ERR_NONE)
    {
        Serial.println("[RADIO] Receive mode started successfully");
        Serial.println("[RADIO] Monitor your SDR for the test transmissions above");
        Serial.println("[RADIO] Test packets should appear as brief bursts on your configured frequency");
    }
    else
    {
        Serial.printf("[RADIO] [FAIL] Failed to restart receive mode (error: %d)\n", rxResult);
    }

    Serial.println("\n[RADIO] Transmission test completed");
    Serial.println("[RADIO] Check SDR for actual RF output during the test");
}

void RadioHAL::checkHardwarePins()
{
    Serial.println("[RADIO] [TEST] Checking hardware pin configuration...");

    Serial.printf("[RADIO] Current pin definitions:\n");
    Serial.printf("[RADIO]   NSS (CS):   %d\n", LORA_NSS);
    Serial.printf("[RADIO]   SCLK:       %d\n", LORA_SCLK);
    Serial.printf("[RADIO]   MOSI:       %d\n", LORA_MOSI);
    Serial.printf("[RADIO]   MISO:       %d\n", LORA_MISO);
    Serial.printf("[RADIO]   RST:        %d\n", LORA_RST);
    Serial.printf("[RADIO]   BUSY:       %d\n", LORA_BUSY);
    Serial.printf("[RADIO]   DIO1:       %d\n", LORA_DIO1);
    Serial.printf("[RADIO]   PA_POWER:   %d\n", LORA_PA_POWER);
    Serial.printf("[RADIO]   PA_EN:      %d\n", LORA_PA_EN);
    Serial.printf("[RADIO]   PA_TX_EN:   %d\n", LORA_PA_TX_EN);

    Serial.println("[RADIO] Heltec WiFi LoRa 32 V4 pin configuration");
    Serial.println("[RADIO] Official Heltec V4 LoRa pins should be:");
    Serial.println("[RADIO]   NSS (CS):   8");
    Serial.println("[RADIO]   SCK:        9");
    Serial.println("[RADIO]   MOSI:      10");
    Serial.println("[RADIO]   MISO:      11");
    Serial.println("[RADIO]   RST:       12");
    Serial.println("[RADIO]   BUSY:      13");
    Serial.println("[RADIO]   DIO1:      14");

    Serial.println("[RADIO] Current pin states:");
    Serial.printf("[RADIO]   NSS:  %s\n", digitalRead(LORA_NSS) ? "HIGH" : "LOW");
    Serial.printf("[RADIO]   RST:  %s\n", digitalRead(LORA_RST) ? "HIGH" : "LOW");
    Serial.printf("[RADIO]   BUSY: %s\n", digitalRead(LORA_BUSY) ? "HIGH" : "LOW");
    Serial.printf("[RADIO]   DIO1: %s\n", digitalRead(LORA_DIO1) ? "HIGH" : "LOW");
    Serial.printf("[RADIO]   PA_EN: %s\n", digitalRead(LORA_PA_EN) ? "HIGH" : "LOW");
    Serial.printf("[RADIO]   PA_TX_EN: %s\n", digitalRead(LORA_PA_TX_EN) ? "HIGH" : "LOW");

    Serial.println("[RADIO] Testing pin manipulation...");

    digitalWrite(LORA_NSS, LOW);
    delay(10);
    bool nssLow = digitalRead(LORA_NSS) == LOW;
    digitalWrite(LORA_NSS, HIGH);
    delay(10);
    bool nssHigh = digitalRead(LORA_NSS) == HIGH;

    Serial.printf("[RADIO] NSS control test: %s\n", (nssLow && nssHigh) ? "[PASS]" : "[FAIL]");

    digitalWrite(LORA_RST, LOW);
    delay(10);
    bool rstLow = digitalRead(LORA_RST) == LOW;
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    bool rstHigh = digitalRead(LORA_RST) == HIGH;

    Serial.printf("[RADIO] RST control test: %s\n", (rstLow && rstHigh) ? "[PASS]" : "[FAIL]");
}

void RadioHAL::testContinuousTransmission()
{
    Serial.println("[RADIO] Starting continuous transmission test...");
    Serial.println("[RADIO] This will transmit continuously for 30 seconds");
    Serial.println("[RADIO] Monitor your SDR during this time!");

    if (!lora)
    {
        Serial.println("[RADIO] ERROR: LoRa not initialized");
        return;
    }

    Serial.println("[RADIO] Setting maximum power (22 dBm) for test...");
    lora->setOutputPower(22);

    uint8_t testPattern[] = {
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    Serial.println("[RADIO] Starting continuous transmission...");
    Serial.printf("[RADIO] Frequency: %.3f MHz\n", frequency);
    Serial.printf("[RADIO] Power: 22 dBm (max)\n");
    Serial.printf("[RADIO] Look for signals on your SDR NOW!\n");

    unsigned long startTime = millis();
    int transmissionCount = 0;

    while (millis() - startTime < 30000)
    {
        int result = lora->transmit(testPattern, sizeof(testPattern));
        transmissionCount++;

        if (result == RADIOLIB_ERR_NONE)
        {
            Serial.printf("[RADIO] TX #%d: SUCCESS\n", transmissionCount);
        }
        else
        {
            Serial.printf("[RADIO] TX #%d: FAILED (error %d)\n", transmissionCount, result);
        }

        delay(500);

        if (Serial.available())
        {
            char c = Serial.read();
            if (c == 'q' || c == 'Q')
            {
                Serial.println("[RADIO] Test aborted by user");
                break;
            }
        }
    }

    Serial.println("[RADIO] Continuous transmission test completed");
    Serial.printf("[RADIO] Total transmissions: %d\n", transmissionCount);

    lora->setOutputPower(txPower);
    lora->startReceive();

    Serial.println("[RADIO] If you saw NO activity on SDR during this test:");
    Serial.println("[RADIO] 1. Check antenna connection");
    Serial.println("[RADIO] 2. Verify SDR frequency setting");
    Serial.println("[RADIO] 3. Pin configuration may be wrong");
    Serial.println("[RADIO] 4. Hardware fault possible");
}
