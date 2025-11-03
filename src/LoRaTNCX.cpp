// LoRaTNCX.cpp
#include "LoRaTNCX.h"
#include "AX25.h"
#include <vector>
#include <Preferences.h>

LoRaTNCX::LoRaTNCX(Stream &io, LoRaRadio &radio)
    : _io(io), _radio(radio), _cmd(io, radio)
{
}

void LoRaTNCX::begin()
{
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
  if (up.length())
  {
    int start = 0;
    while (start < (int)up.length())
    {
      int comma = up.indexOf(',', start);
      String part;
      if (comma == -1)
      {
        part = up.substring(start);
        start = up.length();
      }
      else
      {
        part = up.substring(start, comma);
        start = comma + 1;
      }
      part.trim();
      if (part.length())
        _unproto.push_back(part);
      if (_unproto.size() >= UNPROTO_MAX)
        break;
    }
  }

  // load retry parameter
  _retry = (uint8_t)_prefs.getUInt("retry", 10);
  // load frack parameter
  _frack = (uint8_t)_prefs.getUInt("frack", 8);
  // initialize stream 0 from single-stream fields
  _streams[0].state = _l2state;
  _streams[0].connectedTo = _connectedTo;
  _streams[0].tries = _tries;
  _streams[0].lastFrackMs = _lastFrackMs;

  // register commands
  _cmd.registerCommand("HELP", [this](const String &a)
                       { cmdHelp(a); });
  _cmd.registerCommand("?", [this](const String &a)
                       { cmdHelp(a); });
  _cmd.registerCommand("VERSION", [this](const String &a)
                       { cmdVersion(a); });
  _cmd.registerCommand("STATUS", [this](const String &a)
                       { cmdStatus(a); });
  _cmd.registerCommand("FREQ", [this](const String &a)
                       { cmdFreq(a); });
  _cmd.registerCommand("PWR", [this](const String &a)
                       { cmdPwr(a); });
  _cmd.registerCommand("SEND", [this](const String &a)
                       { cmdSend(a); });
  _cmd.registerCommand("RADIOINIT", [this](const String &a)
                       { cmdRadioInit(a); });

  // SF - get/set spreading factor
  _cmd.registerCommand("SF", [this](const String &a)
                       {
    String s = a; s.trim();
    if (s.length() == 0) { _io.printf("SF %d\r\n", _radio.getSpreadingFactor()); return; }
    int sf = s.toInt();
    int r = _radio.setSpreadingFactor(sf);
    if (r == 0) _io.printf("OK SF %d\r\n", sf); else _io.printf("ERR set SF %d -> %d\r\n", sf, r);
  });

  // BW - get/set bandwidth (kHz)
  _cmd.registerCommand("BW", [this](const String &a)
                       {
    String s = a; s.trim();
    if (s.length() == 0) { _io.printf("BW %ld\r\n", _radio.getBandwidth()); return; }
    long bw = s.toInt();
    int r = _radio.setBandwidth(bw);
    if (r == 0) _io.printf("OK BW %ld\r\n", bw); else _io.printf("ERR set BW %ld -> %d\r\n", bw, r);
  });

  // DISCONNE - disconnect
  _cmd.registerCommand("DISCONNE", [this](const String &a)
                       { cmdDisconne(a); });

  // RESTART - reload persisted settings
  _cmd.registerCommand("RESTART", [this](const String &a)
                       { cmdRestart(a); });

  // RESET - clear persisted settings and re-init
  _cmd.registerCommand("RESET", [this](const String &a)
                       { cmdReset(a); });

  // RETRY - get/set retry count
  _cmd.registerCommand("RETRY", [this](const String &a)
                       { cmdRetry(a); });

  // FRACK - get/set frame ack timeout (seconds)
  _cmd.registerCommand("FRACK", [this](const String &a)
                       { cmdFrack(a); });

  // MYCALL - set/get callsign
  _cmd.registerCommand("MYCALL", [this](const String &a)
                       {
    if (a.length() == 0) {
      _io.printf("MYCALL %s\r\n", _myCall.c_str());
      return;
    }
    _myCall = a;
    _prefs.putString("mycall", _myCall);
    _io.printf("OK MYCALL %s\r\n", _myCall.c_str()); });

  // enable local echo so interactive serial shows typed characters
  _cmd.setLocalEcho(true);

  // MONITOR ON|OFF
  _cmd.registerCommand("MONITOR", [this](const String &a)
                       {
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
    } });

  // MHEARD - list heard stations
  _cmd.registerCommand("MHEARD", [this](const String &a)
                       {
    (void)a;
    if (_mheard_count == 0) { _io.println("MHEARD none"); return; }
    for (int i=0;i<_mheard_count;i++) _io.printf("%d: %s\r\n", i+1, _mheard[i].c_str()); });

  // UNPROTO - show/set digipeat path (comma-separated list)
  _cmd.registerCommand("UNPROTO", [this](const String &a)
                       {
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
    _io.println("OK UNPROTO set"); });

  // BEACON - show/set
  _cmd.registerCommand("BEACON", [this](const String &a)
                       {
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
    } });

  // CONNECT <callsign> - initiate connect state machine (simple L2 approximation)
  _cmd.registerCommand("CONNECT", [this](const String &a)
                       {
    String tgt = a; tgt.trim();
    if (tgt.length() == 0) { _io.println("ERR missing callsign"); return; }
    // initiate connecting
    _connectedTo = tgt;
    _l2state = L2_CONNECTING;
    _tries = 0;
    _lastFrackMs = millis();
    if (_mheard_count < MHEARD_MAX) _mheard[_mheard_count++] = tgt;
    _io.printf("CONNECT %s (attempting)\r\n", tgt.c_str());
    // send an AX.25 SABM control frame to initiate link (L2)
    std::vector<uint8_t> frame = AX25::encodeControlFrame(tgt, _myCall.length()?_myCall:String("NOCALL"), AX25::CTL_SABM);
    if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
      _io.printf("ERR CONNECT frame too large (%u bytes)\r\n", (unsigned)frame.size());
    } else {
      _radio.send(frame.data(), frame.size());
    }
  });
  // wire radio RX handler so incoming packets reach TNC
  _radio.setRxHandler([this](const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)
                      { this->onPacketReceived(buf, len, ai, rssi); });

  _io.println(F("LoRaTNCX ready. Type HELP for commands."));
}

void LoRaTNCX::poll()
{
  // Radio polling now runs in separate FreeRTOS task (started in begin())
  // No need to call _radio.poll() here anymore

  _cmd.poll();

  // beacon handling
  if (_beaconMode != BEACON_OFF && _beaconInterval > 0)
  {
    uint32_t now = millis();
    // convert interval seconds to ms
    uint32_t iv = _beaconInterval * 1000UL;
    if (_beaconMode == BEACON_EVERY)
    {
      if ((now - _lastBeaconMs) >= iv)
      {
        // send beacon as AX.25 UI frame (dest=NOCALL by default)
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload;
        for (size_t i = 0; i < (size_t)b.length(); i++)
          payload.push_back((uint8_t)b[i]);
        // build vector<String> digis from _unproto
        std::vector<String> digis;
        for (auto &d : _unproto)
          digis.push_back(d);
        std::vector<uint8_t> frame = AX25::encodeUIFrame(String("BEACON"), _myCall.length() ? _myCall : String("NOCALL"), digis, payload);
        if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
        }
        else
        {
          _radio.send(frame.data(), frame.size());
        }
        _lastBeaconMs = now;
      }
    }
    else if (_beaconMode == BEACON_AFTER)
    {
      // send once after interval then disable
      if (_lastBeaconMs == 0)
      {
        // first-time: set reference
        _lastBeaconMs = now;
      }
      else if ((now - _lastBeaconMs) >= iv)
      {
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload;
        for (size_t i = 0; i < (size_t)b.length(); i++)
          payload.push_back((uint8_t)b[i]);
        std::vector<String> digis;
        for (auto &d : _unproto)
          digis.push_back(d);
        std::vector<uint8_t> frame = AX25::encodeUIFrame(String("BEACON"), _myCall.length() ? _myCall : String("NOCALL"), digis, payload);
        if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
        }
        else
        {
          _radio.send(frame.data(), frame.size());
        }
        _beaconMode = BEACON_OFF;
        _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      }
    }
  }

  // simple L2 state machine: handle CONNECTING retries (FRACK)
  if (_l2state == L2_CONNECTING) {
    uint32_t now = millis();
    uint32_t frackMs = ((uint32_t)_frack) * 1000UL;
    if (frackMs == 0) frackMs = 1000UL; // guard
    if ((now - _lastFrackMs) >= frackMs) {
      // timeout: retry or fail
      _tries++;
      if (_tries > _retry) {
        _io.println(F("*** retry limit exceeded"));
        _l2state = L2_DISCONNECTED;
        _connectedTo = String();
      } else {
        // resend SABM control frame
        std::vector<uint8_t> frame = AX25::encodeControlFrame(_connectedTo, _myCall.length()?_myCall:String("NOCALL"), AX25::CTL_SABM);
        if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
          _radio.send(frame.data(), frame.size());
          _io.printf("RETRY %u: resent SABM to %s\r\n", (unsigned)_tries, _connectedTo.c_str());
        } else {
          _io.println(F("ERR CONNECT frame too large to retry"));
        }
        _lastFrackMs = now;
      }
    }
  }
}

void LoRaTNCX::addMHeard(const String &callsign)
{
  if (callsign.length() == 0)
    return;
  // avoid duplicates (simple linear scan)
  for (int i = 0; i < _mheard_count; i++)
    if (_mheard[i] == callsign)
      return;
  if (_mheard_count < MHEARD_MAX)
  {
    _mheard[_mheard_count++] = callsign;
  }
  else
  {
    // rotate
    for (int i = 1; i < MHEARD_MAX; i++)
      _mheard[i - 1] = _mheard[i];
    _mheard[MHEARD_MAX - 1] = callsign;
  }
}

void LoRaTNCX::onPacketReceived(const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)
{
  // record heard station (if parsed)
  if (ai.ok && ai.src.length())
    addMHeard(ai.src);

  // Optionally print to monitor: include header info and payload when available
  if (_monitorOn)
  {
    if (ai.ok)
      _io.printf("[%s] ", ai.src.c_str());
    // extract payload portion (if any)
    size_t payload_len = 0;
    if (ai.ok && ai.header_len < len) {
      payload_len = len - ai.header_len;
      // exclude trailing FCS if present
      if (payload_len > 2) payload_len -= 2;
    }
    if (payload_len > 0)
      _io.printf("%.*s ", (int)payload_len, (const char *)(buf + ai.header_len));
    _io.printf("(rssi=%d)\r\n", rssi);
  }

  // If this is a control frame (no PID/payload, but has control), inspect control byte
  if (ai.ok && ai.hasControl) {
    uint8_t ctl = ai.control;
    _io.printf("(control=0x%02X)\r\n", ctl);
    
    // Handle incoming SABM from any station (connection request)
    // This allows a remote station to initiate a connection even if we're DISCONNECTED
    if (ctl == AX25::CTL_SABM) {
      // If we're already connecting to someone else or connected to someone else, ignore
      if (_l2state != L2_DISCONNECTED && !ai.src.equalsIgnoreCase(_connectedTo)) {
        _io.printf("*** Ignored SABM from %s (busy)\r\n", ai.src.c_str());
        return;
      }
      // send UA back
      auto frame = AX25::encodeControlFrame(ai.src, _myCall.length()? _myCall : String("NOCALL"), AX25::CTL_UA);
      if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
        _io.println(F("Sending UA response"));
        _radio.send(frame.data(), frame.size());
      }
      // adopt remote as connected
      _connectedTo = ai.src;
      _l2state = L2_CONNECTED;
      _tries = 0;
      _io.printf("*** CONNECTED to %s (peer)\r\n", _connectedTo.c_str());
      return;
    }
    
    // If we are CONNECTING and we receive UA or any frame from target, finish connect
    if (_l2state == L2_CONNECTING && ai.src.equalsIgnoreCase(_connectedTo)) {
      if (ctl == AX25::CTL_UA || ctl == AX25::CTL_RR || ctl == AX25::CTL_RNR) {
        _l2state = L2_CONNECTED;
        _tries = 0;
        _io.printf("*** CONNECTED to %s\r\n", _connectedTo.c_str());
        return;
      }

      // If remote sent DISC, consider disconnected
      if (ctl == AX25::CTL_DISC) {
        _l2state = L2_DISCONNECTED;
        _connectedTo = String();
        _io.println(F("*** DISCONNECTED (peer DISC)"));
        return;
      }
    }

    // If we are CONNECTED and receive DISC from peer, clear state and notify
    if (_l2state == L2_CONNECTED && ai.src.equalsIgnoreCase(_connectedTo) && ai.control == AX25::CTL_DISC) {
      _l2state = L2_DISCONNECTED;
      _connectedTo = String();
      _io.println(F("*** DISCONNECTED (peer DISC)"));
      return;
    }
  } else {
    // Non-control frames: if connecting and we hear payload from target, consider connected
    if (_l2state == L2_CONNECTING && ai.ok && ai.src.equalsIgnoreCase(_connectedTo)) {
      _l2state = L2_CONNECTED;
      _tries = 0;
      _io.printf("*** CONNECTED to %s\r\n", _connectedTo.c_str());
      return;
    }
  }
}

// --- Handlers ---
void LoRaTNCX::cmdHelp(const String &args)
{
  (void)args;
  _cmd.printHelp();
}

void LoRaTNCX::cmdVersion(const String &args)
{
  (void)args;
  _io.println(F("LoRaTNCX 0.1 - minimal TNC-2 compatible command set"));
}

void LoRaTNCX::cmdStatus(const String &args)
{
  (void)args;
  _io.print(F("Radio freq (MHz): "));
  _io.println(_radio.getFrequency());
  _io.print(F("Tx power (dBm): "));
  _io.println(_radio.getTxPower());
}

void LoRaTNCX::cmdFreq(const String &args)
{
  if (args.length() == 0)
  {
    _io.print(F("Current frequency: "));
    _io.println(_radio.getFrequency());
    return;
  }
  float f = args.toFloat();
  if (f <= 0)
  {
    _io.println(F("Invalid frequency. Provide MHz (e.g. 868.0)"));
    return;
  }
  int r = _radio.setFrequency(f);
  if (r == 0)
  {
    _io.print(F("Frequency set to: "));
    _io.println(f);
  }
  else
  {
    _io.print(F("Failed to set frequency (err "));
    _io.print(r);
    _io.println(")");
  }
}

void LoRaTNCX::cmdPwr(const String &args)
{
  if (args.length() == 0)
  {
    _io.print(F("Current tx power: "));
    _io.println(_radio.getTxPower());
    return;
  }
  int p = args.toInt();
  int r = _radio.setTxPower((int8_t)p);
  if (r == 0)
  {
    _io.print(F("Tx power set to: "));
    _io.println(p);
  }
  else
  {
    _io.print(F("Failed to set tx power (err "));
    _io.print(r);
    _io.println(")");
  }
}

void LoRaTNCX::cmdSend(const String &args)
{
  if (args.length() == 0)
  {
    _io.println(F("Usage: SEND text..."));
    return;
  }
  int r = _radio.send((const uint8_t *)args.c_str(), args.length());
  if (r == 0)
  {
    _io.println(F("Send OK"));
  }
  else
  {
    _io.print(F("Send failed: "));
    _io.println(r);
  }
}

void LoRaTNCX::cmdRadioInit(const String &args)
{
  (void)args;
  if (_radio.begin(_radio.getFrequency()))
  {
    _io.println(F("Radio init OK"));
  }
  else
  {
    _io.println(F("Radio init FAILED"));
  }
}

void LoRaTNCX::cmdDisconne(const String &args)
{
  (void)args;
  if (_connectedTo.length() == 0) {
    _io.println(F("Not connected"));
    return;
  }
  // send a DISC control frame to peer
  std::vector<uint8_t> frame = AX25::encodeControlFrame(_connectedTo, _myCall.length()?_myCall:String("NOCALL"), AX25::CTL_DISC);
  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
    _radio.send(frame.data(), frame.size());
  }
  // clear L2 state
  _l2state = L2_DISCONNECTED;
  _connectedTo = String();
  _io.println(F("*** DISCONNECTED"));
}

void LoRaTNCX::cmdRestart(const String &args)
{
  (void)args;
  // re-open prefs and reload persisted settings
  _prefs.end();
  _prefs.begin("loratncx", false);
  _myCall = _prefs.getString("mycall", "");
  _monitorOn = _prefs.getBool("monitor", false);
  _beaconMode = (BeaconMode)_prefs.getUInt("beacon_mode", BEACON_OFF);
  _beaconInterval = _prefs.getUInt("beacon_interval", 0);
  _beaconText = _prefs.getString("beacon_text", "");
  // reload unproto
  _unproto.clear();
  String up = _prefs.getString("unproto", "");
  if (up.length()) {
    int start = 0;
    while (start < (int)up.length()) {
      int comma = up.indexOf(',', start);
      String part;
      if (comma == -1) { part = up.substring(start); start = up.length(); }
      else { part = up.substring(start, comma); start = comma + 1; }
      part.trim(); if (part.length()) _unproto.push_back(part);
      if (_unproto.size() >= UNPROTO_MAX) break;
    }
  }
  // reload retry
  _retry = (uint8_t)_prefs.getUInt("retry", 10);
  // reload frack
  _frack = (uint8_t)_prefs.getUInt("frack", 8);
  _io.println(F("OK RESTART"));
}

void LoRaTNCX::cmdReset(const String &args)
{
  (void)args;
  // clear persisted preferences
  _prefs.clear();
  // write defaults explicitly
  _prefs.putString("mycall", "");
  _prefs.putBool("monitor", false);
  _prefs.putUInt("beacon_mode", (uint32_t)BEACON_OFF);
  _prefs.putUInt("beacon_interval", 0);
  _prefs.putString("beacon_text", "");
  _prefs.putString("unproto", "");
  _prefs.putUInt("retry", 10);
  _prefs.putUInt("frack", 8);
  // apply to RAM
  _myCall = String();
  _monitorOn = false;
  _beaconMode = BEACON_OFF;
  _beaconInterval = 0;
  _beaconText = String();
  _unproto.clear();
  _retry = 10;
  _io.println(F("OK RESET"));
}

void LoRaTNCX::cmdRetry(const String &args)
{
  String s = args; s.trim();
  if (s.length() == 0) {
    _io.printf("RETRY %u\r\n", (unsigned)_retry);
    return;
  }
  int v = s.toInt();
  if (v < 0 || v > 255) { _io.println(F("ERR invalid retry value")); return; }
  _retry = (uint8_t)v;
  _prefs.putUInt("retry", (uint32_t)_retry);
  _io.printf("OK RETRY %u\r\n", (unsigned)_retry);
}

void LoRaTNCX::cmdFrack(const String &args)
{
  String s = args; s.trim();
  if (s.length() == 0) {
    _io.printf("FRACK %u\r\n", (unsigned)_frack);
    return;
  }
  int v = s.toInt();
  if (v < 0 || v > 255) { _io.println(F("ERR invalid FRACK value")); return; }
  _frack = (uint8_t)v;
  _prefs.putUInt("frack", (uint32_t)_frack);
  _io.printf("OK FRACK %u\r\n", (unsigned)_frack);
}
