// LoRaTNCX.cpp
#include "LoRaTNCX.h"
#include "AX25.h"
#include <vector>
#include <Preferences.h>

LoRaTNCX::LoRaTNCX(Stream &io, LoRaRadio &radio)
  : _io(io), _radio(radio), _cmd(io, radio) {
}

void LoRaTNCX::begin() {
  // open preferences namespace
  _prefs.begin("loratncx", false);

  // load persisted settings
  _myCall = _prefs.getString("mycall", "");
  _monitorOn = _prefs.getBool("monitor", false);
  _beaconMode = (BeaconMode)_prefs.getUInt("beacon_mode", BEACON_OFF);
  _beaconInterval = _prefs.getUInt("beacon_interval", 0);
  _beaconText = _prefs.getString("beacon_text", "");
  // load UNPROTO (comma-separated list)
  String up = _prefs.getString("unproto", "");
  _unproto.clear();
  if (up.length()) {
    int start = 0;
    while (start < (int)up.length()) {
      int comma = up.indexOf(',', start);
      String part;
      if (comma == -1) { part = up.substring(start); start = up.length(); }
      else { part = up.substring(start, comma); start = comma+1; }
      part.trim();
      if (part.length()) _unproto.push_back(part);
      if (_unproto.size() >= UNPROTO_MAX) break;
    }
  }

  // register commands
  _cmd.registerCommand("HELP", [this](const String &a){ cmdHelp(a); });
  _cmd.registerCommand("?", [this](const String &a){ cmdHelp(a); });
  _cmd.registerCommand("VERSION", [this](const String &a){ cmdVersion(a); });
  _cmd.registerCommand("STATUS", [this](const String &a){ cmdStatus(a); });
  _cmd.registerCommand("FREQ", [this](const String &a){ cmdFreq(a); });
  _cmd.registerCommand("PWR", [this](const String &a){ cmdPwr(a); });
  _cmd.registerCommand("SEND", [this](const String &a){ cmdSend(a); });
  _cmd.registerCommand("RADIOINIT", [this](const String &a){ cmdRadioInit(a); });

  // MYCALL - set/get callsign
  _cmd.registerCommand("MYCALL", [this](const String &a){
    if (a.length() == 0) {
      _io.printf("MYCALL %s\r\n", _myCall.c_str());
      return;
    }
    _myCall = a;
    _prefs.putString("mycall", _myCall);
    _io.printf("OK MYCALL %s\r\n", _myCall.c_str());
  });

  // enable local echo so interactive serial shows typed characters
  _cmd.setLocalEcho(true);

  // MONITOR ON|OFF
  _cmd.registerCommand("MONITOR", [this](const String &a){
    String s = a;
    s.trim(); s.toUpperCase();
    if (s == "ON") {
      _monitorOn = true;
      _prefs.putBool("monitor", true);
      _io.println("OK MONITOR ON");
    } else if (s == "OFF") {
      _monitorOn = false;
      _prefs.putBool("monitor", false);
      _io.println("OK MONITOR OFF");
    } else {
      _io.printf("MONITOR %s\r\n", _monitorOn?"ON":"OFF");
    }
  });

  // MHEARD - list heard stations
  _cmd.registerCommand("MHEARD", [this](const String &a){
    (void)a;
    if (_mheard_count == 0) { _io.println("MHEARD none"); return; }
    for (int i=0;i<_mheard_count;i++) _io.printf("%d: %s\r\n", i+1, _mheard[i].c_str());
  });

  // UNPROTO - show/set digipeat path (comma-separated list)
  _cmd.registerCommand("UNPROTO", [this](const String &a){
    String s = a; s.trim();
    if (s.length() == 0) {
      // show
      if (_unproto.empty()) { _io.println("UNPROTO none"); return; }
      _io.print("UNPROTO: ");
      for (size_t i=0;i<_unproto.size();i++) {
        if (i) _io.print(','); _io.print(_unproto[i]);
      }
      _io.println();
      return;
    }
    String up = s;
    up.trim();
    if (up.equalsIgnoreCase("CLEAR")) {
      _unproto.clear();
      _prefs.putString("unproto", "");
      _io.println("OK UNPROTO cleared");
      return;
    }
    // set: comma separated list
    _unproto.clear();
    int start = 0;
    while (start < (int)up.length() && _unproto.size() < UNPROTO_MAX) {
      int comma = up.indexOf(',', start);
      String part;
      if (comma == -1) { part = up.substring(start); start = up.length(); }
      else { part = up.substring(start, comma); start = comma+1; }
      part.trim();
      if (part.length()) _unproto.push_back(part);
    }
    // persist
    String stored = "";
    for (size_t i=0;i<_unproto.size();i++) { if (i) stored += ","; stored += _unproto[i]; }
    _prefs.putString("unproto", stored);
    _io.println("OK UNPROTO set");
  });

  // BEACON - show/set
  _cmd.registerCommand("BEACON", [this](const String &a){
    String s = a; s.trim();
    if (s.length() == 0) {
      _io.printf("BEACON mode=%d interval=%u text=%s\r\n", (int)_beaconMode, _beaconInterval, _beaconText.c_str());
      return;
    }
    int sp = s.indexOf(' ');
    String cmd = (sp==-1)?s:s.substring(0,sp);
    String rest = (sp==-1)?String(""):s.substring(sp+1);
    cmd.toUpperCase();
    if (cmd == "OFF") {
      _beaconMode = BEACON_OFF;
      _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      _io.println("OK BEACON OFF");
    } else if (cmd == "EVERY") {
      uint32_t v = rest.toInt();
      if (v==0) { _io.println("ERR invalid interval"); return; }
      _beaconMode = BEACON_EVERY; _beaconInterval = v;
      _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      _prefs.putUInt("beacon_interval", _beaconInterval);
      _io.printf("OK BEACON EVERY %u\r\n", _beaconInterval);
    } else if (cmd == "SEND") {
      // manual send: build AX.25 UI to BEACON via UNPROTO digis
      String body = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
      std::vector<uint8_t> payload;
      for (size_t i=0;i<(size_t)body.length();i++) payload.push_back((uint8_t)body[i]);
      std::vector<String> digis;
      for (auto &d : _unproto) digis.push_back(d);
      std::vector<uint8_t> frame = AX25::encodeUIFrame(String("BEACON"), _myCall.length()?_myCall:String("NOCALL"), digis, payload);
      if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
        _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
      } else {
        _radio.send(frame.data(), frame.size());
        _io.println("OK BEACON SEND");
      }
    } else if (cmd == "AFTER") {
      uint32_t v = rest.toInt();
      if (v==0) { _io.println("ERR invalid interval"); return; }
      _beaconMode = BEACON_AFTER; _beaconInterval = v;
      _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      _prefs.putUInt("beacon_interval", _beaconInterval);
      _io.printf("OK BEACON AFTER %u\r\n", _beaconInterval);
    } else if (cmd == "TEXT") {
      _beaconText = rest;
      _prefs.putString("beacon_text", _beaconText);
      _io.printf("OK BEACON TEXT %s\r\n", _beaconText.c_str());
    } else {
      _io.println("ERR BEACON syntax");
    }
  });

  // CONNECT <callsign> - simple stub
  _cmd.registerCommand("CONNECT", [this](const String &a){
    String tgt = a; tgt.trim();
    if (tgt.length() == 0) { _io.println("ERR missing callsign"); return; }
    _connectedTo = tgt;
    if (_mheard_count < MHEARD_MAX) _mheard[_mheard_count++] = tgt;
    _io.printf("CONNECT %s\r\n", tgt.c_str());
    // send an AX.25 UI frame for CONNECT
    std::vector<uint8_t> payload;
    String body = String("[CONNECT]") + _myCall + ">" + tgt;
    for (size_t i=0;i<(size_t)body.length();i++) payload.push_back((uint8_t)body[i]);
    std::vector<uint8_t> frame = AX25::encodeUIFrame(tgt, _myCall.length()?_myCall:String("NOCALL"), payload);
    if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
      _io.printf("ERR CONNECT frame too large (%u bytes)\r\n", (unsigned)frame.size());
    } else {
      _radio.send(frame.data(), frame.size());
    }
  });
  // wire radio RX handler so incoming packets reach TNC
  _radio.setRxHandler([this](const String &from, const String &payload, int rssi){
    this->onPacketReceived(from, payload, rssi);
  });

  _io.println(F("LoRaTNCX ready. Type HELP for commands."));
}

void LoRaTNCX::poll() {
  // poll radio first to surface any received packets to the TNC
  _radio.poll();

  _cmd.poll();

  // beacon handling
  if (_beaconMode != BEACON_OFF && _beaconInterval > 0) {
    uint32_t now = millis();
    // convert interval seconds to ms
    uint32_t iv = _beaconInterval * 1000UL;
    if (_beaconMode == BEACON_EVERY) {
      if ((now - _lastBeaconMs) >= iv) {
        // send beacon as AX.25 UI frame (dest=NOCALL by default)
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload;
        for (size_t i=0;i<(size_t)b.length();i++) payload.push_back((uint8_t)b[i]);
  // build vector<String> digis from _unproto
  std::vector<String> digis;
  for (auto &d : _unproto) digis.push_back(d);
      std::vector<uint8_t> frame = AX25::encodeUIFrame(String("BEACON"), _myCall.length()?_myCall:String("NOCALL"), digis, payload);
      if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
        _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
      } else {
        _radio.send(frame.data(), frame.size());
      }
        _lastBeaconMs = now;
      }
    } else if (_beaconMode == BEACON_AFTER) {
      // send once after interval then disable
      if (_lastBeaconMs == 0) {
        // first-time: set reference
        _lastBeaconMs = now;
      } else if ((now - _lastBeaconMs) >= iv) {
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload;
        for (size_t i=0;i<(size_t)b.length();i++) payload.push_back((uint8_t)b[i]);
        std::vector<String> digis;
        for (auto &d : _unproto) digis.push_back(d);
        std::vector<uint8_t> frame = AX25::encodeUIFrame(String("BEACON"), _myCall.length()?_myCall:String("NOCALL"), digis, payload);
        if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
          _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
        } else {
          _radio.send(frame.data(), frame.size());
        }
        _beaconMode = BEACON_OFF;
        _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      }
    }
  }
}

void LoRaTNCX::addMHeard(const String &callsign) {
  if (callsign.length() == 0) return;
  // avoid duplicates (simple linear scan)
  for (int i=0;i<_mheard_count;i++) if (_mheard[i] == callsign) return;
  if (_mheard_count < MHEARD_MAX) {
    _mheard[_mheard_count++] = callsign;
  } else {
    // rotate
    for (int i=1;i<MHEARD_MAX;i++) _mheard[i-1] = _mheard[i];
    _mheard[MHEARD_MAX-1] = callsign;
  }
}

void LoRaTNCX::onPacketReceived(const String &from, const String &payload, int rssi) {
  // record heard station
  addMHeard(from);
  // optionally print to monitor
  if (_monitorOn) {
    if (from.length()) _io.printf("[%s] ", from.c_str());
    if (payload.length()) _io.printf("%s ", payload.c_str());
    _io.printf("(rssi=%d)\r\n", rssi);
  }
}

// --- Handlers ---
void LoRaTNCX::cmdHelp(const String &args) {
  (void)args;
  _cmd.printHelp();
}

void LoRaTNCX::cmdVersion(const String &args) {
  (void)args;
  _io.println(F("LoRaTNCX 0.1 - minimal TNC-2 compatible command set"));
}

void LoRaTNCX::cmdStatus(const String &args) {
  (void)args;
  _io.print(F("Radio freq (MHz): "));
  _io.println(_radio.getFrequency());
  _io.print(F("Tx power (dBm): "));
  _io.println(_radio.getTxPower());
}

void LoRaTNCX::cmdFreq(const String &args) {
  if (args.length() == 0) {
    _io.print(F("Current frequency: "));
    _io.println(_radio.getFrequency());
    return;
  }
  float f = args.toFloat();
  if (f <= 0) {
    _io.println(F("Invalid frequency. Provide MHz (e.g. 868.0)"));
    return;
  }
  int r = _radio.setFrequency(f);
  if (r == 0) {
    _io.print(F("Frequency set to: "));
    _io.println(f);
  } else {
    _io.print(F("Failed to set frequency (err "));
    _io.print(r);
    _io.println(")");
  }
}

void LoRaTNCX::cmdPwr(const String &args) {
  if (args.length() == 0) {
    _io.print(F("Current tx power: "));
    _io.println(_radio.getTxPower());
    return;
  }
  int p = args.toInt();
  int r = _radio.setTxPower((int8_t)p);
  if (r == 0) {
    _io.print(F("Tx power set to: "));
    _io.println(p);
  } else {
    _io.print(F("Failed to set tx power (err "));
    _io.print(r);
    _io.println(")");
  }
}

void LoRaTNCX::cmdSend(const String &args) {
  if (args.length() == 0) {
    _io.println(F("Usage: SEND text..."));
    return;
  }
  int r = _radio.send((const uint8_t*)args.c_str(), args.length());
  if (r == 0) {
    _io.println(F("Send OK"));
  } else {
    _io.print(F("Send failed: "));
    _io.println(r);
  }
}

void LoRaTNCX::cmdRadioInit(const String &args) {
  (void)args;
  if (_radio.begin(_radio.getFrequency())) {
    _io.println(F("Radio init OK"));
  } else {
    _io.println(F("Radio init FAILED"));
  }
}
