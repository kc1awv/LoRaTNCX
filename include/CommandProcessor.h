// CommandProcessor.h
#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include <string>

class LoRaRadio;

class CommandProcessor
{
public:
  using Handler = std::function<void(const String &args)>;
  // Converse handler: (text, endOfLine)
  using ConverseHandler = std::function<void(const String &text, bool endOfLine)>;

  CommandProcessor(Stream &io, LoRaRadio &radio);

  // call periodically from loop()
  void poll();

  // register a command (uppercase command name)
  void registerCommand(const char *name, Handler h);

  // print help (list of registered commands)
  void printHelp();

  // enable/disable local echo of received characters
  void setLocalEcho(bool on);

  // Modes for type-in: COMMAND, CONVERSE (line-based), TRANSPARENT (char-based)
  enum Mode { MODE_COMMAND=0, MODE_CONVERSE, MODE_TRANSPARENT };

  // set current mode
  void setMode(Mode m);

  // set handler to receive lines/characters when in CONVERSE or TRANSPARENT mode
  void setConverseHandler(ConverseHandler h);

  // set the SENDPAC trigger character (0 to disable)
  void setSendPacChar(char c);
  Mode getMode();

private:
  Stream &_io;
  LoRaRadio &_radio;
  String _line;
  std::map<String, Handler> _cmds;
  bool _localEcho = false;
  // mode state and converse handler
  Mode _mode = MODE_COMMAND;
  ConverseHandler _converseHandler = nullptr;
  char _sendPacChar = 0;

  void handleLine(const String &line);
  static String toUpper(const String &s);
};
