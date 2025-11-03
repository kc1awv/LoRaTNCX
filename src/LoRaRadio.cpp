// LoRaRadio.cpp
#include "LoRaRadio.h"
#include "AX25.h"

LoRaRadio::LoRaRadio(int8_t cs, int8_t busy, int8_t dio0, int8_t rst,
                     int8_t paEnPin, int8_t paTxEnPin, int8_t paPowerPin)
    : _cs(cs), _busy(busy), _dio0(dio0), _rst(rst), _paEnPin(paEnPin), _paTxEnPin(paTxEnPin), _paPowerPin(paPowerPin)
{
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
  return _radio->setSpreadingFactor(sf);
}

int LoRaRadio::setBandwidth(long bw)
{
  if (!_radio)
    return -1;
  return _radio->setBandwidth(bw);
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
  int16_t result = _radio->transmit(buf, len);

  // disable PA pins after transmit
  if (_paEnPin >= 0)
    digitalWrite(_paEnPin, LOW);
  if (_paTxEnPin >= 0)
    digitalWrite(_paTxEnPin, LOW);

  return result;
}

void LoRaRadio::poll()
{
  if (!_radio)
    return;

  // try a short non-blocking receive window by calling RadioLib receive with a small timeout
  uint8_t buf[256];
  size_t buflen = sizeof(buf);
  // call RadioLib receive with a short timeout
  int16_t res = _radio->receive(buf, buflen, 10);
  if (res == RADIOLIB_ERR_NONE)
  {
    // query actual packet length
    size_t plen = _radio->getPacketLength();
    if (plen > 0 && plen <= buflen)
    {
      String payload((const char *)buf, plen);
      int rssi = (int)_radio->getRSSI(true);
      String from = String("");
      // try to parse AX.25 addresses to extract source callsign
      AX25::AddrInfo ai = AX25::parseAddresses(buf, plen);
      if (ai.ok)
        from = ai.src;
      if (_rxHandler)
      {
        _rxHandler(from, payload, rssi);
      }
    }
  }
}
