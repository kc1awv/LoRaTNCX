// CommandProcessor.h
#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include <string>
#include "KISSProtocol.h"

class CommandProcessor
{
public:
  using Handler = std::function<void(const String &args)>;
  // Converse handler: (text, endOfLine)
  using ConverseHandler = std::function<void(const String &text, bool endOfLine)>;

  CommandProcessor(Stream &io);

  // call periodically from loop()
  void poll();

  // register a command (uppercase command name)
  void registerCommand(const char *name, Handler h);

  // register help text for a command
  void registerCommandHelp(const char *name, const char *helpText);

  // print help (list of registered commands, or help for specific command)
  void printHelp(const String &args = "");

  // enable/disable local echo of received characters
  void setLocalEcho(bool on);

  // Modes for type-in: COMMAND, CONVERSE (line-based), TRANSPARENT (char-based), KISS (binary frames)
  enum Mode { MODE_COMMAND=0, MODE_CONVERSE, MODE_TRANSPARENT, MODE_KISS };

  // set current mode
  void setMode(Mode m);

  // set handler to receive lines/characters when in CONVERSE or TRANSPARENT mode
  void setConverseHandler(ConverseHandler h);

  // set the SENDPAC trigger character (0 to disable)
  void setSendPacChar(char c);
  Mode getMode();

  // KISS protocol support (delegated to KISSProtocol)
  void setKissFrameHandler(KISSProtocol::FrameHandler h);
  void sendKissFrame(const uint8_t *data, size_t len);
  bool isKissExitRequested() const;
  void clearKissExit();

private:
  Stream &_io;
  String _line;
  std::map<String, Handler> _cmds;
  std::map<String, String> _helpTexts;
  bool _localEcho = false;
  
  // Mode state and handlers
  Mode _mode = MODE_COMMAND;
  ConverseHandler _converseHandler = nullptr;
  char _sendPacChar = 0;

  // KISS protocol handler
  KISSProtocol _kiss;

  void handleLine(const String &line);
  static String toUpper(const String &s);
};
