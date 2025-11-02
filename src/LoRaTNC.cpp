#include "LoRaTNC.h"

// Static instance pointer for callbacks
LoRaTNC *LoRaTNC::instance = nullptr;

LoRaTNC::LoRaTNC(LoRaRadio *lora)
{
    loraRadio = lora;
    kissProtocol = new KissProtocol();

    // Set this as the singleton instance for callbacks
    instance = this;

    // Initialize state
    currentMode = TNC_MODE_COMMAND;

    // Initialize statistics
    memset(&stats, 0, sizeof(stats));

    // Initialize configuration
    beaconEnabled = false;
    beaconInterval = 600000; // 10 minutes default
    lastBeaconTime = 0;
    beaconText = "";

    csmaEnabled = true;
    csmaSlotTime = 100; // 100ms
    csmaMaxRetries = 10;
}

LoRaTNC::~LoRaTNC()
{
    if (kissProtocol)
    {
        delete kissProtocol;
    }

    if (instance == this)
    {
        instance = nullptr;
    }
}

bool LoRaTNC::begin()
{
    Serial.println("[TNC] Initializing LoRa TNC...");

    // Initialize KISS protocol
    kissProtocol->begin();

    // Set up KISS callbacks
    kissProtocol->onDataFrame(kissDataFrameWrapper);
    kissProtocol->onCommand(kissCommandWrapper);
    kissProtocol->onLoRaCommand(kissLoRaCommandWrapper);

    // LoRa callbacks are assumed to be set up externally
    // since we need the LoRaRadio to call our methods

    Serial.println("[TNC] LoRa TNC initialized successfully");
    printConfiguration();

    return true;
}

void LoRaTNC::reset()
{
    // Reset statistics
    memset(&stats, 0, sizeof(stats));

    // Reset KISS protocol
    kissProtocol->reset();

    // Reset beacon timer
    lastBeaconTime = millis();

    Serial.println("[TNC] TNC reset completed");
}

void LoRaTNC::setMode(tnc_mode_t mode)
{
    if (currentMode == mode)
    {
        return;
    }

    tnc_mode_t oldMode = currentMode;
    currentMode = mode;

    if (mode == TNC_MODE_KISS)
    {
        // TAPR TNC-2 compatible KISS mode entry
        // Silent entry - no messages sent to host
        // The TNC should simply start accepting KISS frames
        
        // Reset KISS protocol state
        kissProtocol->reset();
        
        // In TNC-2, entering KISS mode is silent - no output
        // Debug messages only if not coming from command line
        if (oldMode != TNC_MODE_COMMAND)
        {
            Serial.println("[TNC] Entering KISS mode - binary protocol active");
        }
    }
    else if (oldMode == TNC_MODE_KISS)
    {
        // Exiting KISS mode back to command mode
        Serial.println("\r\ncmd:");  // TNC-2 style command mode prompt
    }
    else
    {
        // Other mode transitions (for debugging)
        Serial.printf("[TNC] Mode changed from %s to %s\n",
                      (oldMode == TNC_MODE_COMMAND) ? "COMMAND" : (oldMode == TNC_MODE_KISS) ? "KISS" : "TRANSPARENT",
                      (mode == TNC_MODE_COMMAND) ? "COMMAND" : (mode == TNC_MODE_KISS) ? "KISS" : "TRANSPARENT");
    }
}

bool LoRaTNC::enterKissMode()
{
    setMode(TNC_MODE_KISS);
    return true;
}

bool LoRaTNC::exitKissMode()
{
    setMode(TNC_MODE_COMMAND);
    return true;
}

void LoRaTNC::setBeacon(bool enabled, uint32_t intervalMs, const String &text)
{
    beaconEnabled = enabled;
    beaconInterval = intervalMs;
    beaconText = text;
    lastBeaconTime = millis();

    Serial.printf("[TNC] Beacon %s", enabled ? "enabled" : "disabled");
    if (enabled)
    {
        Serial.printf(" - interval: %u ms, text: \"%s\"", intervalMs, text.c_str());
    }
    Serial.println();
}

void LoRaTNC::enableCSMA(bool enable, uint16_t slotTimeMs, uint8_t maxRetries)
{
    csmaEnabled = enable;
    csmaSlotTime = slotTimeMs;
    csmaMaxRetries = maxRetries;

    Serial.printf("[TNC] CSMA/CD %s", enable ? "enabled" : "disabled");
    if (enable)
    {
        Serial.printf(" - slot time: %u ms, max retries: %u", slotTimeMs, maxRetries);
    }
    Serial.println();
}

bool LoRaTNC::channelIsBusy()
{
    // Simple carrier sense - check if we're currently receiving
    // In a real implementation, you might check RSSI threshold
    return (loraRadio->getState() == LORA_STATE_RX);
}

bool LoRaTNC::waitForChannel()
{
    if (!csmaEnabled)
    {
        return true; // No CSMA, always transmit
    }

    for (uint8_t retry = 0; retry < csmaMaxRetries; retry++)
    {
        // Wait random number of slot times
        uint16_t slots = random(1, 16); // 1-15 slots
        delay(slots * csmaSlotTime);

        // Check if channel is clear
        if (!channelIsBusy())
        {
            return true; // Channel is clear
        }

        Serial.printf("[TNC] Channel busy, retry %d/%d\n", retry + 1, csmaMaxRetries);
    }

    Serial.println("[TNC] Channel remained busy, dropping packet");
    stats.transmitErrors++;
    return false;
}

void LoRaTNC::handleKissDataFrame(uint8_t *data, uint16_t length)
{
    stats.packetsFromHost++;

    // Check if data length exceeds LoRa maximum
    if (length > LORA_BUFFER_SIZE)
    {
        stats.transmitErrors++;
        return;
    }

    // Wait for clear channel if CSMA is enabled
    if (!waitForChannel())
    {
        return; // Channel busy, packet dropped
    }

    // Transmit over LoRa
    int result = loraRadio->send(data, length);
    if (result != RADIOLIB_ERR_NONE)
    {
        stats.transmitErrors++;
    }
    
    // TNC-2 behavior: no confirmation messages in KISS mode
    // Uncomment for debugging:
    // Serial.printf("[DEBUG] KISS data frame: %d bytes, result: %d\n", length, result);
}

void LoRaTNC::handleKissCommand(uint8_t command, uint8_t parameter)
{
    // Handle KISS commands
    switch (command)
    {
    case KISS_CMD_RETURN:
        // Exit KISS mode - return to command mode
        exitKissMode();
        break;
        
    case KISS_CMD_TXDELAY:
    case KISS_CMD_P:
    case KISS_CMD_SLOTTIME:
    case KISS_CMD_TXTAIL:
    case KISS_CMD_FULLDUPLEX:
        // Configuration commands - no response needed in TNC-2 mode
        // Parameters are already updated by KissProtocol::processCommand()
        break;
        
    case KISS_CMD_SETHARDWARE:
        // Hardware-specific command - acknowledge but ignore
        break;
        
    default:
        // Unknown command - silently ignore (TNC-2 behavior)
        break;
    }
    
    // Only log commands in debug mode (not in normal KISS operation)
    // Uncomment the line below for debugging:
    // Serial.printf("[DEBUG] KISS command: %s (0x%02X) parameter: %d\n",
    //               KissProtocol::commandToString(command).c_str(), command, parameter);
}

void LoRaTNC::handleKissLoRaCommand(uint8_t command, uint8_t *data, uint16_t length)
{
    bool success = false;
    uint8_t responseData[8];
    uint16_t responseLength = 0;
    
    // Handle LoRa-specific KISS commands
    switch (command)
    {
    case KISS_LORA_SET_TXPOWER:
        if (length >= 1)
        {
            int8_t power = (int8_t)data[0];
            int result = loraRadio->setTxPower(power);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SET_BANDWIDTH:
        if (length >= 1)
        {
            float bandwidth = KissProtocol::bandwidthIndexToValue(data[0]);
            int result = loraRadio->setBandwidth(bandwidth);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SET_SF:
        if (length >= 1)
        {
            uint8_t sf = data[0];
            if (sf >= 6 && sf <= 12)
            {
                int result = loraRadio->setSpreadingFactor(sf);
                success = (result == RADIOLIB_ERR_NONE);
            }
        }
        break;
        
    case KISS_LORA_SET_CR:
        if (length >= 1)
        {
            uint8_t cr = data[0];
            if (cr >= 5 && cr <= 8)
            {
                int result = loraRadio->setCodingRate(cr);
                success = (result == RADIOLIB_ERR_NONE);
            }
        }
        break;
        
    case KISS_LORA_SET_PREAMBLE:
        if (length >= 2)
        {
            uint16_t preamble = (data[0] << 8) | data[1];
            int result = loraRadio->setPreambleLength(preamble);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SET_SYNCWORD:
        if (length >= 1)
        {
            uint8_t syncWord = data[0];
            int result = loraRadio->setSyncWord(syncWord);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SET_CRC:
        if (length >= 1)
        {
            bool enableCrc = (data[0] != 0);
            int result = loraRadio->setCRC(enableCrc);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SET_FREQ_LOW:
        if (length >= 4)
        {
            uint32_t frequencyHz = (uint32_t(data[0]) << 24) | 
                                  (uint32_t(data[1]) << 16) | 
                                  (uint32_t(data[2]) << 8) | 
                                  uint32_t(data[3]);
            float frequencyMHz = frequencyHz / 1000000.0;
            int result = loraRadio->setFrequency(frequencyMHz);
            success = (result == RADIOLIB_ERR_NONE);
        }
        break;
        
    case KISS_LORA_SELECT_BAND:
        if (length >= 1)
        {
            uint8_t bandIndex = data[0];
            // Get available bands from the frequency band manager
            FrequencyBandManager* bandManager = loraRadio->getBandManager();
            if (bandManager)
            {
                auto bands = bandManager->getAvailableBands();
                if (bandIndex < bands.size())
                {
                    success = bandManager->selectBand(bands[bandIndex].identifier);
                    if (success)
                    {
                        // Set the radio frequency to the band's default
                        loraRadio->setFrequency(bands[bandIndex].defaultFrequency);
                    }
                }
            }
        }
        break;
        
    case KISS_LORA_GET_CONFIG:
        {
            // Send current configuration as response
            lora_config_t config = loraRadio->getConfig();
            
            // Send frequency (4 bytes)
            uint32_t freqHz = (uint32_t)(config.frequency * 1000000);
            responseData[0] = (freqHz >> 24) & 0xFF;
            responseData[1] = (freqHz >> 16) & 0xFF;
            responseData[2] = (freqHz >> 8) & 0xFF;
            responseData[3] = freqHz & 0xFF;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_FREQ_LOW, responseData, 4);
            
            // Send TX power
            responseData[0] = (uint8_t)config.txPower;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_TXPOWER, responseData, 1);
            
            // Send bandwidth
            responseData[0] = KissProtocol::bandwidthValueToIndex(config.bandwidth);
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_BANDWIDTH, responseData, 1);
            
            // Send spreading factor
            responseData[0] = config.spreadingFactor;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_SF, responseData, 1);
            
            // Send coding rate
            responseData[0] = config.codingRate;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_CR, responseData, 1);
            
            // Send preamble length
            responseData[0] = (config.preambleLength >> 8) & 0xFF;
            responseData[1] = config.preambleLength & 0xFF;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_PREAMBLE, responseData, 2);
            
            // Send sync word
            responseData[0] = config.syncWord;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_SYNCWORD, responseData, 1);
            
            // Send CRC setting
            responseData[0] = config.crcEnabled ? 1 : 0;
            kissProtocol->sendLoRaResponse(KISS_LORA_SET_CRC, responseData, 1);
            
            success = true;
        }
        break;
        
    case KISS_LORA_SAVE_CONFIG:
        {
            // Save configuration to NVS (if implemented)
            FrequencyBandManager* bandManager = loraRadio->getBandManager();
            if (bandManager)
            {
                success = bandManager->saveConfiguration();
            }
        }
        break;
        
    case KISS_LORA_RESET_CONFIG:
        {
            // Reset to default configuration
            loraRadio->setFrequency(915.0);         // Default frequency
            loraRadio->setTxPower(LORA_DEFAULT_TX_POWER);
            loraRadio->setBandwidth(LORA_BANDWIDTH_DEFAULT);
            loraRadio->setSpreadingFactor(LORA_SPREADING_FACTOR_DEFAULT);
            loraRadio->setCodingRate(LORA_CODINGRATE_DEFAULT);
            loraRadio->setPreambleLength(LORA_PREAMBLE_LENGTH_DEFAULT);
            loraRadio->setSyncWord(LORA_SYNC_WORD_DEFAULT);
            loraRadio->setCRC(true);
            success = true;
        }
        break;
        
    default:
        // Unknown LoRa command - silently ignore
        break;
    }
    
    // For commands that don't send their own responses, send a simple ACK/NAK
    if (command != KISS_LORA_GET_CONFIG)
    {
        responseData[0] = success ? 0x01 : 0x00;  // 1 = success, 0 = failure
        kissProtocol->sendLoRaResponse(command, responseData, 1);
    }
}

void LoRaTNC::handleLoRaReceive(uint8_t *payload, uint16_t size, int16_t rssi, float snr)
{
    stats.packetsReceived++;

    if (currentMode == TNC_MODE_KISS)
    {
        // Forward to host via KISS - TNC-2 behavior: silent operation
        kissProtocol->sendDataFrame(payload, size);
        stats.packetsToHost++;
    }
    else
    {
        // Command mode - display the packet
        char message[size + 1];
        memcpy(message, payload, size);
        message[size] = '\0';

        Serial.printf("[TNC] LoRa RX: \"%s\" (RSSI: %d dBm, SNR: %.1f dB, Size: %d)\n",
                      message, rssi, snr, size);
    }
}

void LoRaTNC::handleLoRaTransmitDone()
{
    stats.packetsTransmitted++;

    // TNC-2 KISS behavior: silent operation
    if (currentMode == TNC_MODE_COMMAND)
    {
        Serial.println("[TNC] LoRa TX completed successfully");
    }
}

void LoRaTNC::handleLoRaTransmitTimeout()
{
    stats.transmitErrors++;

    // TNC-2 KISS behavior: silent operation, no error messages to host
    if (currentMode == TNC_MODE_COMMAND)
    {
        Serial.println("[TNC] LoRa TX timeout occurred");
    }
}

void LoRaTNC::printStatistics()
{
    Serial.println("[TNC] Statistics:");
    Serial.printf("  Packets from Host: %u\n", stats.packetsFromHost);
    Serial.printf("  Packets to Host: %u\n", stats.packetsToHost);
    Serial.printf("  Packets Transmitted: %u\n", stats.packetsTransmitted);
    Serial.printf("  Packets Received: %u\n", stats.packetsReceived);
    Serial.printf("  Transmit Errors: %u\n", stats.transmitErrors);
    Serial.printf("  Receive Errors: %u\n", stats.receiveErrors);
    Serial.printf("  KISS Errors: %u\n", kissProtocol->getFrameErrors());

    Serial.printf("  Current Mode: %s\n",
                  (currentMode == TNC_MODE_COMMAND) ? "COMMAND" : (currentMode == TNC_MODE_KISS) ? "KISS"
                                                                                                 : "TRANSPARENT");
}

void LoRaTNC::clearStatistics()
{
    memset(&stats, 0, sizeof(stats));
    Serial.println("[TNC] Statistics cleared");
}

void LoRaTNC::printConfiguration()
{
    Serial.println("[TNC] Configuration:");
    Serial.printf("  Mode: %s\n",
                  (currentMode == TNC_MODE_COMMAND) ? "COMMAND" : (currentMode == TNC_MODE_KISS) ? "KISS"
                                                                                                 : "TRANSPARENT");
    Serial.printf("  Beacon: %s", beaconEnabled ? "Enabled" : "Disabled");
    if (beaconEnabled)
    {
        Serial.printf(" (interval: %u ms, text: \"%s\")", beaconInterval, beaconText.c_str());
    }
    Serial.println();
    Serial.printf("  CSMA/CD: %s", csmaEnabled ? "Enabled" : "Disabled");
    if (csmaEnabled)
    {
        Serial.printf(" (slot: %u ms, retries: %u)", csmaSlotTime, csmaMaxRetries);
    }
    Serial.println();
}

void LoRaTNC::sendTestFrame()
{
    String testData = "LoRaTNC Test Frame - " + String(millis());

    if (currentMode == TNC_MODE_KISS)
    {
        // Send as KISS frame
        kissProtocol->sendDataFrame((uint8_t *)testData.c_str(), testData.length());
    }
    else
    {
        // Send directly over LoRa
        loraRadio->send(testData);
    }

    Serial.printf("[TNC] Test frame sent: \"%s\"\n", testData.c_str());
}

bool LoRaTNC::isConnected()
{
    // In TNC mode, we consider ourselves "connected" if LoRa is operational
    return (loraRadio->getState() != LORA_STATE_SLEEP);
}

void LoRaTNC::handle()
{
    // Handle KISS protocol if in KISS mode
    if (currentMode == TNC_MODE_KISS)
    {
        kissProtocol->handleSerialInput();
    }

    // Handle beacon transmission
    if (beaconEnabled && (millis() - lastBeaconTime) >= beaconInterval)
    {
        lastBeaconTime = millis();

        if (beaconText.length() > 0)
        {
            Serial.println("[TNC] Sending beacon");
            loraRadio->send(beaconText);
        }
    }

    // LoRa handling is done externally through callbacks
}

// Static callback wrappers
void LoRaTNC::kissDataFrameWrapper(uint8_t *data, uint16_t length)
{
    if (instance)
    {
        instance->handleKissDataFrame(data, length);
    }
}

void LoRaTNC::kissCommandWrapper(uint8_t command, uint8_t parameter)
{
    if (instance)
    {
        instance->handleKissCommand(command, parameter);
    }
}

void LoRaTNC::kissLoRaCommandWrapper(uint8_t command, uint8_t *data, uint16_t length)
{
    if (instance)
    {
        instance->handleKissLoRaCommand(command, data, length);
    }
}