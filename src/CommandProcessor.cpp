// CommandProcessor.cpp
#include "CommandProcessor.h"
#include "LoRaRadio.h"

CommandProcessor::CommandProcessor(Stream &io, LoRaRadio &radio)
  : _io(io), _radio(radio) {
}

void CommandProcessor::setLocalEcho(bool on) {
  _localEcho = on;
}

void CommandProcessor::registerCommand(const char* name, Handler h) {
  String k = toUpper(String(name));
  _cmds[k] = h;
}

String CommandProcessor::toUpper(const String &s) {
  String r = s;
  r.toUpperCase();
  return r;
}

void CommandProcessor::printHelp() {
  _io.println(F("Available commands:"));
  for (auto &p : _cmds) {
    _io.println(p.first);
  }
}

void CommandProcessor::handleLine(const String &line) {
  String s = line;
  s.trim();
  if (s.length() == 0) return;

  // split command and args
  int sp = s.indexOf(' ');
  String cmd = (sp < 0) ? s : s.substring(0, sp);
  String args = (sp < 0) ? String() : s.substring(sp+1);
  cmd.toUpperCase();

  // try exact match
  auto it = _cmds.find(cmd);
  if (it != _cmds.end()) {
    it->second(args);
    return;
  }

  // try prefix match (if unique)
  size_t matches = 0;
  Handler last;
  for (auto &p : _cmds) {
    if (p.first.startsWith(cmd)) {
      matches++;
      last = p.second;
    }
  }
  if (matches == 1) {
    last(args);
    return;
  }

  _io.print(F("Unknown or ambiguous command: "));
  _io.println(cmd);
}

void CommandProcessor::poll() {
  while (_io.available()) {
    char c = _io.read();
    // handle backspace/delete
    if (c == 8 || c == 127) {
      if (_line.length() > 0) {
        _line.remove(_line.length() - 1);
        if (_localEcho) _io.print("\b \b");
      }
      continue;
    }
    if (c == '\r') continue; // ignore CR
    if (c == '\n') {
      if (_localEcho) _io.print("\r\n");
      handleLine(_line);
      _line = "";
    } else {
      _line += c;
      if (_localEcho) _io.print(c);
      if (_line.length() > 512) {
        // safety - drop overly long lines
        _line = _line.substring(_line.length() - 512);
      }
    }
  }
}
