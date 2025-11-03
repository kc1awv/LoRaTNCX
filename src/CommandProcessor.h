// CommandProcessor.h
#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include <string>

class LoRaRadio;

class CommandProcessor {
public:
  using Handler = std::function<void(const String& args)>;

  CommandProcessor(Stream &io, LoRaRadio &radio);

  // call periodically from loop()
  void poll();

  // register a command (uppercase command name)
  void registerCommand(const char* name, Handler h);

  // print help (list of registered commands)
  void printHelp();

  // enable/disable local echo of received characters
  void setLocalEcho(bool on);

private:
  Stream &_io;
  LoRaRadio &_radio;
  String _line;
  std::map<String, Handler> _cmds;
  bool _localEcho = false;

  void handleLine(const String &line);
  static String toUpper(const String &s);
};
