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

  // ============================================================================
  // PERSISTENT SETTINGS
  // ============================================================================
  
  // Station identification
  String _myCall;
  
  // Monitoring
  bool _monitorOn = false;
  
  // Connection control
  bool _conok = true;           // accept incoming connections
  bool _cmsgOn = false;         // send CTEXT on connect
  bool _cmsgDisc = false;       // auto-disconnect after CTEXT
  String _ctext;                // connection text message
  
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
  uint32_t _lastBeaconMs = 0;
  
  // UNPROTO digipeat path
  static const int UNPROTO_MAX = 8;
  std::vector<String> _unproto;
  
  // Link parameters
  uint8_t _retry = 10;          // number of retransmits before giving up
  uint8_t _frack = 8;           // frame ack timeout in seconds
  
  // Packetization settings
  uint16_t _paclen = 256;       // max packet length
  uint32_t _pactime = 1000;     // ms timer for packetization
  char _sendpac = '\n';         // character that forces packet send in converse mode
  bool _crOn = true;            // treat CR as packet terminator in converse mode

  // ============================================================================
  // RUNTIME STATE
  // ============================================================================
  
  // Heard stations list (in-memory)
  static const int MHEARD_MAX = 32;
  String _mheard[MHEARD_MAX];
  int _mheard_count = 0;

  // L2 connection state
  enum L2State {
    L2_DISCONNECTED = 0,
    L2_CONNECTING,
    L2_CONNECTED
  };
  L2State _l2state = L2_DISCONNECTED;
  String _connectedTo;
  uint8_t _tries = 0;
  uint32_t _lastFrackMs = 0;
  
  // Multi-stream scaffold (future use)
  struct StreamState {
    L2State state = L2_DISCONNECTED;
    String connectedTo;
    uint8_t tries = 0;
    uint32_t lastFrackMs = 0;
  };
  static const int STREAMS = 10;
  StreamState _streams[STREAMS];
  int _activeStream = 0; // 0 == A

  // Converse mode buffer
  String _converseBuf;
  uint32_t _converseBufMs = 0;

  // ============================================================================
  // HELPER METHODS
  // ============================================================================
  
  // Settings persistence
  void loadSettings();
  void saveSettings();
  
  // Heard list management
  void addMHeard(const String &callsign);
  
  // Packet reception
  void onPacketReceived(const uint8_t *buf, size_t len, const AX25::AddrInfo &ai, int rssi = 0);
  
  // Connection management
  void maybeSendConnectText();
  
  // Converse mode
  void handleConverseLine(const String &line, bool endOfLine);
  void flushConverseBuffer();

  // ============================================================================
  // COMMAND HANDLERS - Core
  // ============================================================================
  void cmdHelp(const String &args);
  void cmdVersion(const String &args);
  void cmdStatus(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Radio Control
  // ============================================================================
  void cmdFreq(const String &args);
  void cmdPwr(const String &args);
  void cmdSpreadingFactor(const String &args);
  void cmdBandwidth(const String &args);
  void cmdCodingRate(const String &args);
  void cmdRadioInit(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Station & Monitoring
  // ============================================================================
  void cmdMyCall(const String &args);
  void cmdMonitor(const String &args);
  void cmdMHeard(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Connection & Link
  // ============================================================================
  void cmdConnect(const String &args);
  void cmdDisconne(const String &args);
  void cmdConok(const String &args);
  void cmdRetry(const String &args);
  void cmdFrack(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Connect Text
  // ============================================================================
  void cmdCText(const String &args);
  void cmdCMsg(const String &args);
  void cmdCMsgDisc(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Beacon & Unproto
  // ============================================================================
  void cmdBeacon(const String &args);
  void cmdBText(const String &args);
  void cmdUnproto(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Converse & Packetization
  // ============================================================================
  void cmdConverse(const String &args);
  void cmdTrans(const String &args);
  void cmdPaclen(const String &args);
  void cmdPactime(const String &args);
  void cmdSendpac(const String &args);
  void cmdCR(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Utility
  // ============================================================================
  void cmdSend(const String &args);
  void cmdRestart(const String &args);
  void cmdReset(const String &args);
  
  // ============================================================================
  // COMMAND REGISTRATION
  // ============================================================================
  void registerAllCommands();
};
