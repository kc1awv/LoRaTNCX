// LoRaTNCX.h
#pragma once

#include <Arduino.h>
#include "LoRaRadio.h"
#include "CommandProcessor.h"
#include <Preferences.h>
#include <vector>

class LoRaTNCX
{
public:
  LoRaTNCX(Stream &io, LoRaRadio &radio);
  // initialize the TNC (wire radio defaults, command registrations)
  void begin();

  // call from loop()
  void poll();

private:
  Stream &_io;
  LoRaRadio &_radio;
  CommandProcessor _cmd;
  Preferences _prefs;

  // persistent settings
  String _myCall;
  bool _monitorOn = false;

  // Beacon settings
  enum BeaconMode
  {
    BEACON_OFF = 0,
    BEACON_EVERY,
    BEACON_AFTER
  };
  BeaconMode _beaconMode = BEACON_OFF;
  uint32_t _beaconInterval = 0; // seconds
  String _beaconText;
  // UNPROTO digipeat path (persisted as comma-separated string)
  static const int UNPROTO_MAX = 8;
  std::vector<String> _unproto;

  // simple heard list (in-memory, small fixed size)
  static const int MHEARD_MAX = 32;
  String _mheard[MHEARD_MAX];
  int _mheard_count = 0;

  // connect state
  String _connectedTo;
  // beacon scheduling
  uint32_t _lastBeaconMs = 0;

  // add a received packet or heard station
  void addMHeard(const String &callsign);
  // interface for external RX code to notify TNC of a received packet
  void onPacketReceived(const String &from, const String &payload, int rssi = 0);

  // command handlers
  void cmdHelp(const String &args);
  void cmdVersion(const String &args);
  void cmdStatus(const String &args);
  void cmdFreq(const String &args);
  void cmdPwr(const String &args);
  void cmdSend(const String &args);
  void cmdRadioInit(const String &args);
};
