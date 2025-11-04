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

  // AX.25 control field constants (common values)
  enum ControlField : uint8_t {
    CTL_RR = 0x01,
    CTL_RNR = 0x05,
    CTL_REJ = 0x09,
    CTL_SABM = 0x2F,
    CTL_DM = 0x0F,
    CTL_DISC = 0x43,
    CTL_UA = 0x63
  };

} // namespace AX25
