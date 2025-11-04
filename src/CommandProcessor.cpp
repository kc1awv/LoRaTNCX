// CommandProcessor.cpp
#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(Stream &io)
    : _io(io), _kiss(io)
{
}

void CommandProcessor::setLocalEcho(bool on)
{
  _localEcho = on;
}

void CommandProcessor::setMode(Mode m)
{
  _mode = m;
}

void CommandProcessor::setConverseHandler(ConverseHandler h)
{
  _converseHandler = h;
}

void CommandProcessor::setSendPacChar(char c)
{
  _sendPacChar = c;
}

CommandProcessor::Mode CommandProcessor::getMode()
{
  return _mode;
}

void CommandProcessor::registerCommand(const char *name, Handler h)
{
  String k = toUpper(String(name));
  _cmds[k] = h;
}

String CommandProcessor::toUpper(const String &s)
{
  String r = s;
  r.toUpperCase();
  return r;
}

void CommandProcessor::printHelp()
{
  _io.println(F("Available commands:"));
  for (auto &p : _cmds)
  {
    _io.println(p.first);
  }
}

void CommandProcessor::handleLine(const String &line)
{
  String s = line;
  s.trim();
  if (s.length() == 0)
    return;

  // split command and args
  int sp = s.indexOf(' ');
  String cmd = (sp < 0) ? s : s.substring(0, sp);
  String args = (sp < 0) ? String() : s.substring(sp + 1);
  cmd.toUpperCase();

  // try exact match
  auto it = _cmds.find(cmd);
  if (it != _cmds.end())
  {
    it->second(args);
    return;
  }

  // try prefix match (if unique)
  size_t matches = 0;
  Handler last;
  for (auto &p : _cmds)
  {
    if (p.first.startsWith(cmd))
    {
      matches++;
      last = p.second;
    }
  }
  if (matches == 1)
  {
    last(args);
    return;
  }

  _io.print(F("Unknown or ambiguous command: "));
  _io.println(cmd);
}

void CommandProcessor::poll()
{
  while (_io.available())
  {
    char c = _io.read();

    // Ctrl-C (ASCII 3) returns to command mode
    if (c == 3)
    {
      _mode = MODE_COMMAND;
      _io.println();
      _io.println(F("<CMD MODE>"));
      continue;
    }

    // TRANSPARENT: forward every char immediately
    if (_mode == MODE_TRANSPARENT)
    {
      if (_converseHandler)
      {
        char ch[2] = {c, 0};
        _converseHandler(String(ch), false);
      }
      if (_localEcho)
        _io.print(c);
      continue;
    }

    // CONVERSE: collect until CR/LF or SENDPAC, but detect SENDPAC mid-line and deliver immediate
    if (_mode == MODE_CONVERSE)
    {
      // handle backspace/delete
      if (c == 8 || c == 127)
      {
        if (_line.length() > 0)
        {
          _line.remove(_line.length() - 1);
          if (_localEcho)
            _io.print("\b \b");
        }
        continue;
      }

      // If this char equals SENDPAC, deliver the current line immediately (if non-empty)
      if (_sendPacChar != 0 && c == _sendPacChar)
      {
        if (_line.length() > 0)
        {
          if (_localEcho)
            _io.print("\r\n");
          if (_converseHandler)
            _converseHandler(_line, true);
          _line = "";
        }
        continue;
      }

      if (c == '\r' || c == '\n')
      {
        if (_line.length() == 0)
        {
          // ignore empty (handles CRLF)
          continue;
        }
        if (_localEcho)
          _io.print("\r\n");
        if (_converseHandler)
          _converseHandler(_line, true);
        _line = "";
        continue;
      }

      // Normal char in converse mode: accumulate but do not call handler yet
      _line += c;
      if (_localEcho)
        _io.print(c);
      if (_line.length() > 512)
      {
        _line = _line.substring(_line.length() - 512);
      }
      continue;
    }

    // KISS: binary frame protocol
    if (_mode == MODE_KISS)
    {
      _kiss.processByte(c);
      continue;
    }

    // Default: COMMAND mode behavior
    // handle backspace/delete
    if (c == 8 || c == 127)
    {
      if (_line.length() > 0)
      {
        _line.remove(_line.length() - 1);
        if (_localEcho)
          _io.print("\b \b");
      }
      continue;
    }

    // Accept both \r and \n as line terminators
    if (c == '\r' || c == '\n')
    {
      if (_line.length() == 0)
        continue;
      if (_localEcho)
        _io.print("\r\n");
      handleLine(_line);
      _line = "";
      continue;
    }

    _line += c;
    if (_localEcho)
      _io.print(c);
    if (_line.length() > 512)
    {
      _line = _line.substring(_line.length() - 512);
    }
  }
}

// ============================================================================
// KISS PROTOCOL IMPLEMENTATION
// ============================================================================

// KISS delegation methods
void CommandProcessor::setKissFrameHandler(KISSProtocol::FrameHandler h)
{
  _kiss.setFrameHandler(h);
}

void CommandProcessor::sendKissFrame(const uint8_t *data, size_t len)
{
  _kiss.sendFrame(data, len);
}

bool CommandProcessor::isKissExitRequested() const
{
  return _kiss.isExitRequested();
}

void CommandProcessor::clearKissExit()
{
  _kiss.clearExitRequest();
}


