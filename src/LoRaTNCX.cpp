// LoRaTNCX.cpp
#include "LoRaTNCX.h"
#include "AX25.h"
#include <vector>
#include <Preferences.h>

// ============================================================================
// CONSTRUCTOR & INITIALIZATION
// ============================================================================

LoRaTNCX::LoRaTNCX(Stream &io, LoRaRadio &radio)
    : _io(io), _radio(radio), _cmd(io, radio)
{
}

void LoRaTNCX::begin()
{
  // Open preferences and load settings
  _prefs.begin("loratncx", false);
  loadSettings();

  // Initialize runtime state
  _converseBuf = String();
  _converseBufMs = 0;
  _streams[0].state = _l2state;
  _streams[0].connectedTo = _connectedTo;
  _streams[0].tries = _tries;
  _streams[0].lastFrackMs = _lastFrackMs;

  // Register all commands
  registerAllCommands();

  // Enable local echo for interactive use
  _cmd.setLocalEcho(true);

  // Propagate SENDPAC to CommandProcessor
  _cmd.setSendPacChar(_sendpac);

  // Wire radio RX handler
  _radio.setRxHandler([this](const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)
                      { this->onPacketReceived(buf, len, ai, rssi); });

  _io.println(F("LoRaTNCX ready. Type HELP for commands."));
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void LoRaTNCX::poll()
{
  // Command processor polling
  _cmd.poll();

  // PACTIME: flush converse buffer if idle
  if (_converseBufMs != 0 && _cmd.getMode() == CommandProcessor::MODE_CONVERSE)
  {
    uint32_t now = millis();
    if (_pactime == 0)
      _pactime = 1;
    if ((now - _converseBufMs) >= _pactime)
    {
      flushConverseBuffer();
    }
  }

  // Beacon handling
  if (_beaconMode != BEACON_OFF && _beaconInterval > 0)
  {
    uint32_t now = millis();
    uint32_t iv = _beaconInterval * 1000UL;

    if (_beaconMode == BEACON_EVERY)
    {
      if ((now - _lastBeaconMs) >= iv)
      {
        // Build and send beacon
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload(b.begin(), b.end());
        std::vector<String> digis(_unproto.begin(), _unproto.end());
        std::vector<uint8_t> frame = AX25::encodeUIFrame(
            String("BEACON"),
            _myCall.length() ? _myCall : String("NOCALL"),
            digis,
            payload);

        if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(frame.data(), frame.size());
        }
        else
        {
          _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
        }
        _lastBeaconMs = now;
      }
    }
    else if (_beaconMode == BEACON_AFTER)
    {
      if (_lastBeaconMs == 0)
      {
        _lastBeaconMs = now;
      }
      else if ((now - _lastBeaconMs) >= iv)
      {
        // Send once and disable
        String b = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
        std::vector<uint8_t> payload(b.begin(), b.end());
        std::vector<String> digis(_unproto.begin(), _unproto.end());
        std::vector<uint8_t> frame = AX25::encodeUIFrame(
            String("BEACON"),
            _myCall.length() ? _myCall : String("NOCALL"),
            digis,
            payload);

        if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(frame.data(), frame.size());
        }

        _beaconMode = BEACON_OFF;
        _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
      }
    }
  }

  // L2 state machine: handle CONNECTING retries (FRACK)
  if (_l2state == L2_CONNECTING)
  {
    uint32_t now = millis();
    uint32_t frackMs = ((uint32_t)_frack) * 1000UL;
    if (frackMs == 0)
      frackMs = 1000UL;

    if ((now - _lastFrackMs) >= frackMs)
    {
      _tries++;
      if (_tries > _retry)
      {
        _io.println(F("*** retry limit exceeded"));
        _l2state = L2_DISCONNECTED;
        _connectedTo = String();
      }
      else
      {
        // Resend SABM
        std::vector<uint8_t> frame = AX25::encodeControlFrame(
            _connectedTo,
            _myCall.length() ? _myCall : String("NOCALL"),
            AX25::CTL_SABM);
        if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(frame.data(), frame.size());
          _io.printf("RETRY %u: resent SABM to %s\r\n", (unsigned)_tries, _connectedTo.c_str());
        }
        _lastFrackMs = now;
      }
    }
  }
}

// ============================================================================
// SETTINGS PERSISTENCE
// ============================================================================

void LoRaTNCX::loadSettings()
{
  _myCall = _prefs.getString("mycall", "");
  _monitorOn = _prefs.getBool("monitor", false);
  _conok = _prefs.getBool("conok", true);
  _cmsgOn = _prefs.getBool("cmsg", false);
  _cmsgDisc = _prefs.getBool("cmsgdisc", false);
  _ctext = _prefs.getString("ctext", "");
  _beaconMode = (BeaconMode)_prefs.getUInt("beacon_mode", BEACON_OFF);
  _beaconInterval = _prefs.getUInt("beacon_interval", 0);
  _beaconText = _prefs.getString("beacon_text", "");
  _retry = (uint8_t)_prefs.getUInt("retry", 10);
  _frack = (uint8_t)_prefs.getUInt("frack", 8);
  _paclen = (uint16_t)_prefs.getUInt("paclen", 256);
  _pactime = (uint32_t)_prefs.getUInt("pactime", 1000);
  String sp = _prefs.getString("sendpac", String("\n"));
  if (sp.length() > 0)
    _sendpac = sp.charAt(0);
  _crOn = _prefs.getBool("cr", true);

  // Load UNPROTO path
  String up = _prefs.getString("unproto", "");
  _unproto.clear();
  if (up.length())
  {
    int start = 0;
    while (start < (int)up.length())
    {
      int comma = up.indexOf(',', start);
      String part = (comma == -1) ? up.substring(start) : up.substring(start, comma);
      start = (comma == -1) ? up.length() : comma + 1;
      part.trim();
      if (part.length())
        _unproto.push_back(part);
      if (_unproto.size() >= UNPROTO_MAX)
        break;
    }
  }
}

void LoRaTNCX::saveSettings()
{
  _prefs.putString("mycall", _myCall);
  _prefs.putBool("monitor", _monitorOn);
  _prefs.putBool("conok", _conok);
  _prefs.putBool("cmsg", _cmsgOn);
  _prefs.putBool("cmsgdisc", _cmsgDisc);
  _prefs.putString("ctext", _ctext);
  _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
  _prefs.putUInt("beacon_interval", _beaconInterval);
  _prefs.putString("beacon_text", _beaconText);
  _prefs.putUInt("retry", (uint32_t)_retry);
  _prefs.putUInt("frack", (uint32_t)_frack);
  _prefs.putUInt("paclen", (uint32_t)_paclen);
  _prefs.putUInt("pactime", _pactime);
  _prefs.putString("sendpac", String(_sendpac));
  _prefs.putBool("cr", _crOn);

  // Save UNPROTO path
  String stored = "";
  for (size_t i = 0; i < _unproto.size(); i++)
  {
    if (i)
      stored += ",";
    stored += _unproto[i];
  }
  _prefs.putString("unproto", stored);
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void LoRaTNCX::addMHeard(const String &callsign)
{
  if (callsign.length() == 0)
    return;

  // Check for duplicates
  for (int i = 0; i < _mheard_count; i++)
  {
    if (_mheard[i] == callsign)
      return;
  }

  // Add or rotate
  if (_mheard_count < MHEARD_MAX)
  {
    _mheard[_mheard_count++] = callsign;
  }
  else
  {
    for (int i = 1; i < MHEARD_MAX; i++)
    {
      _mheard[i - 1] = _mheard[i];
    }
    _mheard[MHEARD_MAX - 1] = callsign;
  }
}

void LoRaTNCX::onPacketReceived(const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)
{
  // Add to heard list
  if (ai.ok && ai.src.length())
  {
    addMHeard(ai.src);
  }

  // Monitor output
  if (_monitorOn)
  {
    if (ai.ok)
    {
      _io.printf("[%s] ", ai.src.c_str());
    }

    // Extract payload
    size_t payload_len = 0;
    if (ai.ok && ai.header_len < len)
    {
      payload_len = len - ai.header_len;
      if (payload_len > 2)
        payload_len -= 2; // exclude FCS
    }

    if (payload_len > 0)
    {
      _io.printf("%.*s ", (int)payload_len, (const char *)(buf + ai.header_len));
    }
    _io.printf("(rssi=%d)\r\n", rssi);
  }

  // Handle control frames
  if (ai.ok && ai.hasControl)
  {
    uint8_t ctl = ai.control;

    // Incoming SABM (connection request)
    if (ctl == AX25::CTL_SABM)
    {
      if (_l2state != L2_DISCONNECTED && !ai.src.equalsIgnoreCase(_connectedTo))
      {
        _io.printf("*** Ignored SABM from %s (busy)\r\n", ai.src.c_str());
        return;
      }

      if (!_conok)
      {
        // Reject with DM
        auto dm = AX25::encodeControlFrame(ai.src, _myCall.length() ? _myCall : String("NOCALL"), AX25::CTL_DM);
        if (dm.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(dm.data(), dm.size());
        }
        _io.printf("connect request: %s\r\n", ai.src.c_str());
        return;
      }

      // Accept with UA
      auto frame = AX25::encodeControlFrame(ai.src, _myCall.length() ? _myCall : String("NOCALL"), AX25::CTL_UA);
      if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
      {
        _radio.send(frame.data(), frame.size());
      }

      _connectedTo = ai.src;
      _l2state = L2_CONNECTED;
      _tries = 0;
      _io.printf("*** CONNECTED to %s (peer)\r\n", _connectedTo.c_str());
      maybeSendConnectText();
      return;
    }

    // Response to our CONNECT attempt
    if (_l2state == L2_CONNECTING && ai.src.equalsIgnoreCase(_connectedTo))
    {
      if (ctl == AX25::CTL_UA || ctl == AX25::CTL_RR || ctl == AX25::CTL_RNR)
      {
        _l2state = L2_CONNECTED;
        _tries = 0;
        _io.printf("*** CONNECTED to %s\r\n", _connectedTo.c_str());
        maybeSendConnectText();
        return;
      }

      if (ctl == AX25::CTL_DISC)
      {
        _l2state = L2_DISCONNECTED;
        _connectedTo = String();
        _io.println(F("*** DISCONNECTED (peer DISC)"));
        return;
      }
    }

    // Peer disconnect
    if (_l2state == L2_CONNECTED && ai.src.equalsIgnoreCase(_connectedTo) && ctl == AX25::CTL_DISC)
    {
      _l2state = L2_DISCONNECTED;
      _connectedTo = String();
      _io.println(F("*** DISCONNECTED (peer DISC)"));
      return;
    }
  }
  else
  {
    // Non-control frames from target while connecting = connected
    if (_l2state == L2_CONNECTING && ai.ok && ai.src.equalsIgnoreCase(_connectedTo))
    {
      _l2state = L2_CONNECTED;
      _tries = 0;
      _io.printf("*** CONNECTED to %s\r\n", _connectedTo.c_str());
      maybeSendConnectText();
    }
  }
}

void LoRaTNCX::maybeSendConnectText()
{
  if (!_cmsgOn || _ctext.length() == 0 || _connectedTo.length() == 0)
  {
    return;
  }

  // Build and send CTEXT
  std::vector<uint8_t> payload(_ctext.begin(), _ctext.end());
  std::vector<uint8_t> frame = AX25::encodeUIFrame(
      _connectedTo,
      _myCall.length() ? _myCall : String("NOCALL"),
      payload);

  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(frame.data(), frame.size());
    _io.println("OK CTEXT sent");
  }
  else
  {
    _io.printf("ERR CTEXT frame too large (%u bytes)\r\n", (unsigned)frame.size());
  }

  // Auto-disconnect if configured
  if (_cmsgDisc)
  {
    auto df = AX25::encodeControlFrame(
        _connectedTo,
        _myCall.length() ? _myCall : String("NOCALL"),
        AX25::CTL_DISC);
    if (df.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
    {
      _radio.send(df.data(), df.size());
    }
    _l2state = L2_DISCONNECTED;
    _connectedTo = String();
    _io.println(F("*** DISCONNECTED (CMSGDISC)"));
  }
}

// ============================================================================
// CONVERSE MODE
// ============================================================================

void LoRaTNCX::handleConverseLine(const String &line, bool endOfLine)
{
  if (line.length() == 0)
    return;

  for (size_t i = 0; i < (size_t)line.length(); ++i)
  {
    char c = line.charAt(i);

    // SENDPAC character triggers flush
    if ((char)_sendpac == c)
    {
      if (_converseBuf.length() > 0)
      {
        flushConverseBuffer();
      }
      continue;
    }

    // CR handling
    if (_crOn && (c == '\r' || c == '\n'))
    {
      if (_converseBuf.length() > 0)
      {
        _converseBuf += '\r';
        flushConverseBuffer();
      }
      continue;
    }

    // Normal character
    _converseBuf += c;
    if ((size_t)_converseBuf.length() >= (size_t)_paclen)
    {
      flushConverseBuffer();
    }
    _converseBufMs = millis();
  }

  // End of line triggers flush
  if (endOfLine)
  {
    flushConverseBuffer();
  }
}

void LoRaTNCX::flushConverseBuffer()
{
  if (_converseBuf.length() == 0)
    return;

  bool useConnected = (_l2state == L2_CONNECTED && _connectedTo.length() > 0);
  String dest = useConnected ? _connectedTo : String("NOCALL");

  size_t pos = 0;
  while (pos < (size_t)_converseBuf.length())
  {
    size_t remaining = _converseBuf.length() - pos;
    size_t chunk = remaining > _paclen ? _paclen : remaining;
    std::vector<uint8_t> payload;
    for (size_t i = 0; i < chunk; i++)
    {
      payload.push_back((uint8_t)_converseBuf.charAt(pos + i));
    }

    std::vector<uint8_t> frame;
    if (useConnected)
    {
      frame = AX25::encodeUIFrame(dest, _myCall.length() ? _myCall : String("NOCALL"), payload);
    }
    else
    {
      std::vector<String> digis(_unproto.begin(), _unproto.end());
      frame = AX25::encodeUIFrame(dest, _myCall.length() ? _myCall : String("NOCALL"), digis, payload);
    }

    if (frame.size() > (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
    {
      _io.printf("ERR converse frame too large (%u bytes)\r\n", (unsigned)frame.size());
      _converseBuf = String();
      _converseBufMs = 0;
      return;
    }

    _radio.send(frame.data(), frame.size());
    pos += chunk;
  }

  _converseBuf = String();
  _converseBufMs = 0;
  _io.println(F("OK SENDPAC"));
}

// ============================================================================
// COMMAND HANDLERS - Core
// ============================================================================

void LoRaTNCX::cmdHelp(const String &args)
{
  (void)args;
  _cmd.printHelp();
}

void LoRaTNCX::cmdVersion(const String &args)
{
  (void)args;
  _io.println(F("LoRaTNCX 0.1 - TNC-2 compatible command set"));
}

void LoRaTNCX::cmdStatus(const String &args)
{
  (void)args;
  _io.printf("Radio freq (MHz): %.2f\r\n", _radio.getFrequency());
  _io.printf("Tx power (dBm): %d\r\n", _radio.getTxPower());
  _io.printf("SF: %d\r\n", _radio.getSpreadingFactor());
  _io.printf("BW: %ld kHz\r\n", _radio.getBandwidth());
  _io.printf("MYCALL: %s\r\n", _myCall.c_str());
  _io.printf("MONITOR: %s\r\n", _monitorOn ? "ON" : "OFF");
  _io.printf("CONOK: %s\r\n", _conok ? "ON" : "OFF");
  _io.printf("Connection: %s\r\n", _l2state == L2_CONNECTED ? _connectedTo.c_str() : "DISCONNECTED");
}

// ============================================================================
// COMMAND HANDLERS - Radio Control
// ============================================================================

void LoRaTNCX::cmdFreq(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("FREQ %.2f\r\n", _radio.getFrequency());
    return;
  }

  float f = s.toFloat();
  if (f <= 0)
  {
    _io.println(F("ERR invalid frequency"));
    return;
  }

  int r = _radio.setFrequency(f);
  if (r == 0)
  {
    _io.printf("OK FREQ %.2f\r\n", f);
  }
  else
  {
    _io.printf("ERR set frequency: %d\r\n", r);
  }
}

void LoRaTNCX::cmdPwr(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("PWR %d\r\n", _radio.getTxPower());
    return;
  }

  int p = s.toInt();
  int r = _radio.setTxPower((int8_t)p);
  if (r == 0)
  {
    _io.printf("OK PWR %d\r\n", p);
  }
  else
  {
    _io.printf("ERR set power: %d\r\n", r);
  }
}

void LoRaTNCX::cmdSpreadingFactor(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("SF %d\r\n", _radio.getSpreadingFactor());
    return;
  }

  int sf = s.toInt();
  int r = _radio.setSpreadingFactor(sf);
  if (r == 0)
  {
    _io.printf("OK SF %d\r\n", sf);
  }
  else
  {
    _io.printf("ERR set SF: %d\r\n", r);
  }
}

void LoRaTNCX::cmdBandwidth(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("BW %ld\r\n", _radio.getBandwidth());
    return;
  }

  long bw = s.toInt();
  int r = _radio.setBandwidth(bw);
  if (r == 0)
  {
    _io.printf("OK BW %ld\r\n", bw);
  }
  else
  {
    _io.printf("ERR set BW: %d\r\n", r);
  }
}

void LoRaTNCX::cmdCodingRate(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.println(F("CODING not yet implemented"));
    return;
  }
  _io.println(F("ERR CODING not yet implemented"));
}

void LoRaTNCX::cmdRadioInit(const String &args)
{
  (void)args;
  if (_radio.begin(_radio.getFrequency()))
  {
    _io.println(F("OK RADIOINIT"));
  }
  else
  {
    _io.println(F("ERR RADIOINIT failed"));
  }
}

// ============================================================================
// COMMAND HANDLERS - Station & Monitoring
// ============================================================================

void LoRaTNCX::cmdMyCall(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("MYCALL %s\r\n", _myCall.c_str());
    return;
  }

  _myCall = s;
  _prefs.putString("mycall", _myCall);
  _io.printf("OK MYCALL %s\r\n", _myCall.c_str());
}

void LoRaTNCX::cmdMonitor(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MONITOR %s\r\n", _monitorOn ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitorOn = true;
    _prefs.putBool("monitor", true);
    _io.println("OK MONITOR ON");
  }
  else if (s == "OFF")
  {
    _monitorOn = false;
    _prefs.putBool("monitor", false);
    _io.println("OK MONITOR OFF");
  }
  else
  {
    _io.println(F("ERR MONITOR must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMHeard(const String &args)
{
  (void)args;
  if (_mheard_count == 0)
  {
    _io.println("MHEARD none");
    return;
  }

  for (int i = 0; i < _mheard_count; i++)
  {
    _io.printf("%d: %s\r\n", i + 1, _mheard[i].c_str());
  }
}

// ============================================================================
// COMMAND HANDLERS - Connection & Link
// ============================================================================

void LoRaTNCX::cmdConnect(const String &args)
{
  String tgt = args;
  tgt.trim();
  if (tgt.length() == 0)
  {
    _io.println(F("ERR missing callsign"));
    return;
  }

  // Initiate connection
  _connectedTo = tgt;
  _l2state = L2_CONNECTING;
  _tries = 0;
  _lastFrackMs = millis();

  if (_mheard_count < MHEARD_MAX)
  {
    _mheard[_mheard_count++] = tgt;
  }

  _io.printf("CONNECT %s (attempting)\r\n", tgt.c_str());

  // Send SABM
  std::vector<uint8_t> frame = AX25::encodeControlFrame(
      tgt,
      _myCall.length() ? _myCall : String("NOCALL"),
      AX25::CTL_SABM);

  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(frame.data(), frame.size());
  }
  else
  {
    _io.printf("ERR CONNECT frame too large (%u bytes)\r\n", (unsigned)frame.size());
  }
}

void LoRaTNCX::cmdDisconne(const String &args)
{
  (void)args;
  if (_connectedTo.length() == 0)
  {
    _io.println(F("Not connected"));
    return;
  }

  // Send DISC
  std::vector<uint8_t> frame = AX25::encodeControlFrame(
      _connectedTo,
      _myCall.length() ? _myCall : String("NOCALL"),
      AX25::CTL_DISC);
  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(frame.data(), frame.size());
  }

  _l2state = L2_DISCONNECTED;
  _connectedTo = String();
  _io.println(F("*** DISCONNECTED"));
}

void LoRaTNCX::cmdConok(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CONOK %s\r\n", _conok ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _conok = true;
    _prefs.putBool("conok", true);
    _io.println("OK CONOK ON");
  }
  else if (s == "OFF")
  {
    _conok = false;
    _prefs.putBool("conok", false);
    _io.println("OK CONOK OFF");
  }
  else
  {
    _io.println(F("ERR CONOK must be ON or OFF"));
  }
}

void LoRaTNCX::cmdRetry(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("RETRY %u\r\n", (unsigned)_retry);
    return;
  }

  int v = s.toInt();
  if (v < 0 || v > 255)
  {
    _io.println(F("ERR invalid retry value"));
    return;
  }

  _retry = (uint8_t)v;
  _prefs.putUInt("retry", (uint32_t)_retry);
  _io.printf("OK RETRY %u\r\n", (unsigned)_retry);
}

void LoRaTNCX::cmdFrack(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("FRACK %u\r\n", (unsigned)_frack);
    return;
  }

  int v = s.toInt();
  if (v < 0 || v > 255)
  {
    _io.println(F("ERR invalid FRACK value"));
    return;
  }

  _frack = (uint8_t)v;
  _prefs.putUInt("frack", (uint32_t)_frack);
  _io.printf("OK FRACK %u\r\n", (unsigned)_frack);
}

// ============================================================================
// COMMAND HANDLERS - Connect Text
// ============================================================================

void LoRaTNCX::cmdCText(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("CTEXT %s\r\n", _ctext.c_str());
    return;
  }

  _ctext = s;
  _prefs.putString("ctext", _ctext);
  _io.printf("OK CTEXT %s\r\n", _ctext.c_str());
}

void LoRaTNCX::cmdCMsg(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CMSG %s\r\n", _cmsgOn ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _cmsgOn = true;
    _prefs.putBool("cmsg", true);
    _io.println("OK CMSG ON");
  }
  else if (s == "OFF")
  {
    _cmsgOn = false;
    _prefs.putBool("cmsg", false);
    _io.println("OK CMSG OFF");
  }
  else
  {
    _io.println(F("ERR CMSG must be ON or OFF"));
  }
}

void LoRaTNCX::cmdCMsgDisc(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CMSGDISC %s\r\n", _cmsgDisc ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _cmsgDisc = true;
    _prefs.putBool("cmsgdisc", true);
    _io.println("OK CMSGDISC ON");
  }
  else if (s == "OFF")
  {
    _cmsgDisc = false;
    _prefs.putBool("cmsgdisc", false);
    _io.println("OK CMSGDISC OFF");
  }
  else
  {
    _io.println(F("ERR CMSGDISC must be ON or OFF"));
  }
}

// ============================================================================
// COMMAND HANDLERS - Beacon & Unproto
// ============================================================================

void LoRaTNCX::cmdBeacon(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("BEACON mode=%d interval=%u text=%s\r\n",
               (int)_beaconMode, _beaconInterval, _beaconText.c_str());
    return;
  }

  int sp = s.indexOf(' ');
  String cmd = (sp == -1) ? s : s.substring(0, sp);
  String rest = (sp == -1) ? String("") : s.substring(sp + 1);
  cmd.toUpperCase();

  if (cmd == "OFF")
  {
    _beaconMode = BEACON_OFF;
    _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
    _io.println("OK BEACON OFF");
  }
  else if (cmd == "EVERY")
  {
    uint32_t v = rest.toInt();
    if (v == 0)
    {
      _io.println("ERR invalid interval");
      return;
    }
    _beaconMode = BEACON_EVERY;
    _beaconInterval = v;
    _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
    _prefs.putUInt("beacon_interval", _beaconInterval);
    _io.printf("OK BEACON EVERY %u\r\n", _beaconInterval);
  }
  else if (cmd == "AFTER")
  {
    uint32_t v = rest.toInt();
    if (v == 0)
    {
      _io.println("ERR invalid interval");
      return;
    }
    _beaconMode = BEACON_AFTER;
    _beaconInterval = v;
    _prefs.putUInt("beacon_mode", (uint32_t)_beaconMode);
    _prefs.putUInt("beacon_interval", _beaconInterval);
    _io.printf("OK BEACON AFTER %u\r\n", _beaconInterval);
  }
  else if (cmd == "SEND")
  {
    // Manual beacon send
    String body = _beaconText.length() ? _beaconText : (String("BEACON ") + _myCall);
    std::vector<uint8_t> payload(body.begin(), body.end());
    std::vector<String> digis(_unproto.begin(), _unproto.end());
    std::vector<uint8_t> frame = AX25::encodeUIFrame(
        String("BEACON"),
        _myCall.length() ? _myCall : String("NOCALL"),
        digis,
        payload);

    if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
    {
      _radio.send(frame.data(), frame.size());
      _io.println("OK BEACON SEND");
    }
    else
    {
      _io.printf("ERR BEACON frame too large (%u bytes)\r\n", (unsigned)frame.size());
    }
  }
  else
  {
    _io.println("ERR BEACON syntax");
  }
}

void LoRaTNCX::cmdBText(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("BTEXT %s\r\n", _beaconText.c_str());
    return;
  }

  _beaconText = s;
  _prefs.putString("beacon_text", _beaconText);
  _io.printf("OK BTEXT %s\r\n", _beaconText.c_str());
}

void LoRaTNCX::cmdUnproto(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    // Show current path
    if (_unproto.empty())
    {
      _io.println("UNPROTO none");
      return;
    }
    _io.print("UNPROTO: ");
    for (size_t i = 0; i < _unproto.size(); i++)
    {
      if (i)
        _io.print(',');
      _io.print(_unproto[i]);
    }
    _io.println();
    return;
  }

  if (s.equalsIgnoreCase("CLEAR"))
  {
    _unproto.clear();
    _prefs.putString("unproto", "");
    _io.println("OK UNPROTO cleared");
    return;
  }

  // Parse comma-separated list
  _unproto.clear();
  int start = 0;
  while (start < (int)s.length() && _unproto.size() < UNPROTO_MAX)
  {
    int comma = s.indexOf(',', start);
    String part = (comma == -1) ? s.substring(start) : s.substring(start, comma);
    start = (comma == -1) ? s.length() : comma + 1;
    part.trim();
    if (part.length())
      _unproto.push_back(part);
  }

  // Save
  String stored = "";
  for (size_t i = 0; i < _unproto.size(); i++)
  {
    if (i)
      stored += ",";
    stored += _unproto[i];
  }
  _prefs.putString("unproto", stored);
  _io.println("OK UNPROTO set");
}

// ============================================================================
// COMMAND HANDLERS - Converse & Packetization
// ============================================================================

void LoRaTNCX::cmdConverse(const String &args)
{
  (void)args;
  _cmd.setConverseHandler([this](const String &l, bool eol)
                          { this->handleConverseLine(l, eol); });
  _cmd.setMode(CommandProcessor::MODE_CONVERSE);
  _cmd.setLocalEcho(true);
  _io.println(F("<CONVERSE MODE> Press Ctrl-C to exit"));
}

void LoRaTNCX::cmdTrans(const String &args)
{
  (void)args;
  _cmd.setConverseHandler([this](const String &l, bool eol)
                          { this->handleConverseLine(l, eol); });
  _cmd.setMode(CommandProcessor::MODE_TRANSPARENT);
  _cmd.setLocalEcho(false);
  _io.println(F("<TRANSPARENT MODE> Press Ctrl-C three times to exit"));
}

void LoRaTNCX::cmdPaclen(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("PACLEN %u\r\n", (unsigned)_paclen);
    return;
  }

  int v = s.toInt();
  if (v <= 0 || v > RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _io.println(F("ERR invalid PACLEN"));
    return;
  }

  _paclen = (uint16_t)v;
  _prefs.putUInt("paclen", (uint32_t)_paclen);
  _io.printf("OK PACLEN %u\r\n", (unsigned)_paclen);
}

void LoRaTNCX::cmdPactime(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("PACTIME %lu\r\n", (unsigned long)_pactime);
    return;
  }

  long v = s.toInt();
  if (v < 0)
  {
    _io.println(F("ERR invalid PACTIME"));
    return;
  }

  _pactime = (uint32_t)v;
  _prefs.putUInt("pactime", _pactime);
  _io.printf("OK PACTIME %lu\r\n", (unsigned long)_pactime);
}

void LoRaTNCX::cmdSendpac(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    int v = (int)_sendpac;
    _io.printf("SENDPAC %d ('%c')\r\n", v, (v >= 32 && v < 127) ? _sendpac : ' ');
    return;
  }

  if (s.equalsIgnoreCase("CR"))
  {
    _sendpac = '\r';
  }
  else if (s.equalsIgnoreCase("LF") || s.equalsIgnoreCase("NL"))
  {
    _sendpac = '\n';
  }
  else if (s.length() == 1)
  {
    _sendpac = s.charAt(0);
  }
  else
  {
    int v = s.toInt();
    if (v < 0 || v > 255)
    {
      _io.println(F("ERR invalid SENDPAC"));
      return;
    }
    _sendpac = (char)v;
  }

  _prefs.putString("sendpac", String(_sendpac));
  _cmd.setSendPacChar(_sendpac);
  _io.printf("OK SENDPAC %d\r\n", (int)_sendpac);
}

void LoRaTNCX::cmdCR(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CR %s\r\n", _crOn ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _crOn = true;
    _prefs.putBool("cr", true);
    _io.println("OK CR ON");
  }
  else if (s == "OFF")
  {
    _crOn = false;
    _prefs.putBool("cr", false);
    _io.println("OK CR OFF");
  }
  else
  {
    _io.println(F("ERR CR must be ON or OFF"));
  }
}

// ============================================================================
// COMMAND HANDLERS - Utility
// ============================================================================

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
    _io.printf("Send failed: %d\r\n", r);
  }
}

void LoRaTNCX::cmdRestart(const String &args)
{
  (void)args;
  _prefs.end();
  _prefs.begin("loratncx", false);
  loadSettings();
  _io.println(F("OK RESTART"));
}

void LoRaTNCX::cmdReset(const String &args)
{
  (void)args;
  _prefs.clear();

  // Write defaults
  _myCall = String();
  _monitorOn = false;
  _beaconMode = BEACON_OFF;
  _beaconInterval = 0;
  _beaconText = String();
  _unproto.clear();
  _retry = 10;
  _frack = 8;
  _paclen = 256;
  _pactime = 1000;
  _sendpac = '\n';
  _crOn = true;
  _conok = true;
  _cmsgOn = false;
  _cmsgDisc = false;
  _ctext = String();

  saveSettings();
  _io.println(F("OK RESET"));
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void LoRaTNCX::registerAllCommands()
{
  // Core commands
  _cmd.registerCommand("HELP", [this](const String &a)
                       { cmdHelp(a); });
  _cmd.registerCommand("?", [this](const String &a)
                       { cmdHelp(a); });
  _cmd.registerCommand("VERSION", [this](const String &a)
                       { cmdVersion(a); });
  _cmd.registerCommand("STATUS", [this](const String &a)
                       { cmdStatus(a); });

  // Radio control
  _cmd.registerCommand("FREQ", [this](const String &a)
                       { cmdFreq(a); });
  _cmd.registerCommand("FREQUENCY", [this](const String &a)
                       { cmdFreq(a); });
  _cmd.registerCommand("PWR", [this](const String &a)
                       { cmdPwr(a); });
  _cmd.registerCommand("POWER", [this](const String &a)
                       { cmdPwr(a); });
  _cmd.registerCommand("SF", [this](const String &a)
                       { cmdSpreadingFactor(a); });
  _cmd.registerCommand("SPREADING", [this](const String &a)
                       { cmdSpreadingFactor(a); });
  _cmd.registerCommand("BW", [this](const String &a)
                       { cmdBandwidth(a); });
  _cmd.registerCommand("BANDWIDTH", [this](const String &a)
                       { cmdBandwidth(a); });
  _cmd.registerCommand("CODING", [this](const String &a)
                       { cmdCodingRate(a); });
  _cmd.registerCommand("RADIOINIT", [this](const String &a)
                       { cmdRadioInit(a); });
  _cmd.registerCommand("RADIO", [this](const String &a)
                       { cmdRadioInit(a); });

  // Station & monitoring
  _cmd.registerCommand("MYCALL", [this](const String &a)
                       { cmdMyCall(a); });
  _cmd.registerCommand("MY", [this](const String &a)
                       { cmdMyCall(a); });
  _cmd.registerCommand("MONITOR", [this](const String &a)
                       { cmdMonitor(a); });
  _cmd.registerCommand("M", [this](const String &a)
                       { cmdMonitor(a); });
  _cmd.registerCommand("MHEARD", [this](const String &a)
                       { cmdMHeard(a); });

  // Connection & link
  _cmd.registerCommand("CONNECT", [this](const String &a)
                       { cmdConnect(a); });
  _cmd.registerCommand("C", [this](const String &a)
                       { cmdConnect(a); });
  _cmd.registerCommand("DISCONNE", [this](const String &a)
                       { cmdDisconne(a); });
  _cmd.registerCommand("D", [this](const String &a)
                       { cmdDisconne(a); });
  _cmd.registerCommand("CONOK", [this](const String &a)
                       { cmdConok(a); });
  _cmd.registerCommand("CONO", [this](const String &a)
                       { cmdConok(a); });
  _cmd.registerCommand("RETRY", [this](const String &a)
                       { cmdRetry(a); });
  _cmd.registerCommand("RE", [this](const String &a)
                       { cmdRetry(a); });
  _cmd.registerCommand("FRACK", [this](const String &a)
                       { cmdFrack(a); });
  _cmd.registerCommand("FR", [this](const String &a)
                       { cmdFrack(a); });

  // Connect text
  _cmd.registerCommand("CTEXT", [this](const String &a)
                       { cmdCText(a); });
  _cmd.registerCommand("CMSG", [this](const String &a)
                       { cmdCMsg(a); });
  _cmd.registerCommand("CMS", [this](const String &a)
                       { cmdCMsg(a); });
  _cmd.registerCommand("CMSGDISC", [this](const String &a)
                       { cmdCMsgDisc(a); });
  _cmd.registerCommand("CMSGD", [this](const String &a)
                       { cmdCMsgDisc(a); });

  // Beacon & unproto
  _cmd.registerCommand("BEACON", [this](const String &a)
                       { cmdBeacon(a); });
  _cmd.registerCommand("B", [this](const String &a)
                       { cmdBeacon(a); });
  _cmd.registerCommand("BTEXT", [this](const String &a)
                       { cmdBText(a); });
  _cmd.registerCommand("BT", [this](const String &a)
                       { cmdBText(a); });
  _cmd.registerCommand("UNPROTO", [this](const String &a)
                       { cmdUnproto(a); });
  _cmd.registerCommand("U", [this](const String &a)
                       { cmdUnproto(a); });

  // Converse & packetization
  _cmd.registerCommand("CONVERSE", [this](const String &a)
                       { cmdConverse(a); });
  _cmd.registerCommand("CONV", [this](const String &a)
                       { cmdConverse(a); });
  _cmd.registerCommand("K", [this](const String &a)
                       { cmdConverse(a); });
  _cmd.registerCommand("TRANS", [this](const String &a)
                       { cmdTrans(a); });
  _cmd.registerCommand("T", [this](const String &a)
                       { cmdTrans(a); });
  _cmd.registerCommand("PACLEN", [this](const String &a)
                       { cmdPaclen(a); });
  _cmd.registerCommand("P", [this](const String &a)
                       { cmdPaclen(a); });
  _cmd.registerCommand("PACTIME", [this](const String &a)
                       { cmdPactime(a); });
  _cmd.registerCommand("PACT", [this](const String &a)
                       { cmdPactime(a); });
  _cmd.registerCommand("SENDPAC", [this](const String &a)
                       { cmdSendpac(a); });
  _cmd.registerCommand("SE", [this](const String &a)
                       { cmdSendpac(a); });
  _cmd.registerCommand("CR", [this](const String &a)
                       { cmdCR(a); });

  // Utility
  _cmd.registerCommand("SEND", [this](const String &a)
                       { cmdSend(a); });
  _cmd.registerCommand("RESTART", [this](const String &a)
                       { cmdRestart(a); });
  _cmd.registerCommand("RESET", [this](const String &a)
                       { cmdReset(a); });
}
