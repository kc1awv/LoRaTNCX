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
  // PERSISTENT SETTINGS (grouped by function)
  // ============================================================================
  
  // Station identification
  struct {
    String myCall;
    String myAlias;              // digipeater alias callsign
  } _station;
  
  // Terminal control
  struct {
    bool echo = true;            // local echo of typed characters
  } _terminal;
  
  // Monitoring settings
  struct {
    bool enabled = false;
    bool mstamp = false;         // display timestamps on monitored packets
    bool mall = true;            // monitor all stations (vs only unconnected)
    bool mcom = false;           // monitor control packets (vs only info packets)
    bool mcon = false;           // monitor while connected
    bool mrpt = true;            // display full digipeat path in monitored packets
    bool trace = false;          // display full packet hex dump
  } _monitor;
  
  // Digipeater settings
  struct {
    bool enabled = true;         // enable digipeater functionality
    bool hid = true;             // auto-ID after digipeating (every 9.5 minutes)
    uint32_t lastHidMs = 0;      // last HID transmission time
  } _digi;
  
  // Connection settings
  struct {
    bool conok = true;           // accept incoming connections
    uint8_t retry = 10;          // number of retransmits before giving up
    uint8_t frack = 8;           // frame ack timeout in seconds
    bool cmsgOn = false;         // send CTEXT on connect
    bool cmsgDisc = false;       // auto-disconnect after CTEXT
    String ctext;                // connection text message
  } _connection;
  
  // Beacon settings
  enum BeaconMode
  {
    BEACON_OFF = 0,
    BEACON_EVERY,
    BEACON_AFTER
  };
  struct {
    BeaconMode mode = BEACON_OFF;
    uint32_t interval = 0;       // seconds
    String text;
    uint32_t lastMs = 0;         // last beacon transmission time
  } _beacon;
  
  // UNPROTO digipeat path
  static const int UNPROTO_MAX = 8;
  std::vector<String> _unproto;
  
  // Packetization settings
  struct {
    uint16_t paclen = 256;       // max packet length
    uint32_t pactime = 1000;     // ms timer for packetization
    char sendpac = '\n';         // character that forces packet send in converse mode
    bool cr = true;              // treat CR as packet terminator in converse mode
    bool cpactime = false;       // when ON and in Converse mode, sends packet at PACTIME intervals
  } _packet;
  
  // Connection mode control
  enum ConMode
  {
    CONMODE_CONVERSE = 0,
    CONMODE_TRANS
  };
  struct {
    ConMode mode = CONMODE_CONVERSE;  // mode to auto-enter on connection
    bool newmode = false;        // enter mode immediately on CONNECT command
    bool nomode = false;         // when ON, disable auto-enter mode
  } _conmode;
  
  // Advanced protocol controls
  struct {
    bool flow = true;            // terminal flow control (key entry stops display)
    bool passall = false;        // when ON, accept only error frames; when OFF, reject error frames
    uint8_t resptime = 0;        // ACK response delay in 100ms units (0-250)
    std::vector<String> epath;   // digipeater path for UISSID 10 or 14
  } _protocol;
  
  // Date/Time settings (manually entered, volatile until GPS/RTC integration)
  struct {
    bool constamp = false;       // display timestamp on connect
    bool daystamp = false;       // include date with Ctrl-T in converse mode
    bool dayusa = true;          // US date format (MM/DD/YY) vs European (DD-MM-YY)
    String value;                // manually set date/time YYMMDDhhmmss (12 chars)
  } _datetime;
  
  // GPS/Location settings (manually entered, volatile until GPS integration)
  enum LocationMode
  {
    LOCATION_OFF = 0,
    LOCATION_EVERY,
    LOCATION_AFTER
  };
  struct {
    LocationMode mode = LOCATION_OFF;
    uint32_t interval = 0;       // in 10-second units (0-250)
    String lpath = "GPS";        // GPS data destination path
    String ltext;                // message in GPS data (0-159 chars)
    uint8_t ltmon = 0;           // LTEXT display interval in seconds (0-250)
    uint32_t lastMs = 0;         // last location transmission time
    uint32_t lastLtmonMs = 0;    // last LTEXT display time
  } _location;

  // ============================================================================
  // RUNTIME STATE
  // ============================================================================
  
  // Heard stations list (in-memory)
  static const int MHEARD_MAX = 32;
  String _mheard[MHEARD_MAX];
  int _mheard_count = 0;

  // ============================================================================
  // FUTURE IMPLEMENTATION: AX.25 Layer 2 Connected Mode
  // ============================================================================
  // TODO: The following structures are scaffolding for future AX.25 Layer 2
  // connected mode implementation. Currently only UNPROTO (UI frames) is
  // supported. Full L2 implementation requires:
  // - Connection establishment (SABM/SABME/UA)
  // - Window management (RR/RNR/REJ acknowledgments)
  // - Retransmission logic using _retry/_frack settings
  // - Sequence number tracking (N(S) and N(R))
  // - Segmentation/reassembly of long messages
  // - Multiple simultaneous connections (streams)
  // - Converse mode buffer accumulation and packetization
  //
  // L2 connection state (single connection - for initial implementation)
  enum L2State {
    L2_DISCONNECTED = 0,
    L2_CONNECTING,
    L2_CONNECTED
  };
  L2State _l2state = L2_DISCONNECTED;
  String _connectedTo;
  uint8_t _tries = 0;
  uint32_t _lastFrackMs = 0;
  
  // Multi-stream scaffold (10 streams: A-J, for future use)
  struct StreamState {
    L2State state = L2_DISCONNECTED;
    String connectedTo;
    uint8_t tries = 0;
    uint32_t lastFrackMs = 0;
  };
  static const int STREAMS = 10;
  StreamState _streams[STREAMS];
  int _activeStream = 0; // 0 == A

  // Converse mode buffer (accumulates text until SENDPAC or PACTIME)
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
  void maybeEnterMode();
  
  // Identification
  void sendId();
  
  // Converse mode
  void handleConverseLine(const String &line, bool endOfLine);
  void flushConverseBuffer();

  // ============================================================================
  // COMMAND HANDLERS - Core
  // ============================================================================
  void cmdHelp(const String &args);
  void cmdVersion(const String &args);
  void cmdStatus(const String &args);
  void cmdDisplay(const String &args);
  void cmdEcho(const String &args);
  
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
  void cmdMyAlias(const String &args);
  void cmdMonitor(const String &args);
  void cmdMHeard(const String &args);
  void cmdDigipeat(const String &args);
  void cmdMStamp(const String &args);
  void cmdMAll(const String &args);
  void cmdMCom(const String &args);
  void cmdMCon(const String &args);
  void cmdMRpt(const String &args);
  void cmdId(const String &args);
  void cmdHId(const String &args);
  
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
  void cmdConMode(const String &args);
  void cmdNewMode(const String &args);
  void cmdNoMode(const String &args);
  void cmdCPacTime(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Advanced Protocol
  // ============================================================================
  void cmdTrace(const String &args);
  void cmdFlow(const String &args);
  void cmdPassAll(const String &args);
  void cmdRespTime(const String &args);
  void cmdEPath(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - Date/Time
  // ============================================================================
  void cmdConStamp(const String &args);
  void cmdDayStamp(const String &args);
  void cmdDayUsa(const String &args);
  void cmdDayTime(const String &args);
  
  // ============================================================================
  // COMMAND HANDLERS - GPS/Location
  // ============================================================================
  void cmdLocation(const String &args);
  void cmdLPath(const String &args);
  void cmdLText(const String &args);
  void cmdLtMon(const String &args);
  
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
