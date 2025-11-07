// AX25.h - minimal AX.25 address parsing helpers
#pragma once

#include <Arduino.h>
#include <vector>

namespace AX25
{

  struct AddrInfo
  {
    String dest;
    String src;
    uint8_t src_ssid = 0;
    // digipeater path info
    std::vector<String> digis;      // digipeater callsigns in order
    std::vector<bool> digi_used;    // has-been-repeated flags for each digi
    int next_digi_index = -1;       // index of next unused digi (-1 if none or all used)
    // control field info (if present)
    bool hasControl = false;
    uint8_t control = 0;
    size_t header_len = 0; // bytes consumed by address+control+pid
    bool ok = false;
  };

  // Parse AX.25 addresses from buffer. Returns AddrInfo with dest/src and header length.
  // This is a minimal parser: it expects at least destination and source address fields (14 bytes).
  AddrInfo parseAddresses(const uint8_t *buf, size_t len);

  // Helper: decode a 7-byte AX.25 address field into callsign (e.g. "N0CALL-1")
  String decodeAddress(const uint8_t *addr7);

  // Encode helpers
  // Build an AX.25 UI frame (addresses + control + PID + payload + FCS)
  // dest and src are callsigns (e.g. "N0CALL" or "N0CALL-1"); returns bytes ready to transmit
  std::vector<uint8_t> encodeUIFrame(const String &dest, const String &src, const std::vector<uint8_t> &payload);
  // encode with digipeater path (addresses after src)
  std::vector<uint8_t> encodeUIFrame(const String &dest, const String &src, const std::vector<String> &digis, const std::vector<uint8_t> &payload);

  // control frame encoding (addresses + control + FCS)
  std::vector<uint8_t> encodeControlFrame(const String &dest, const String &src, uint8_t control);

  // Validation helpers
  // Validate FCS (Frame Check Sequence) of a received packet
  // Returns true if the FCS is correct (last 2 bytes match calculated CRC)
  bool validateFCS(const uint8_t *buf, size_t len);
  
  // Comprehensive packet validation - checks minimum size, FCS, and address format
  // Returns true if packet appears to be a valid AX.25 frame
  bool isValidPacket(const uint8_t *buf, size_t len, bool checkFCS = true);

  // Digipeater helpers
  // Check if a packet should be digipeated by this station
  // Returns true if myCall or myAlias matches the next unused digi in path
  bool shouldDigipeat(const AddrInfo &info, const String &myCall, const String &myAlias);
  
  // Mark the next digi as used and rebuild the packet for retransmission
  // Returns modified packet ready to send, or empty vector if error
  std::vector<uint8_t> digipeatPacket(const uint8_t *buf, size_t len, const AddrInfo &info);

  // AX.25 control field constants (common values)
  enum ControlField : uint8_t {
    // Supervisory frames (S-frames)
    CTL_RR = 0x01,    // Receive Ready
    CTL_RNR = 0x05,   // Receive Not Ready
    CTL_REJ = 0x09,   // Reject
    CTL_SREJ = 0x0D,  // Selective Reject
    
    // Unnumbered frames (U-frames)
    CTL_SABM = 0x2F,  // Set Async Balanced Mode
    CTL_SABME = 0x6F, // Set Async Balanced Mode Extended
    CTL_DM = 0x0F,    // Disconnect Mode
    CTL_DISC = 0x43,  // Disconnect
    CTL_UA = 0x63,    // Unnumbered Acknowledge
    CTL_FRMR = 0x87,  // Frame Reject
    CTL_UI = 0x03,    // Unnumbered Information
    CTL_XID = 0xAF,   // Exchange Identification
    CTL_TEST = 0xE3   // Test
  };
  
  // Frame type detection helpers
  inline bool isIFrame(uint8_t control) { return (control & 0x01) == 0; }
  inline bool isSFrame(uint8_t control) { return (control & 0x03) == 0x01; }
  inline bool isUFrame(uint8_t control) { return (control & 0x03) == 0x03; }

} // namespace AX25
