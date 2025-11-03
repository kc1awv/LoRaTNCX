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

} // namespace AX25
