// LoRaRadio.cpp
#include "LoRaRadio.h"
#include "AX25.h"

LoRaRadio::LoRaRadio(int8_t cs, int8_t busy, int8_t dio0, int8_t rst,
                     int8_t paEnPin, int8_t paTxEnPin, int8_t paPowerPin)
    : _cs(cs), _busy(busy), _dio0(dio0), _rst(rst), _paEnPin(paEnPin), _paTxEnPin(paTxEnPin), _paPowerPin(paPowerPin)
{
}

LoRaRadio::~LoRaRadio()
{
  // Stop the RX task if running
  stopRxTask();

  // Clean up dynamically allocated objects
  if (_radio)
  {
    delete _radio;
    _radio = nullptr;
  }
  if (_mod)
  {
    delete _mod;
    _mod = nullptr;
  }
}

void LoRaRadio::setRxHandler(RxHandler h)
{
  _rxHandler = h;
}

bool LoRaRadio::begin(float freq)
{
  _freq = freq;
  // Create module and radio objects dynamically so we can keep header lightweight
  // Module signature: Module(cs, dio0, rst, busy) is used by RadioLib examples. If this
  // changes across versions, adapt accordingly.
  _mod = new Module(_cs, _dio0, _rst, _busy);
  // RadioLib's SX1262 constructor takes a Module* (SPI handled internally)
  _radio = new SX1262(_mod);

  // optional PA control pins (follow HelTec factory behavior)
  if (_paPowerPin >= 0)
  {
    // set power control pin as an output and enable (factory uses analog write)
    pinMode(_paPowerPin, OUTPUT);
    digitalWrite(_paPowerPin, HIGH);
  }
  if (_paEnPin >= 0)
  {
    pinMode(_paEnPin, OUTPUT);
    // enable PA by default during initialization (factory code enables PA pins before radio init)
    digitalWrite(_paEnPin, HIGH);
  }
  if (_paTxEnPin >= 0)
  {
    pinMode(_paTxEnPin, OUTPUT);
    digitalWrite(_paTxEnPin, HIGH);
  }

  // small settle time after enabling PA pins
  if ((_paEnPin >= 0) || (_paTxEnPin >= 0) || (_paPowerPin >= 0))
  {
    delay(5);
  }

  // try to initialize
  int16_t state = _radio->begin(_freq);
  if (state != RADIOLIB_ERR_NONE)
  {
    delete _radio;
    _radio = nullptr;
    delete _mod;
    _mod = nullptr;
    return false;
  }

  // default radio settings - these can be adjusted via setters
  _radio->setFrequency(915.0);
  _radio->setBandwidth(125.0);
  _radio->setSpreadingFactor(7);
  _radio->setCodingRate(5);
  _radio->setOutputPower(14);
  _txPower = 14;

  // Start in receive mode (proven method from previous implementation)
  int16_t rxState = _radio->startReceive();
  if (rxState != RADIOLIB_ERR_NONE)
  {
    Serial.print("Failed to start receive mode, error: ");
    Serial.println(rxState);
    return false;
  }

  // Start the RX polling task on separate core
  startRxTask();

  return true;
}

int LoRaRadio::setTxPower(int8_t power)
{
  if (!_radio)
    return -1;
  int res = _radio->setOutputPower(power);
  if (res == RADIOLIB_ERR_NONE)
  {
    _txPower = power;
  }
  return res;
}

int LoRaRadio::setFrequency(float freq)
{
  if (!_radio)
    return -1;
  _freq = freq;
  return _radio->setFrequency(_freq);
}

float LoRaRadio::getFrequency() const
{
  return _freq;
}

int8_t LoRaRadio::getTxPower() const
{
  return _txPower;
}

int LoRaRadio::setSpreadingFactor(int sf)
{
  if (!_radio)
    return -1;
  int r = _radio->setSpreadingFactor(sf);
  if (r == RADIOLIB_ERR_NONE) _spreadingFactor = sf;
  return r;
}

int LoRaRadio::setBandwidth(long bw)
{
  if (!_radio)
    return -1;
  int r = _radio->setBandwidth(bw);
  if (r == RADIOLIB_ERR_NONE) _bandwidth = bw;
  return r;
}

int LoRaRadio::getSpreadingFactor() const
{
  return _spreadingFactor;
}

long LoRaRadio::getBandwidth() const
{
  return _bandwidth;
}

int LoRaRadio::send(const uint8_t *buf, size_t len, unsigned long timeout)
{
  if (!_radio)
    return -1;
  // enforce hardware max payload
  const size_t maxlen = (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH;
  if (len > maxlen)
  {
    return -2; // payload too large
  }

  // If PA pins exist, enable them before transmit (mimic factory test)
  if (_paEnPin >= 0)
    digitalWrite(_paEnPin, HIGH);
  if (_paTxEnPin >= 0)
    digitalWrite(_paTxEnPin, HIGH);
  if ((_paEnPin >= 0) || (_paTxEnPin >= 0))
  {
    delay(2); // small settle time
  }

  // send packet (blocking)
  // Debug: print outgoing packet hex for troubleshooting
  Serial.printf("[LoRaRadio] TX len=%u data= ", (unsigned)len);
  for (size_t i = 0; i < len; i++) Serial.printf("%02X", buf[i]);
  Serial.println();

  int16_t result = _radio->transmit(buf, len);

  // disable PA pins after transmit
  if (_paEnPin >= 0)
    digitalWrite(_paEnPin, LOW);
  if (_paTxEnPin >= 0)
    digitalWrite(_paTxEnPin, LOW);

  // Restart receive mode after transmit (proven method)
  _radio->startReceive();

  return result;
}

// Static task function for FreeRTOS
void LoRaRadio::rxTaskFunction(void *parameter)
{
  LoRaRadio *radio = static_cast<LoRaRadio *>(parameter);

  while (radio->_rxTaskRunning)
  {
    radio->pollInternal();
    // Small delay to yield to other tasks and prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  // Task ending, delete itself
  radio->_rxTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

void LoRaRadio::startRxTask()
{
  if (_rxTaskHandle != nullptr)
    return; // Already running

  _rxTaskRunning = true;

  // Create task on core 0 (core 1 typically runs Arduino loop)
  // Stack size: 4096 bytes, priority: 1 (low priority, won't block serial)
  xTaskCreatePinnedToCore(
      rxTaskFunction, // Task function
      "LoRaRxTask",   // Task name
      4096,           // Stack size (bytes)
      this,           // Parameter passed to task
      1,              // Priority (1 = low, lower than default loop priority)
      &_rxTaskHandle, // Task handle
      0               // Core 0 (Arduino loop runs on core 1)
  );
}

void LoRaRadio::stopRxTask()
{
  if (_rxTaskHandle == nullptr)
    return; // Not running

  _rxTaskRunning = false;

  // Wait for task to finish (max 1 second)
  unsigned long start = millis();
  while (_rxTaskHandle != nullptr && (millis() - start) < 1000)
  {
    delay(10);
  }
}

void LoRaRadio::poll()
{
  // Deprecated: for backward compatibility, just call internal poll
  pollInternal();
}

void LoRaRadio::pollInternal()
{
  if (!_radio)
    return;

  uint8_t buf[256];
  size_t buflen = sizeof(buf);

  // Use receive() with 0 timeout instead of readData() in startReceive() mode
  // This properly handles the RX complete flag and won't return the same packet repeatedly
  String str;
  int16_t res = _radio->receive(str, 0); // 0 = non-blocking, return immediately

  // Check result
  if (res == RADIOLIB_ERR_RX_TIMEOUT)
  {
    // No packet available, this is normal in non-blocking mode
    return;
  }

  if (res != RADIOLIB_ERR_NONE)
  {
    // Actual error occurred
    if (res != RADIOLIB_ERR_CRC_MISMATCH)
    {
      Serial.print("Receive error: ");
      Serial.println(res);
    }
    return;
  }

  // Success! We have a packet
  if (str.length() == 0)
  {
    // Got success but no data - shouldn't happen, but handle it
    return;
  }

  // We have actual data - process it
  size_t plen = min((size_t)str.length(), buflen);
  memcpy(buf, str.c_str(), plen);

  if (plen > 0 && plen <= buflen)
  {
    int rssi = (int)_radio->getRSSI();
    String from = String("");
    String payload = String("");

    // try to parse AX.25 addresses to extract source callsign and payload
    AX25::AddrInfo ai = AX25::parseAddresses(buf, plen);

    // Debug: print received packet summary to Serial to aid troubleshooting
    {
      Serial.printf("[LoRaRadio] RX len=%u rssi=%d parse_ok=%d\r\n", (unsigned)plen, rssi, ai.ok ? 1 : 0);
      // print parsed addresses when available
      if (ai.ok)
      {
        Serial.printf("[LoRaRadio] dest=%s src=%s hdr_len=%u\r\n", ai.dest.c_str(), ai.src.c_str(), (unsigned)ai.header_len);
      }
      // hex dump first up to 64 bytes
      Serial.print("[LoRaRadio] data= ");
      for (size_t i = 0; i < plen && i < 64; i++)
      {
        Serial.printf("%02X", buf[i]);
        if (i + 1 < plen && (i + 1) % 16 == 0)
          Serial.print(' ');
      }
      Serial.println();
    }

    if (ai.ok)
    {
      from = ai.src;
      // Extract payload: skip AX.25 header, exclude trailing FCS (2 bytes)
      if (ai.header_len < plen)
      {
        size_t payload_len = plen - ai.header_len;
        // AX.25 frames have 2-byte FCS at the end, exclude it
        if (payload_len > 2)
        {
          payload_len -= 2;
        }
        payload = String((const char *)(buf + ai.header_len), payload_len);
      }
      // Debug: print control field when present
      if (ai.hasControl) {
        Serial.printf("[LoRaRadio] control=0x%02X\r\n", ai.control);
      }
    }
    else
    {
      // If parsing failed, treat entire packet as payload (fallback for non-AX25 packets)
      payload = String((const char *)buf, plen);
    }

    if (_rxHandler)
    {
      _rxHandler(buf, plen, ai, rssi);
    }
  }

  // receive() with timeout automatically manages RX state, no need to call startReceive() here
}
