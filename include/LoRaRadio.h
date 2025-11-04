// LoRaRadio.h
// Lightweight wrapper for SX1262 using RadioLib.
// - Configurable pins (CS, RST, BUSY, DIO0, optional PA enable pin)
// - Exposes begin(), setTxPower(), setFrequency(), send()
// - If `paPin` >= 0, it will be asserted before transmit and released after transmit.

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <functional>
#include "AX25.h"

class LoRaRadio
{
public:
  // Construct with SPI pins / control pins. Pass -1 for optional PA pins if not used.
  // paEnPin: main PA enable (LORA_PA_EN)
  // paTxEnPin: PA TX enable (LORA_PA_TX_EN)
  // paPowerPin: analog power control (LORA_PA_POWER)
  LoRaRadio(int8_t cs, int8_t busy, int8_t dio0, int8_t rst,
            int8_t paEnPin = -1, int8_t paTxEnPin = -1, int8_t paPowerPin = -1);

  ~LoRaRadio();

  // Initialize the radio. Returns true on success.
  bool begin(float freq = 915.0);

  // Parameter setters
  int setTxPower(int8_t power); // dBm
  int setFrequency(float freq);
  int setSpreadingFactor(int sf);
  int setBandwidth(long bw);
  int setCodingRate(int cr);

  // getters for modem params
  int getSpreadingFactor() const;
  long getBandwidth() const;
  int getCodingRate() const;

  // getters
  float getFrequency() const;
  int8_t getTxPower() const;
  
  // Get statistics from last received packet
  int getLastRSSI() const;
  float getLastSNR() const;
  int getLastFreqError() const;

  // Simple blocking send. Returns 0 on success, non-zero error code on failure.
  // max payload enforced to RADIOLIB_SX126X_MAX_PACKET_LENGTH (usually 255)
  // Returns 0 on success, negative on local error (e.g. -2 payload too large), or RadioLib error codes.
  int send(const uint8_t *buf, size_t len, unsigned long timeout = 5000);

  // Rx support: set a receive handler (from, payload, rssi)
  // Rx handler now provides the raw frame buffer, its length, parsed AX.25 address info and RSSI
  using RxHandler = std::function<void(const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)>;
  void setRxHandler(RxHandler h);

  // Start/stop the RX task (runs in separate FreeRTOS task)
  void startRxTask();
  void stopRxTask();

private:
  // Static task function for FreeRTOS
  static void rxTaskFunction(void *parameter);

  // Internal poll method called by task
  void pollInternal();
  int8_t _cs, _busy, _dio0, _rst;
  int8_t _paEnPin = -1;
  int8_t _paTxEnPin = -1;
  int8_t _paPowerPin = -1;
  Module *_mod = nullptr;
  SX1262 *_radio = nullptr;
  // store some current settings
  float _freq = 915.0;
  int8_t _txPower = 0;
  int _spreadingFactor = 7;
  long _bandwidth = 125;
  int _codingRate = 5;
  RxHandler _rxHandler = nullptr;

  // Last packet statistics
  int _lastRSSI = 0;
  float _lastSNR = 0.0;
  int _lastFreqError = 0;

  // FreeRTOS task handle for RX polling
  TaskHandle_t _rxTaskHandle = nullptr;
  volatile bool _rxTaskRunning = false;
};
