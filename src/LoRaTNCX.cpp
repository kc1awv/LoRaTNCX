// LoRaTNCX.cpp
#include "LoRaTNCX.h"
#include "AX25.h"
#include <vector>
#include <Preferences.h>

// ============================================================================
// CONSTRUCTOR & INITIALIZATION
// ============================================================================

LoRaTNCX::LoRaTNCX(Stream &io, LoRaRadio &radio)
    : _io(io), _radio(radio), _cmd(io)
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

  // Set local echo from loaded settings
  _cmd.setLocalEcho(_terminal.echo);

  // Propagate SENDPAC to CommandProcessor
  _cmd.setSendPacChar(_packet.sendpac);

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
    if (_packet.pactime == 0)
      _packet.pactime = 1;
    if ((now - _converseBufMs) >= _packet.pactime)
    {
      flushConverseBuffer();
    }
  }

  // Beacon handling
  if (_beacon.mode != BEACON_OFF && _beacon.interval > 0)
  {
    uint32_t now = millis();
    uint32_t iv = _beacon.interval * 1000UL;

    if (_beacon.mode == BEACON_EVERY)
    {
      if ((now - _beacon.lastMs) >= iv)
      {
        // Build and send beacon
        String b = _beacon.text.length() ? _beacon.text : (String("BEACON ") + _station.myCall);
        std::vector<uint8_t> payload(b.begin(), b.end());
        std::vector<String> digis(_unproto.begin(), _unproto.end());
        std::vector<uint8_t> frame = AX25::encodeUIFrame(
            String("BEACON"),
            _station.myCall.length() ? _station.myCall : String("NOCALL"),
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
        _beacon.lastMs = now;
      }
    }
    else if (_beacon.mode == BEACON_AFTER)
    {
      if (_beacon.lastMs == 0)
      {
        _beacon.lastMs = now;
      }
      else if ((now - _beacon.lastMs) >= iv)
      {
        // Send once and disable
        String b = _beacon.text.length() ? _beacon.text : (String("BEACON ") + _station.myCall);
        std::vector<uint8_t> payload(b.begin(), b.end());
        std::vector<String> digis(_unproto.begin(), _unproto.end());
        std::vector<uint8_t> frame = AX25::encodeUIFrame(
            String("BEACON"),
            _station.myCall.length() ? _station.myCall : String("NOCALL"),
            digis,
            payload);

        if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(frame.data(), frame.size());
        }

        _beacon.mode = BEACON_OFF;
        _prefs.putUInt("beacon_mode", (uint32_t)_beacon.mode);
      }
    }
  }

  // HID: auto-ID after digipeating (every 9.5 minutes = 570000 ms)
  if (_digi.hid && _digi.lastHidMs != 0)
  {
    uint32_t now = millis();
    uint32_t hidInterval = 570000UL; // 9.5 minutes
    if ((now - _digi.lastHidMs) >= hidInterval)
    {
      sendId();
      _digi.lastHidMs = now;
    }
  }

  // L2 state machine: handle CONNECTING retries (FRACK)
  if (_l2state == L2_CONNECTING)
  {
    uint32_t now = millis();
    uint32_t frackMs = ((uint32_t)_connection.frack) * 1000UL;
    if (frackMs == 0)
      frackMs = 1000UL;

    if ((now - _lastFrackMs) >= frackMs)
    {
      _tries++;
      if (_tries > _connection.retry)
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
            _station.myCall.length() ? _station.myCall : String("NOCALL"),
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
  _station.myCall = _prefs.getString("mycall", "");
  _station.myAlias = _prefs.getString("myalias", "");
  _terminal.echo = _prefs.getBool("echo", true);
  _monitor.enabled = _prefs.getBool("monitor", false);
  _monitor.mstamp = _prefs.getBool("mstamp", false);
  _monitor.mall = _prefs.getBool("mall", true);
  _monitor.mcom = _prefs.getBool("mcom", false);
  _monitor.mcon = _prefs.getBool("mcon", false);
  _monitor.mrpt = _prefs.getBool("mrpt", true);
  _digi.enabled = _prefs.getBool("digipeat", true);
  _digi.hid = _prefs.getBool("hid", true);
  _connection.conok = _prefs.getBool("conok", true);
  _connection.cmsgOn = _prefs.getBool("cmsg", false);
  _connection.cmsgDisc = _prefs.getBool("cmsgdisc", false);
  _connection.ctext = _prefs.getString("ctext", "");
  _beacon.mode = (BeaconMode)_prefs.getUInt("beacon_mode", BEACON_OFF);
  _beacon.interval = _prefs.getUInt("beacon_interval", 0);
  _beacon.text = _prefs.getString("beacon_text", "");
  _connection.retry = (uint8_t)_prefs.getUInt("retry", 10);
  _connection.frack = (uint8_t)_prefs.getUInt("frack", 8);
  _packet.paclen = (uint16_t)_prefs.getUInt("paclen", 256);
  _packet.pactime = (uint32_t)_prefs.getUInt("pactime", 1000);
  String sp = _prefs.getString("sendpac", String("\n"));
  if (sp.length() > 0)
    _packet.sendpac = sp.charAt(0);
  _packet.cr = _prefs.getBool("cr", true);
  _conmode.mode = (ConMode)_prefs.getUInt("conmode", CONMODE_CONVERSE);
  _conmode.newmode = _prefs.getBool("newmode", false);
  _conmode.nomode = _prefs.getBool("nomode", false);
  _monitor.trace = _prefs.getBool("trace", false);
  _protocol.flow = _prefs.getBool("flow", true);
  _protocol.passall = _prefs.getBool("passall", false);
  _protocol.resptime = (uint8_t)_prefs.getUInt("resptime", 0);
  _packet.cpactime = _prefs.getBool("cpactime", false);
  _datetime.constamp = _prefs.getBool("constamp", false);
  _datetime.daystamp = _prefs.getBool("daystamp", false);
  _datetime.dayusa = _prefs.getBool("dayusa", true);
  _datetime.value = _prefs.getString("datetime", "");
  _location.mode = (LocationMode)_prefs.getUInt("loc_mode", LOCATION_OFF);
  _location.interval = _prefs.getUInt("loc_interval", 0);
  _location.lpath = _prefs.getString("lpath", "GPS");
  _location.ltext = _prefs.getString("ltext", "");
  _location.ltmon = (uint8_t)_prefs.getUInt("ltmon", 0);

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

  // Load EPATH
  String ep = _prefs.getString("epath", "");
  _protocol.epath.clear();
  if (ep.length())
  {
    int start = 0;
    while (start < (int)ep.length())
    {
      int comma = ep.indexOf(',', start);
      String part = (comma == -1) ? ep.substring(start) : ep.substring(start, comma);
      start = (comma == -1) ? ep.length() : comma + 1;
      part.trim();
      if (part.length())
        _protocol.epath.push_back(part);
    }
  }

  // Load KISS
  _kissEnabled = _prefs.getBool("kiss", false);
}

void LoRaTNCX::saveSettings()
{
  _prefs.putString("mycall", _station.myCall);
  _prefs.putString("myalias", _station.myAlias);
  _prefs.putBool("echo", _terminal.echo);
  _prefs.putBool("monitor", _monitor.enabled);
  _prefs.putBool("mstamp", _monitor.mstamp);
  _prefs.putBool("mall", _monitor.mall);
  _prefs.putBool("mcom", _monitor.mcom);
  _prefs.putBool("mcon", _monitor.mcon);
  _prefs.putBool("mrpt", _monitor.mrpt);
  _prefs.putBool("digipeat", _digi.enabled);
  _prefs.putBool("hid", _digi.hid);
  _prefs.putBool("conok", _connection.conok);
  _prefs.putBool("cmsg", _connection.cmsgOn);
  _prefs.putBool("cmsgdisc", _connection.cmsgDisc);
  _prefs.putString("ctext", _connection.ctext);
  _prefs.putUInt("beacon_mode", (uint32_t)_beacon.mode);
  _prefs.putUInt("beacon_interval", _beacon.interval);
  _prefs.putString("beacon_text", _beacon.text);
  _prefs.putUInt("retry", (uint32_t)_connection.retry);
  _prefs.putUInt("frack", (uint32_t)_connection.frack);
  _prefs.putUInt("paclen", (uint32_t)_packet.paclen);
  _prefs.putUInt("pactime", _packet.pactime);
  _prefs.putString("sendpac", String(_packet.sendpac));
  _prefs.putBool("cr", _packet.cr);
  _prefs.putUInt("conmode", (uint32_t)_conmode.mode);
  _prefs.putBool("newmode", _conmode.newmode);
  _prefs.putBool("nomode", _conmode.nomode);
  _prefs.putBool("trace", _monitor.trace);
  _prefs.putBool("flow", _protocol.flow);
  _prefs.putBool("passall", _protocol.passall);
  _prefs.putUInt("resptime", (uint32_t)_protocol.resptime);
  _prefs.putBool("cpactime", _packet.cpactime);
  _prefs.putBool("constamp", _datetime.constamp);
  _prefs.putBool("daystamp", _datetime.daystamp);
  _prefs.putBool("dayusa", _datetime.dayusa);
  _prefs.putString("datetime", _datetime.value);
  _prefs.putUInt("loc_mode", (uint32_t)_location.mode);
  _prefs.putUInt("loc_interval", _location.interval);
  _prefs.putString("lpath", _location.lpath);
  _prefs.putString("ltext", _location.ltext);
  _prefs.putUInt("ltmon", (uint32_t)_location.ltmon);

  // Save UNPROTO path
  String stored = "";
  for (size_t i = 0; i < _unproto.size(); i++)
  {
    if (i)
      stored += ",";
    stored += _unproto[i];
  }
  _prefs.putString("unproto", stored);

  // Save EPATH
  String epathStored = "";
  for (size_t i = 0; i < _protocol.epath.size(); i++)
  {
    if (i)
      epathStored += ",";
    epathStored += _protocol.epath[i];
  }
  _prefs.putString("epath", epathStored);

  // Save KISS
  _prefs.putBool("kiss", _kissEnabled);
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

void LoRaTNCX::onKissFrame(const uint8_t *data, size_t len)
{
  // Received a KISS data frame - transmit it over LoRa
  if (len > 0 && len <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(data, len);
  }
}

void LoRaTNCX::onPacketReceived(const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi)
{
  // PASSALL filtering: when ON, only accept frames with errors; when OFF, only accept correct frames
  bool fcsValid = AX25::validateFCS(buf, len);
  if (_protocol.passall && fcsValid)
  {
    // PASSALL is ON and FCS is valid, so reject this packet (we only want error frames)
    return;
  }
  if (!_protocol.passall && !fcsValid)
  {
    // PASSALL is OFF and FCS is invalid, so reject this packet (we only want good frames)
    return;
  }

  // Add to heard list
  if (ai.ok && ai.src.length())
  {
    addMHeard(ai.src);
  }

  // TRACE mode: display full packet as hex dump
  if (_monitor.trace)
  {
    _io.print("TRACE: ");
    for (size_t i = 0; i < len; i++)
    {
      _io.printf("%02X ", buf[i]);
      if ((i + 1) % 16 == 0 && i + 1 < len)
      {
        _io.print("\r\n       "); // Indent continuation lines
      }
    }
    _io.printf("(len=%u, rssi=%d)\r\n", len, rssi);
  }

  // Monitor output
  if (_monitor.enabled)
  {
    // Check MCON: don't monitor while connected if MCON is OFF
    if (!_monitor.mcon && _l2state == L2_CONNECTED)
    {
      // Skip monitoring while connected
    }
    else
    {
      // Check MALL: if OFF, only monitor unconnected stations
      bool shouldMonitor = true;
      if (!_monitor.mall && _l2state == L2_CONNECTED && ai.ok && ai.src.equalsIgnoreCase(_connectedTo))
      {
        shouldMonitor = false; // Skip monitoring our connected station
      }

      // Check MCOM: if OFF, only monitor info packets (not control packets)
      if (!_monitor.mcom && ai.hasControl)
      {
        shouldMonitor = false; // Skip control packets
      }

      if (shouldMonitor)
      {
        // Timestamp if enabled
        if (_monitor.mstamp)
        {
          unsigned long now = millis();
          unsigned long seconds = now / 1000;
          unsigned long minutes = seconds / 60;
          unsigned long hours = minutes / 60;
          _io.printf("[%02lu:%02lu:%02lu] ", hours % 24, minutes % 60, seconds % 60);
        }

        if (ai.ok)
        {
          _io.printf("[%s", ai.src.c_str());

          // Display digipeat path if MRPT is ON and path exists
          if (_monitor.mrpt && !ai.digis.empty())
          {
            _io.print(" via");
            for (size_t i = 0; i < ai.digis.size(); i++)
            {
              _io.print(" ");
              _io.print(ai.digis[i]);
              if (i < ai.digi_used.size() && ai.digi_used[i])
              {
                _io.print("*"); // Mark digis that have been used
              }
            }
          }

          _io.print("] ");
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
    }
  }

  // Digipeater logic - check if we should digipeat this packet
  if (_digi.enabled && ai.ok && AX25::shouldDigipeat(ai, _station.myCall, _station.myAlias))
  {
    // Build modified packet with H bit set for our digi
    std::vector<uint8_t> newPacket = AX25::digipeatPacket(buf, len, ai);

    if (!newPacket.empty() && newPacket.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
    {
      // Transmit the digipeated packet
      _radio.send(newPacket.data(), newPacket.size());

      // Start/reset HID timer
      if (_digi.hid)
      {
        _digi.lastHidMs = millis();
      }

      // Log the digipeat action
      if (_monitor.enabled)
      {
        if (_monitor.mstamp)
        {
          unsigned long now = millis();
          unsigned long seconds = now / 1000;
          unsigned long minutes = seconds / 60;
          unsigned long hours = minutes / 60;
          _io.printf("[%02lu:%02lu:%02lu] ", hours % 24, minutes % 60, seconds % 60);
        }
        _io.printf("*** DIGIPEATED from %s via %s\r\n",
                   ai.src.c_str(),
                   ai.digis[ai.next_digi_index].c_str());
      }
    }
    else if (_monitor.enabled && !newPacket.empty())
    {
      _io.printf("*** DIGIPEAT skipped: packet too large (%u bytes)\r\n", (unsigned)newPacket.size());
    }
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

      if (!_connection.conok)
      {
        // Reject with DM
        auto dm = AX25::encodeControlFrame(ai.src, _station.myCall.length() ? _station.myCall : String("NOCALL"), AX25::CTL_DM);
        if (dm.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
        {
          _radio.send(dm.data(), dm.size());
        }
        _io.printf("connect request: %s\r\n", ai.src.c_str());
        return;
      }

      // Accept with UA
      auto frame = AX25::encodeControlFrame(ai.src, _station.myCall.length() ? _station.myCall : String("NOCALL"), AX25::CTL_UA);
      if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
      {
        _radio.send(frame.data(), frame.size());
      }

      _connectedTo = ai.src;
      _l2state = L2_CONNECTED;
      _tries = 0;
      _io.printf("*** CONNECTED to %s (peer)\r\n", _connectedTo.c_str());
      maybeSendConnectText();
      maybeEnterMode();
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
        maybeEnterMode();
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

  // Forward packet to KISS mode if active
  if (_cmd.getMode() == CommandProcessor::MODE_KISS)
  {
    _cmd.sendKissFrame(buf, len);
  }
}

void LoRaTNCX::maybeSendConnectText()
{
  if (!_connection.cmsgOn || _connection.ctext.length() == 0 || _connectedTo.length() == 0)
  {
    return;
  }

  // Build and send CTEXT
  std::vector<uint8_t> payload(_connection.ctext.begin(), _connection.ctext.end());
  std::vector<uint8_t> frame = AX25::encodeUIFrame(
      _connectedTo,
      _station.myCall.length() ? _station.myCall : String("NOCALL"),
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
  if (_connection.cmsgDisc)
  {
    auto df = AX25::encodeControlFrame(
        _connectedTo,
        _station.myCall.length() ? _station.myCall : String("NOCALL"),
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

void LoRaTNCX::sendId()
{
  // Build ID packet with callsign
  String idText = String("ID ") + (_station.myCall.length() ? _station.myCall : String("NOCALL"));
  std::vector<uint8_t> payload(idText.begin(), idText.end());

  // Send as UI frame to ID destination
  std::vector<uint8_t> frame = AX25::encodeUIFrame(
      String("ID"),
      _station.myCall.length() ? _station.myCall : String("NOCALL"),
      payload);

  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(frame.data(), frame.size());
    _io.println("*** ID sent");
  }
  else
  {
    _io.printf("ERR ID frame too large (%u bytes)\r\n", (unsigned)frame.size());
  }
}

void LoRaTNCX::maybeEnterMode()
{
  // Only auto-enter mode if NOMODE is OFF and NEWMODE is OFF
  // (NEWMODE ON means we already entered immediately on CONNECT command)
  if (_conmode.nomode || _conmode.newmode)
  {
    return;
  }

  // Enter the configured mode
  if (_conmode.mode == CONMODE_CONVERSE)
  {
    _cmd.setMode(CommandProcessor::MODE_CONVERSE);
    _io.println("*** CONVERSE MODE");
  }
  else
  {
    _cmd.setMode(CommandProcessor::MODE_TRANSPARENT);
    _io.println("*** TRANSPARENT MODE");
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
    if ((char)_packet.sendpac == c)
    {
      if (_converseBuf.length() > 0)
      {
        flushConverseBuffer();
      }
      continue;
    }

    // CR handling
    if (_packet.cr && (c == '\r' || c == '\n'))
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
    if ((size_t)_converseBuf.length() >= (size_t)_packet.paclen)
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
    size_t chunk = remaining > _packet.paclen ? _packet.paclen : remaining;
    std::vector<uint8_t> payload;
    for (size_t i = 0; i < chunk; i++)
    {
      payload.push_back((uint8_t)_converseBuf.charAt(pos + i));
    }

    std::vector<uint8_t> frame;
    if (useConnected)
    {
      frame = AX25::encodeUIFrame(dest, _station.myCall.length() ? _station.myCall : String("NOCALL"), payload);
    }
    else
    {
      std::vector<String> digis(_unproto.begin(), _unproto.end());
      frame = AX25::encodeUIFrame(dest, _station.myCall.length() ? _station.myCall : String("NOCALL"), digis, payload);
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
  _io.printf("Coding rate: %d (4/%d)\r\n", _radio.getCodingRate(), _radio.getCodingRate());
  _io.printf("MYCALL: %s\r\n", _station.myCall.c_str());
  _io.printf("MONITOR: %s\r\n", _monitor.enabled ? "ON" : "OFF");
  _io.printf("CONOK: %s\r\n", _connection.conok ? "ON" : "OFF");
  _io.printf("Connection: %s\r\n", _l2state == L2_CONNECTED ? _connectedTo.c_str() : "DISCONNECTED");
}

void LoRaTNCX::cmdDisplay(const String &args)
{
  (void)args;
  _io.println("=== LoRaTNCX Settings ===");
  _io.println();
  _io.println("-- Station --");
  _io.printf("MYCALL:   %s\r\n", _station.myCall.c_str());
  _io.printf("MYALIAS:  %s\r\n", _station.myAlias.c_str());
  _io.println();
  _io.println("-- Radio --");
  _io.printf("FREQ:     %.2f MHz\r\n", _radio.getFrequency());
  _io.printf("POWER:    %d dBm\r\n", _radio.getTxPower());
  _io.printf("SF:       %d\r\n", _radio.getSpreadingFactor());
  _io.printf("BW:       %ld kHz\r\n", _radio.getBandwidth());
  _io.printf("CODING:   %d (4/%d)\r\n", _radio.getCodingRate(), _radio.getCodingRate());
  _io.println();
  _io.println("-- Monitoring --");
  _io.printf("MONITOR:  %s\r\n", _monitor.enabled ? "ON" : "OFF");
  _io.printf("MSTAMP:   %s\r\n", _monitor.mstamp ? "ON" : "OFF");
  _io.printf("MALL:     %s\r\n", _monitor.mall ? "ON" : "OFF");
  _io.printf("MCOM:     %s\r\n", _monitor.mcom ? "ON" : "OFF");
  _io.printf("MCON:     %s\r\n", _monitor.mcon ? "ON" : "OFF");
  _io.printf("MRPT:     %s\r\n", _monitor.mrpt ? "ON" : "OFF");
  _io.println();
  _io.println("-- Connection --");
  _io.printf("CONOK:    %s\r\n", _connection.conok ? "ON" : "OFF");
  _io.printf("RETRY:    %u\r\n", (unsigned)_connection.retry);
  _io.printf("FRACK:    %u sec\r\n", (unsigned)_connection.frack);
  _io.printf("State:    %s\r\n", _l2state == L2_CONNECTED ? "CONNECTED" : (_l2state == L2_CONNECTING ? "CONNECTING" : "DISCONNECTED"));
  if (_l2state != L2_DISCONNECTED)
    _io.printf("To:       %s\r\n", _connectedTo.c_str());
  _io.println();
  _io.println("-- Beacon --");
  _io.printf("BEACON:   %s", _beacon.mode == BEACON_OFF ? "OFF" : (_beacon.mode == BEACON_EVERY ? "EVERY" : "AFTER"));
  if (_beacon.mode != BEACON_OFF)
    _io.printf(" %u sec", _beacon.interval);
  _io.println();
  _io.printf("BTEXT:    %s\r\n", _beacon.text.c_str());
  _io.print("UNPROTO:  ");
  if (_unproto.empty())
  {
    _io.println("none");
  }
  else
  {
    for (size_t i = 0; i < _unproto.size(); i++)
    {
      if (i)
        _io.print(",");
      _io.print(_unproto[i]);
    }
    _io.println();
  }
  _io.println();
  _io.println("-- Converse --");
  _io.printf("PACLEN:   %u\r\n", (unsigned)_packet.paclen);
  _io.printf("PACTIME:  %lu ms\r\n", (unsigned long)_packet.pactime);
  _io.printf("SENDPAC:  %d (0x%02X)\r\n", (int)_packet.sendpac, (int)_packet.sendpac);
  _io.printf("CR:       %s\r\n", _packet.cr ? "ON" : "OFF");
  _io.printf("ECHO:     %s\r\n", _terminal.echo ? "ON" : "OFF");
  _io.printf("CONMODE:  %s\r\n", _conmode.mode == CONMODE_CONVERSE ? "CONVERSE" : "TRANS");
  _io.printf("NEWMODE:  %s\r\n", _conmode.newmode ? "ON" : "OFF");
  _io.printf("NOMODE:   %s\r\n", _conmode.nomode ? "ON" : "OFF");
  _io.println();
  _io.println("-- Digipeat --");
  _io.printf("DIGIPEAT: %s\r\n", _digi.enabled ? "ON" : "OFF");
  _io.println();
  _io.println("-- Identification --");
  _io.printf("HID:      %s\r\n", _digi.hid ? "ON" : "OFF");
  _io.println();
  _io.println("-- Connect Text --");
  _io.printf("CMSG:     %s\r\n", _connection.cmsgOn ? "ON" : "OFF");
  _io.printf("CMSGDISC: %s\r\n", _connection.cmsgDisc ? "ON" : "OFF");
  _io.printf("CTEXT:    %s\r\n", _connection.ctext.c_str());
  _io.println();
  _io.println("-- Advanced Protocol --");
  _io.printf("TRACE:    %s\r\n", _monitor.trace ? "ON" : "OFF");
  _io.printf("FLOW:     %s\r\n", _protocol.flow ? "ON" : "OFF");
  _io.printf("PASSALL:  %s\r\n", _protocol.passall ? "ON" : "OFF");
  _io.printf("RESPTIME: %u (x100ms = %u ms)\r\n", _protocol.resptime, _protocol.resptime * 100);
  _io.printf("CPACTIME: %s\r\n", _packet.cpactime ? "ON" : "OFF");
  _io.print("EPATH:    ");
  if (_protocol.epath.empty())
  {
    _io.println("none");
  }
  else
  {
    for (size_t i = 0; i < _protocol.epath.size(); i++)
    {
      if (i)
        _io.print(",");
      _io.print(_protocol.epath[i]);
    }
    _io.println();
  }
  _io.println();
  _io.println("-- Date/Time (Manual Entry) --");
  _io.printf("CONSTAMP: %s\r\n", _datetime.constamp ? "ON" : "OFF");
  _io.printf("DAYSTAMP: %s\r\n", _datetime.daystamp ? "ON" : "OFF");
  _io.printf("DAYUSA:   %s (%s)\r\n", _datetime.dayusa ? "ON" : "OFF", _datetime.dayusa ? "MM/DD/YY" : "DD-MM-YY");
  if (_datetime.value.length() == 0)
  {
    _io.println("DAYTIME:  not set");
  }
  else if (_datetime.value.length() >= 12)
  {
    String yy = _datetime.value.substring(0, 2);
    String mm = _datetime.value.substring(2, 4);
    String dd = _datetime.value.substring(4, 6);
    String hh = _datetime.value.substring(6, 8);
    String min = _datetime.value.substring(8, 10);
    String ss = _datetime.value.substring(10, 12);
    if (_datetime.dayusa)
    {
      _io.printf("DAYTIME:  %s/%s/%s %s:%s:%s\r\n", mm.c_str(), dd.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
    }
    else
    {
      _io.printf("DAYTIME:  %s-%s-%s %s:%s:%s\r\n", dd.c_str(), mm.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
    }
  }
  else
  {
    _io.printf("DAYTIME:  %s (invalid)\r\n", _datetime.value.c_str());
  }
  _io.println();
  _io.println("-- GPS/Location (Manual Entry) --");
  if (_location.mode == LOCATION_OFF)
  {
    _io.println("LOCATION: EVERY 0 (OFF)");
  }
  else if (_location.mode == LOCATION_EVERY)
  {
    _io.printf("LOCATION: EVERY %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
  }
  else
  {
    _io.printf("LOCATION: AFTER %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
  }
  _io.printf("LPATH:    %s\r\n", _location.lpath.c_str());
  if (_location.ltext.length() == 0)
  {
    _io.println("LTEXT:    (none)");
  }
  else
  {
    _io.printf("LTEXT:    %s\r\n", _location.ltext.c_str());
  }
  _io.printf("LTMON:    %u seconds\r\n", _location.ltmon);

  // KISS mode
  _io.println();
  _io.printf("KISS:     %s\r\n", _kissEnabled ? "ON" : "OFF");
  if (_kissEnabled)
  {
    _io.println("  Use RESTART to enter KISS mode");
    _io.println("  Exit with ESC (0x1B) or CMD_RETURN (0xFF)");
  }
}

void LoRaTNCX::cmdEcho(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("ECHO %s\r\n", _terminal.echo ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _terminal.echo = true;
    _cmd.setLocalEcho(true);
    _prefs.putBool("echo", true);
    _io.println("OK ECHO ON");
  }
  else if (s == "OFF")
  {
    _terminal.echo = false;
    _cmd.setLocalEcho(false);
    _prefs.putBool("echo", false);
    _io.println("OK ECHO OFF");
  }
  else
  {
    _io.println(F("ERR ECHO must be ON or OFF"));
  }
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
    _io.printf("CODING %d\r\n", _radio.getCodingRate());
    return;
  }

  int cr = s.toInt();
  // Valid coding rates are 5-8 (corresponding to 4/5, 4/6, 4/7, 4/8)
  if (cr < 5 || cr > 8)
  {
    _io.println(F("ERR CODING must be 5-8 (4/5 to 4/8)"));
    return;
  }

  int r = _radio.setCodingRate(cr);
  if (r == 0)
  {
    _io.printf("OK CODING %d\r\n", cr);
  }
  else
  {
    _io.printf("ERR set coding rate: %d\r\n", r);
  }
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
    _io.printf("MYCALL %s\r\n", _station.myCall.c_str());
    return;
  }

  _station.myCall = s;
  _prefs.putString("mycall", _station.myCall);
  _io.printf("OK MYCALL %s\r\n", _station.myCall.c_str());
}

void LoRaTNCX::cmdMyAlias(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("MYALIAS %s\r\n", _station.myAlias.c_str());
    return;
  }

  _station.myAlias = s;
  _prefs.putString("myalias", _station.myAlias);
  _io.printf("OK MYALIAS %s\r\n", _station.myAlias.c_str());
}

void LoRaTNCX::cmdMonitor(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MONITOR %s\r\n", _monitor.enabled ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.enabled = true;
    _prefs.putBool("monitor", true);
    _io.println("OK MONITOR ON");
  }
  else if (s == "OFF")
  {
    _monitor.enabled = false;
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

void LoRaTNCX::cmdDigipeat(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("DIGIPEAT %s\r\n", _digi.enabled ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _digi.enabled = true;
    _prefs.putBool("digipeat", true);
    _io.println("OK DIGIPEAT ON");
  }
  else if (s == "OFF")
  {
    _digi.enabled = false;
    _prefs.putBool("digipeat", false);
    _io.println("OK DIGIPEAT OFF");
  }
  else
  {
    _io.println(F("ERR DIGIPEAT must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMStamp(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MSTAMP %s\r\n", _monitor.mstamp ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.mstamp = true;
    _prefs.putBool("mstamp", true);
    _io.println("OK MSTAMP ON");
  }
  else if (s == "OFF")
  {
    _monitor.mstamp = false;
    _prefs.putBool("mstamp", false);
    _io.println("OK MSTAMP OFF");
  }
  else
  {
    _io.println(F("ERR MSTAMP must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMAll(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MALL %s\r\n", _monitor.mall ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.mall = true;
    saveSettings();
    _io.println("OK MALL ON");
  }
  else if (s == "OFF")
  {
    _monitor.mall = false;
    saveSettings();
    _io.println("OK MALL OFF");
  }
  else
  {
    _io.println(F("ERR MALL must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMCom(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MCOM %s\r\n", _monitor.mcom ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.mcom = true;
    saveSettings();
    _io.println("OK MCOM ON");
  }
  else if (s == "OFF")
  {
    _monitor.mcom = false;
    saveSettings();
    _io.println("OK MCOM OFF");
  }
  else
  {
    _io.println(F("ERR MCOM must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMCon(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MCON %s\r\n", _monitor.mcon ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.mcon = true;
    saveSettings();
    _io.println("OK MCON ON");
  }
  else if (s == "OFF")
  {
    _monitor.mcon = false;
    saveSettings();
    _io.println("OK MCON OFF");
  }
  else
  {
    _io.println(F("ERR MCON must be ON or OFF"));
  }
}

void LoRaTNCX::cmdMRpt(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("MRPT %s\r\n", _monitor.mrpt ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.mrpt = true;
    saveSettings();
    _io.println("OK MRPT ON");
  }
  else if (s == "OFF")
  {
    _monitor.mrpt = false;
    saveSettings();
    _io.println("OK MRPT OFF");
  }
  else
  {
    _io.println(F("ERR MRPT must be ON or OFF"));
  }
}

void LoRaTNCX::cmdId(const String &args)
{
  (void)args;
  sendId();
}

void LoRaTNCX::cmdHId(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("HID %s\r\n", _digi.hid ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _digi.hid = true;
    saveSettings();
    _io.println("OK HID ON");
  }
  else if (s == "OFF")
  {
    _digi.hid = false;
    saveSettings();
    _io.println("OK HID OFF");
  }
  else
  {
    _io.println(F("ERR HID must be ON or OFF"));
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
      _station.myCall.length() ? _station.myCall : String("NOCALL"),
      AX25::CTL_SABM);

  if (frame.size() <= (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _radio.send(frame.data(), frame.size());
  }
  else
  {
    _io.printf("ERR CONNECT frame too large (%u bytes)\r\n", (unsigned)frame.size());
  }

  // NEWMODE: Enter mode immediately on CONNECT command (if NOMODE is OFF)
  if (_conmode.newmode && !_conmode.nomode)
  {
    if (_conmode.mode == CONMODE_CONVERSE)
    {
      _cmd.setMode(CommandProcessor::MODE_CONVERSE);
      _io.println("*** CONVERSE MODE");
    }
    else
    {
      _cmd.setMode(CommandProcessor::MODE_TRANSPARENT);
      _io.println("*** TRANSPARENT MODE");
    }
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
      _station.myCall.length() ? _station.myCall : String("NOCALL"),
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
    _io.printf("CONOK %s\r\n", _connection.conok ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _connection.conok = true;
    _prefs.putBool("conok", true);
    _io.println("OK CONOK ON");
  }
  else if (s == "OFF")
  {
    _connection.conok = false;
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
    _io.printf("RETRY %u\r\n", (unsigned)_connection.retry);
    return;
  }

  int v = s.toInt();
  if (v < 0 || v > 255)
  {
    _io.println(F("ERR invalid retry value"));
    return;
  }

  _connection.retry = (uint8_t)v;
  _prefs.putUInt("retry", (uint32_t)_connection.retry);
  _io.printf("OK RETRY %u\r\n", (unsigned)_connection.retry);
}

void LoRaTNCX::cmdFrack(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("FRACK %u\r\n", (unsigned)_connection.frack);
    return;
  }

  int v = s.toInt();
  if (v < 0 || v > 255)
  {
    _io.println(F("ERR invalid FRACK value"));
    return;
  }

  _connection.frack = (uint8_t)v;
  _prefs.putUInt("frack", (uint32_t)_connection.frack);
  _io.printf("OK FRACK %u\r\n", (unsigned)_connection.frack);
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
    _io.printf("CTEXT %s\r\n", _connection.ctext.c_str());
    return;
  }

  _connection.ctext = s;
  _prefs.putString("ctext", _connection.ctext);
  _io.printf("OK CTEXT %s\r\n", _connection.ctext.c_str());
}

void LoRaTNCX::cmdCMsg(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CMSG %s\r\n", _connection.cmsgOn ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _connection.cmsgOn = true;
    _prefs.putBool("cmsg", true);
    _io.println("OK CMSG ON");
  }
  else if (s == "OFF")
  {
    _connection.cmsgOn = false;
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
    _io.printf("CMSGDISC %s\r\n", _connection.cmsgDisc ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _connection.cmsgDisc = true;
    _prefs.putBool("cmsgdisc", true);
    _io.println("OK CMSGDISC ON");
  }
  else if (s == "OFF")
  {
    _connection.cmsgDisc = false;
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
               (int)_beacon.mode, _beacon.interval, _beacon.text.c_str());
    return;
  }

  int sp = s.indexOf(' ');
  String cmd = (sp == -1) ? s : s.substring(0, sp);
  String rest = (sp == -1) ? String("") : s.substring(sp + 1);
  cmd.toUpperCase();

  if (cmd == "OFF")
  {
    _beacon.mode = BEACON_OFF;
    _prefs.putUInt("beacon_mode", (uint32_t)_beacon.mode);
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
    _beacon.mode = BEACON_EVERY;
    _beacon.interval = v;
    _prefs.putUInt("beacon_mode", (uint32_t)_beacon.mode);
    _prefs.putUInt("beacon_interval", _beacon.interval);
    _io.printf("OK BEACON EVERY %u\r\n", _beacon.interval);
  }
  else if (cmd == "AFTER")
  {
    uint32_t v = rest.toInt();
    if (v == 0)
    {
      _io.println("ERR invalid interval");
      return;
    }
    _beacon.mode = BEACON_AFTER;
    _beacon.interval = v;
    _prefs.putUInt("beacon_mode", (uint32_t)_beacon.mode);
    _prefs.putUInt("beacon_interval", _beacon.interval);
    _io.printf("OK BEACON AFTER %u\r\n", _beacon.interval);
  }
  else if (cmd == "SEND")
  {
    // Manual beacon send
    String body = _beacon.text.length() ? _beacon.text : (String("BEACON ") + _station.myCall);
    std::vector<uint8_t> payload(body.begin(), body.end());
    std::vector<String> digis(_unproto.begin(), _unproto.end());
    std::vector<uint8_t> frame = AX25::encodeUIFrame(
        String("BEACON"),
        _station.myCall.length() ? _station.myCall : String("NOCALL"),
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
    _io.printf("BTEXT %s\r\n", _beacon.text.c_str());
    return;
  }

  _beacon.text = s;
  _prefs.putString("beacon_text", _beacon.text);
  _io.printf("OK BTEXT %s\r\n", _beacon.text.c_str());
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
    _io.printf("PACLEN %u\r\n", (unsigned)_packet.paclen);
    return;
  }

  int v = s.toInt();
  if (v <= 0 || v > RADIOLIB_SX126X_MAX_PACKET_LENGTH)
  {
    _io.println(F("ERR invalid PACLEN"));
    return;
  }

  _packet.paclen = (uint16_t)v;
  _prefs.putUInt("paclen", (uint32_t)_packet.paclen);
  _io.printf("OK PACLEN %u\r\n", (unsigned)_packet.paclen);
}

void LoRaTNCX::cmdPactime(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("PACTIME %lu\r\n", (unsigned long)_packet.pactime);
    return;
  }

  long v = s.toInt();
  if (v < 0)
  {
    _io.println(F("ERR invalid PACTIME"));
    return;
  }

  _packet.pactime = (uint32_t)v;
  _prefs.putUInt("pactime", _packet.pactime);
  _io.printf("OK PACTIME %lu\r\n", (unsigned long)_packet.pactime);
}

void LoRaTNCX::cmdSendpac(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    int v = (int)_packet.sendpac;
    _io.printf("SENDPAC %d ('%c')\r\n", v, (v >= 32 && v < 127) ? _packet.sendpac : ' ');
    return;
  }

  if (s.equalsIgnoreCase("CR"))
  {
    _packet.sendpac = '\r';
  }
  else if (s.equalsIgnoreCase("LF") || s.equalsIgnoreCase("NL"))
  {
    _packet.sendpac = '\n';
  }
  else if (s.length() == 1)
  {
    _packet.sendpac = s.charAt(0);
  }
  else
  {
    int v = s.toInt();
    if (v < 0 || v > 255)
    {
      _io.println(F("ERR invalid SENDPAC"));
      return;
    }
    _packet.sendpac = (char)v;
  }

  _prefs.putString("sendpac", String(_packet.sendpac));
  _cmd.setSendPacChar(_packet.sendpac);
  _io.printf("OK SENDPAC %d\r\n", (int)_packet.sendpac);
}

void LoRaTNCX::cmdCR(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CR %s\r\n", _packet.cr ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _packet.cr = true;
    _prefs.putBool("cr", true);
    _io.println("OK CR ON");
  }
  else if (s == "OFF")
  {
    _packet.cr = false;
    _prefs.putBool("cr", false);
    _io.println("OK CR OFF");
  }
  else
  {
    _io.println(F("ERR CR must be ON or OFF"));
  }
}

void LoRaTNCX::cmdConMode(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CONMODE %s\r\n", _conmode.mode == CONMODE_CONVERSE ? "CONVERSE" : "TRANS");
    return;
  }

  if (s == "CONVERSE" || s == "CONV")
  {
    _conmode.mode = CONMODE_CONVERSE;
    saveSettings();
    _io.println("OK CONMODE CONVERSE");
  }
  else if (s == "TRANS" || s == "T")
  {
    _conmode.mode = CONMODE_TRANS;
    saveSettings();
    _io.println("OK CONMODE TRANS");
  }
  else
  {
    _io.println(F("ERR CONMODE must be CONVERSE or TRANS"));
  }
}

void LoRaTNCX::cmdNewMode(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("NEWMODE %s\r\n", _conmode.newmode ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _conmode.newmode = true;
    saveSettings();
    _io.println("OK NEWMODE ON");
  }
  else if (s == "OFF")
  {
    _conmode.newmode = false;
    saveSettings();
    _io.println("OK NEWMODE OFF");
  }
  else
  {
    _io.println(F("ERR NEWMODE must be ON or OFF"));
  }
}

void LoRaTNCX::cmdNoMode(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("NOMODE %s\r\n", _conmode.nomode ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _conmode.nomode = true;
    saveSettings();
    _io.println("OK NOMODE ON");
  }
  else if (s == "OFF")
  {
    _conmode.nomode = false;
    saveSettings();
    _io.println("OK NOMODE OFF");
  }
  else
  {
    _io.println(F("ERR NOMODE must be ON or OFF"));
  }
}

void LoRaTNCX::cmdCPacTime(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CPACTIME %s\r\n", _packet.cpactime ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _packet.cpactime = true;
    saveSettings();
    _io.println("OK CPACTIME ON");
  }
  else if (s == "OFF")
  {
    _packet.cpactime = false;
    saveSettings();
    _io.println("OK CPACTIME OFF");
  }
  else
  {
    _io.println(F("ERR CPACTIME must be ON or OFF"));
  }
}

// ============================================================================
// COMMAND HANDLERS - Advanced Protocol
// ============================================================================

void LoRaTNCX::cmdTrace(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("TRACE %s\r\n", _monitor.trace ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _monitor.trace = true;
    saveSettings();
    _io.println("OK TRACE ON");
  }
  else if (s == "OFF")
  {
    _monitor.trace = false;
    saveSettings();
    _io.println("OK TRACE OFF");
  }
  else
  {
    _io.println(F("ERR TRACE must be ON or OFF"));
  }
}

void LoRaTNCX::cmdFlow(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("FLOW %s\r\n", _protocol.flow ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _protocol.flow = true;
    saveSettings();
    _io.println("OK FLOW ON");
  }
  else if (s == "OFF")
  {
    _protocol.flow = false;
    saveSettings();
    _io.println("OK FLOW OFF");
  }
  else
  {
    _io.println(F("ERR FLOW must be ON or OFF"));
  }
}

void LoRaTNCX::cmdPassAll(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("PASSALL %s\r\n", _protocol.passall ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _protocol.passall = true;
    saveSettings();
    _io.println("OK PASSALL ON");
  }
  else if (s == "OFF")
  {
    _protocol.passall = false;
    saveSettings();
    _io.println("OK PASSALL OFF");
  }
  else
  {
    _io.println(F("ERR PASSALL must be ON or OFF"));
  }
}

void LoRaTNCX::cmdRespTime(const String &args)
{
  String s = args;
  s.trim();
  if (s.length() == 0)
  {
    _io.printf("RESPTIME %u (x100ms = %u ms)\r\n", _protocol.resptime, _protocol.resptime * 100);
    return;
  }

  int val = s.toInt();
  if (val < 0 || val > 250)
  {
    _io.println(F("ERR RESPTIME must be 0..250"));
    return;
  }

  _protocol.resptime = (uint8_t)val;
  saveSettings();
  _io.printf("OK RESPTIME %u (x100ms = %u ms)\r\n", _protocol.resptime, _protocol.resptime * 100);
}

void LoRaTNCX::cmdEPath(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();

  if (s.length() == 0)
  {
    // Display current EPATH
    _io.print("EPATH");
    if (_protocol.epath.empty())
    {
      _io.println(" (none)");
    }
    else
    {
      for (size_t i = 0; i < _protocol.epath.size(); i++)
      {
        _io.print(" ");
        _io.print(_protocol.epath[i]);
      }
      _io.println();
    }
    return;
  }

  // Parse comma-separated or space-separated callsigns
  _protocol.epath.clear();
  int start = 0;
  for (int i = 0; i <= (int)s.length(); i++)
  {
    if (i == (int)s.length() || s[i] == ',' || s[i] == ' ')
    {
      if (i > start)
      {
        String call = s.substring(start, i);
        call.trim();
        if (call.length() > 0)
        {
          _protocol.epath.push_back(call);
        }
      }
      start = i + 1;
    }
  }

  saveSettings();
  _io.print("OK EPATH");
  if (_protocol.epath.empty())
  {
    _io.println(" (cleared)");
  }
  else
  {
    for (size_t i = 0; i < _protocol.epath.size(); i++)
    {
      _io.print(" ");
      _io.print(_protocol.epath[i]);
    }
    _io.println();
  }
}

// ============================================================================
// COMMAND HANDLERS - Date/Time
// ============================================================================

void LoRaTNCX::cmdConStamp(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("CONSTAMP %s\r\n", _datetime.constamp ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _datetime.constamp = true;
    saveSettings();
    _io.println("OK CONSTAMP ON");
  }
  else if (s == "OFF")
  {
    _datetime.constamp = false;
    saveSettings();
    _io.println("OK CONSTAMP OFF");
  }
  else
  {
    _io.println(F("ERR CONSTAMP must be ON or OFF"));
  }
}

void LoRaTNCX::cmdDayStamp(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("DAYSTAMP %s\r\n", _datetime.daystamp ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _datetime.daystamp = true;
    saveSettings();
    _io.println("OK DAYSTAMP ON");
  }
  else if (s == "OFF")
  {
    _datetime.daystamp = false;
    saveSettings();
    _io.println("OK DAYSTAMP OFF");
  }
  else
  {
    _io.println(F("ERR DAYSTAMP must be ON or OFF"));
  }
}

void LoRaTNCX::cmdDayUsa(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();
  if (s.length() == 0)
  {
    _io.printf("DAYUSA %s\r\n", _datetime.dayusa ? "ON" : "OFF");
    return;
  }

  if (s == "ON")
  {
    _datetime.dayusa = true;
    saveSettings();
    _io.println("OK DAYUSA ON (MM/DD/YY)");
  }
  else if (s == "OFF")
  {
    _datetime.dayusa = false;
    saveSettings();
    _io.println("OK DAYUSA OFF (DD-MM-YY)");
  }
  else
  {
    _io.println(F("ERR DAYUSA must be ON or OFF"));
  }
}

void LoRaTNCX::cmdDayTime(const String &args)
{
  String s = args;
  s.trim();

  if (s.length() == 0)
  {
    // Display current date/time if set
    if (_datetime.value.length() == 0)
    {
      _io.println("DAYTIME not set");
    }
    else
    {
      // Parse and display: YYMMDDhhmmss
      if (_datetime.value.length() >= 12)
      {
        String yy = _datetime.value.substring(0, 2);
        String mm = _datetime.value.substring(2, 4);
        String dd = _datetime.value.substring(4, 6);
        String hh = _datetime.value.substring(6, 8);
        String min = _datetime.value.substring(8, 10);
        String ss = _datetime.value.substring(10, 12);

        if (_datetime.dayusa)
        {
          _io.printf("DAYTIME %s/%s/%s %s:%s:%s\r\n", mm.c_str(), dd.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
        }
        else
        {
          _io.printf("DAYTIME %s-%s-%s %s:%s:%s\r\n", dd.c_str(), mm.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
        }
      }
      else
      {
        _io.printf("DAYTIME %s (invalid format)\r\n", _datetime.value.c_str());
      }
    }
    return;
  }

  // Parse YYMMDDhhmmss or YYMMDDhhmm (seconds default to 00)
  if (s.length() != 12 && s.length() != 10)
  {
    _io.println(F("ERR DAYTIME format: YYMMDDhhmmss or YYMMDDhhmm"));
    return;
  }

  // Validate all digits
  for (size_t i = 0; i < s.length(); i++)
  {
    if (!isdigit(s[i]))
    {
      _io.println(F("ERR DAYTIME must contain only digits"));
      return;
    }
  }

  // Append 00 for seconds if not provided
  if (s.length() == 10)
  {
    s += "00";
  }

  _datetime.value = s;
  saveSettings();

  String yy = _datetime.value.substring(0, 2);
  String mm = _datetime.value.substring(2, 4);
  String dd = _datetime.value.substring(4, 6);
  String hh = _datetime.value.substring(6, 8);
  String min = _datetime.value.substring(8, 10);
  String ss = _datetime.value.substring(10, 12);

  if (_datetime.dayusa)
  {
    _io.printf("OK DAYTIME set to %s/%s/%s %s:%s:%s\r\n", mm.c_str(), dd.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
  }
  else
  {
    _io.printf("OK DAYTIME set to %s-%s-%s %s:%s:%s\r\n", dd.c_str(), mm.c_str(), yy.c_str(), hh.c_str(), min.c_str(), ss.c_str());
  }
}

// ============================================================================
// COMMAND HANDLERS - GPS/Location
// ============================================================================

void LoRaTNCX::cmdLocation(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();

  if (s.length() == 0)
  {
    // Display current setting
    if (_location.mode == LOCATION_OFF)
    {
      _io.println("LOCATION EVERY 0 (OFF)");
    }
    else if (_location.mode == LOCATION_EVERY)
    {
      _io.printf("LOCATION EVERY %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
    }
    else // LOCATION_AFTER
    {
      _io.printf("LOCATION AFTER %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
    }
    return;
  }

  // Parse "EVERY n" or "AFTER n"
  int spaceIdx = s.indexOf(' ');
  if (spaceIdx == -1)
  {
    _io.println(F("ERR LOCATION format: EVERY n or AFTER n (n=0..250)"));
    return;
  }

  String mode = s.substring(0, spaceIdx);
  String valStr = s.substring(spaceIdx + 1);
  valStr.trim();

  int val = valStr.toInt();
  if (val < 0 || val > 250)
  {
    _io.println(F("ERR LOCATION interval must be 0..250"));
    return;
  }

  if (mode == "EVERY")
  {
    if (val == 0)
    {
      _location.mode = LOCATION_OFF;
      _location.interval = 0;
      saveSettings();
      _io.println("OK LOCATION EVERY 0 (OFF)");
    }
    else
    {
      _location.mode = LOCATION_EVERY;
      _location.interval = (uint32_t)val;
      saveSettings();
      _io.printf("OK LOCATION EVERY %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
    }
  }
  else if (mode == "AFTER")
  {
    _location.mode = LOCATION_AFTER;
    _location.interval = (uint32_t)val;
    saveSettings();
    _io.printf("OK LOCATION AFTER %u (x10s = %u s)\r\n", _location.interval, _location.interval * 10);
  }
  else
  {
    _io.println(F("ERR LOCATION format: EVERY n or AFTER n"));
  }
}

void LoRaTNCX::cmdLPath(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();

  if (s.length() == 0)
  {
    _io.printf("LPATH %s\r\n", _location.lpath.c_str());
    return;
  }

  _location.lpath = s;
  saveSettings();
  _io.printf("OK LPATH %s\r\n", _location.lpath.c_str());
}

void LoRaTNCX::cmdLText(const String &args)
{
  String s = args;
  s.trim();

  if (s.length() == 0)
  {
    if (_location.ltext.length() == 0)
    {
      _io.println("LTEXT (none)");
    }
    else
    {
      _io.printf("LTEXT %s\r\n", _location.ltext.c_str());
    }
    return;
  }

  // Validate ASCII range 0x20-0x7E and max 159 chars
  if (s.length() > 159)
  {
    _io.println(F("ERR LTEXT max 159 characters"));
    return;
  }

  for (size_t i = 0; i < s.length(); i++)
  {
    if (s[i] < 0x20 || s[i] > 0x7E)
    {
      _io.println(F("ERR LTEXT must contain only ASCII 0x20-0x7E"));
      return;
    }
  }

  _location.ltext = s;
  saveSettings();
  _io.printf("OK LTEXT %s\r\n", _location.ltext.c_str());
}

void LoRaTNCX::cmdLtMon(const String &args)
{
  String s = args;
  s.trim();

  if (s.length() == 0)
  {
    _io.printf("LTMON %u seconds\r\n", _location.ltmon);
    return;
  }

  int val = s.toInt();
  if (val < 0 || val > 250)
  {
    _io.println(F("ERR LTMON must be 0..250"));
    return;
  }

  _location.ltmon = (uint8_t)val;
  saveSettings();
  _io.printf("OK LTMON %u seconds\r\n", _location.ltmon);
}

// ============================================================================
// COMMAND HANDLERS - KISS Mode
// ============================================================================

void LoRaTNCX::cmdKiss(const String &args)
{
  String s = args;
  s.trim();
  s.toUpperCase();

  if (s.length() == 0)
  {
    _io.printf("KISS %s\r\n", _kissEnabled ? "ON" : "OFF");
    _io.println(F("  Use RESTART to enter/exit KISS mode when enabled"));
    return;
  }

  if (s == "ON")
  {
    _kissEnabled = true;
    _prefs.putBool("kiss", true);
    _io.println(F("OK KISS ON"));
    _io.println(F("  Use RESTART to enter KISS mode"));
  }
  else if (s == "OFF")
  {
    _kissEnabled = false;
    _prefs.putBool("kiss", false);
    _io.println(F("OK KISS OFF"));
  }
  else
  {
    _io.println(F("ERR KISS must be ON or OFF"));
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

  // Check if we should enter KISS mode
  if (_kissEnabled)
  {
    _io.println(F("Entering KISS mode..."));
    _io.println(F("Send ESC (0x1B) or CMD_RETURN (0xFF) to exit"));
    _io.flush();

    // Enter KISS mode
    _cmd.setMode(CommandProcessor::MODE_KISS);

    // Set up KISS frame handler
    _cmd.setKissFrameHandler([this](const uint8_t *data, size_t len)
                             { this->onKissFrame(data, len); });

    // Poll until exit requested
    while (!_cmd.isKissExitRequested())
    {
      _cmd.poll();
      delay(1); // Small delay to avoid busy-wait
    }

    // Exit KISS mode
    _cmd.setMode(CommandProcessor::MODE_COMMAND);
    _cmd.clearKissExit();
    _io.println();
    _io.println(F("Exited KISS mode"));
    return;
  }

  // Normal restart without KISS mode
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
  _station.myCall = String();
  _station.myAlias = String();
  _terminal.echo = true;
  _monitor.enabled = false;
  _monitor.mstamp = false;
  _digi.enabled = true;
  _beacon.mode = BEACON_OFF;
  _beacon.interval = 0;
  _beacon.text = String();
  _unproto.clear();
  _connection.retry = 10;
  _connection.frack = 8;
  _packet.paclen = 256;
  _packet.pactime = 1000;
  _packet.sendpac = '\n';
  _packet.cr = true;
  _connection.conok = true;
  _connection.cmsgOn = false;
  _connection.cmsgDisc = false;
  _connection.ctext = String();

  saveSettings();
  _cmd.setLocalEcho(_terminal.echo);
  _io.println(F("OK RESET"));
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

// Helper macro to reduce command registration boilerplate
// Usage: REG_CMD("MYCALL", cmdMyCall)  or  REG_CMD("MY", cmdMyCall)
#define REG_CMD(name, handler) \
  _cmd.registerCommand((name), [this](const String &a) { handler(a); })

void LoRaTNCX::registerAllCommands()
{
  // Core commands
  REG_CMD("HELP", cmdHelp);
  REG_CMD("?", cmdHelp);
  REG_CMD("VERSION", cmdVersion);
  REG_CMD("STATUS", cmdStatus);
  REG_CMD("DISPLAY", cmdDisplay);
  REG_CMD("DISP", cmdDisplay);
  REG_CMD("ECHO", cmdEcho);
  REG_CMD("E", cmdEcho);

  // Radio control
  REG_CMD("FREQ", cmdFreq);
  REG_CMD("FREQUENCY", cmdFreq);
  REG_CMD("PWR", cmdPwr);
  REG_CMD("POWER", cmdPwr);
  REG_CMD("SF", cmdSpreadingFactor);
  REG_CMD("SPREADING", cmdSpreadingFactor);
  REG_CMD("BW", cmdBandwidth);
  REG_CMD("BANDWIDTH", cmdBandwidth);
  REG_CMD("CODING", cmdCodingRate);
  REG_CMD("CRATE", cmdCodingRate);
  REG_CMD("RADIOINIT", cmdRadioInit);
  REG_CMD("RADIO", cmdRadioInit);

  // Station & monitoring
  REG_CMD("MYCALL", cmdMyCall);
  REG_CMD("MY", cmdMyCall);
  REG_CMD("MYALIAS", cmdMyAlias);
  REG_CMD("MYA", cmdMyAlias);
  REG_CMD("MONITOR", cmdMonitor);
  REG_CMD("M", cmdMonitor);
  REG_CMD("MHEARD", cmdMHeard);
  REG_CMD("DIGIPEAT", cmdDigipeat);
  REG_CMD("DIG", cmdDigipeat);
  REG_CMD("MSTAMP", cmdMStamp);
  REG_CMD("MS", cmdMStamp);
  REG_CMD("MALL", cmdMAll);
  REG_CMD("MA", cmdMAll);
  REG_CMD("MCOM", cmdMCom);
  REG_CMD("MCON", cmdMCon);
  REG_CMD("MC", cmdMCon);
  REG_CMD("MRPT", cmdMRpt);
  REG_CMD("MR", cmdMRpt);
  REG_CMD("ID", cmdId);
  REG_CMD("I", cmdId);
  REG_CMD("HID", cmdHId);
  REG_CMD("HI", cmdHId);

  // Connection & link
  REG_CMD("CONNECT", cmdConnect);
  REG_CMD("C", cmdConnect);
  REG_CMD("DISCONNE", cmdDisconne);
  REG_CMD("D", cmdDisconne);
  REG_CMD("CONOK", cmdConok);
  REG_CMD("CONO", cmdConok);
  REG_CMD("RETRY", cmdRetry);
  REG_CMD("RE", cmdRetry);
  REG_CMD("FRACK", cmdFrack);
  REG_CMD("FR", cmdFrack);

  // Connect text
  REG_CMD("CTEXT", cmdCText);
  REG_CMD("CMSG", cmdCMsg);
  REG_CMD("CMS", cmdCMsg);
  REG_CMD("CMSGDISC", cmdCMsgDisc);
  REG_CMD("CMSGD", cmdCMsgDisc);

  // Beacon & unproto
  REG_CMD("BEACON", cmdBeacon);
  REG_CMD("B", cmdBeacon);
  REG_CMD("BTEXT", cmdBText);
  REG_CMD("BT", cmdBText);
  REG_CMD("UNPROTO", cmdUnproto);
  REG_CMD("U", cmdUnproto);

  // Converse & packetization
  REG_CMD("CONVERSE", cmdConverse);
  REG_CMD("CONV", cmdConverse);
  REG_CMD("K", cmdConverse);
  REG_CMD("TRANS", cmdTrans);
  REG_CMD("T", cmdTrans);
  REG_CMD("PACLEN", cmdPaclen);
  REG_CMD("P", cmdPaclen);
  REG_CMD("PACTIME", cmdPactime);
  REG_CMD("PACT", cmdPactime);
  REG_CMD("SENDPAC", cmdSendpac);
  REG_CMD("SE", cmdSendpac);
  REG_CMD("CR", cmdCR);
  REG_CMD("CONMODE", cmdConMode);
  REG_CMD("CONM", cmdConMode);
  REG_CMD("NEWMODE", cmdNewMode);
  REG_CMD("NE", cmdNewMode);
  REG_CMD("NOMODE", cmdNoMode);
  REG_CMD("NO", cmdNoMode);
  REG_CMD("CPACTIME", cmdCPacTime);
  REG_CMD("CP", cmdCPacTime);

  // Advanced protocol
  REG_CMD("TRACE", cmdTrace);
  REG_CMD("TRAC", cmdTrace);
  REG_CMD("FLOW", cmdFlow);
  REG_CMD("F", cmdFlow);
  REG_CMD("PASSALL", cmdPassAll);
  REG_CMD("PASSA", cmdPassAll);
  REG_CMD("RESPTIME", cmdRespTime);
  REG_CMD("RES", cmdRespTime);
  REG_CMD("EPATH", cmdEPath);

  // Date/Time
  REG_CMD("CONSTAMP", cmdConStamp);
  REG_CMD("CONS", cmdConStamp);
  REG_CMD("DAYSTAMP", cmdDayStamp);
  REG_CMD("DAYS", cmdDayStamp);
  REG_CMD("DAYUSA", cmdDayUsa);
  REG_CMD("DAYU", cmdDayUsa);
  REG_CMD("DAYTIME", cmdDayTime);
  REG_CMD("DA", cmdDayTime);

  // GPS/Location
  REG_CMD("LOCATION", cmdLocation);
  REG_CMD("LOC", cmdLocation);
  REG_CMD("LPATH", cmdLPath);
  REG_CMD("LPA", cmdLPath);
  REG_CMD("LTEXT", cmdLText);
  REG_CMD("LT", cmdLText);
  REG_CMD("LTMON", cmdLtMon);
  REG_CMD("LTM", cmdLtMon);

  // KISS mode
  REG_CMD("KISS", cmdKiss);

  // Utility
  REG_CMD("SEND", cmdSend);
  REG_CMD("RESTART", cmdRestart);
  REG_CMD("RESET", cmdReset);
}
