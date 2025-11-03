// AX25.cpp - minimal AX.25 parsing implementation
#include "AX25.h"
#include <vector>

// CRC-16-CCITT (poly 0x1021) implementation, initial value 0xFFFF
static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++)
  {
    crc ^= ((uint16_t)data[i]) << 8;
    for (int j = 0; j < 8; j++)
    {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}

namespace AX25
{

  static String callsignFrom6(const uint8_t *s)
  {
    // each character is left-shifted by 1 in AX.25
    char tmp[7];
    int out = 0;
    for (int i = 0; i < 6; i++)
    {
      char c = (char)(s[i] >> 1);
      tmp[out++] = c;
    }
    // trim trailing spaces
    while (out > 0 && tmp[out - 1] == ' ')
      out--;
    tmp[out] = '\0';
    return String(tmp);
  }

  String decodeAddress(const uint8_t *addr7)
  {
    String callsign = callsignFrom6(addr7);
    uint8_t ssid = (addr7[6] >> 1) & 0x0F; // bits 1..4 hold SSID
    if (ssid == 0)
      return callsign;
    // append -SSID (omit if 0)
    callsign += '-';
    callsign += String(ssid);
    return callsign;
  }

  AddrInfo parseAddresses(const uint8_t *buf, size_t len)
  {
    AddrInfo info;
    if (len < 14)
      return info; // need at least dest+src

    // each address field is 7 bytes. Determine how many address fields exist by scanning
    // for extension bit (LSB of addr[6]) set to 1.
    size_t pos = 0;
    int fields = 0;
    while (pos + 7 <= len && fields < 10)
    {
      uint8_t last = buf[pos + 6];
      fields++;
      pos += 7;
      if (last & 0x01)
        break; // extension bit set -> last address
    }
    if (fields < 2)
      return info; // invalid

    // Destination is first field, source is second
    info.dest = decodeAddress(buf);
    info.src = decodeAddress(buf + 7);
    info.src_ssid = (buf[7 + 6] >> 1) & 0x0F;

    // header length includes all address fields plus control (1) and PID (1) when present
    size_t hdr = fields * 7;
    if (len >= hdr + 2)
      hdr += 2; // control + PID
    info.header_len = hdr;
    info.ok = true;
    return info;
  }

  // pack callsign ("CALL" or "CALL-1") into 7-byte AX.25 address field
  static void packAddressField(const String &callsign, uint8_t *out, bool last)
  {
    // parse callsign and optional -SSID
    String base = callsign;
    int ssid = 0;
    int dash = callsign.indexOf('-');
    if (dash >= 0)
    {
      base = callsign.substring(0, dash);
      ssid = callsign.substring(dash + 1).toInt();
    }
    // ensure base is at most 6 chars, pad with spaces
    char b[7];
    for (int i = 0; i < 6; i++)
      b[i] = ' ';
    for (int i = 0; i < 6 && i < (int)base.length(); i++)
      b[i] = base[i];
    // left-shift each char by 1
    for (int i = 0; i < 6; i++)
      out[i] = ((uint8_t)b[i]) << 1;
    // construct SSID byte: bits 1..4 = SSID, bit0 = extension (1 if last), set commonly-used flags to 0x60
    uint8_t ssdb = (uint8_t)((ssid & 0x0F) << 1);
    ssdb |= 0x60; // set reserved/command bits per common implementations
    if (last)
      ssdb |= 0x01; // set extension bit to mark last address
    out[6] = ssdb;
  }

  std::vector<uint8_t> encodeUIFrame(const String &dest, const String &src, const std::vector<uint8_t> &payload)
  {
    std::vector<uint8_t> out;
    out.reserve(2 * 7 + 2 + payload.size() + 2);
    uint8_t a[7];
    // dest (not last)
    packAddressField(dest, a, false);
    for (int i = 0; i < 7; i++)
      out.push_back(a[i]);
    // src (last address before control)
    packAddressField(src, a, true);
    for (int i = 0; i < 7; i++)
      out.push_back(a[i]);
    // Control (UI frame) and PID (no layer 3)
    out.push_back(0x03);
    out.push_back(0xF0);
    // payload
    for (auto b : payload)
      out.push_back(b);
    // compute FCS (CRC-16-CCITT) and append low byte then high byte
    uint16_t crc = crc16_ccitt(out.data(), out.size());
    out.push_back((uint8_t)(crc & 0xFF));
    out.push_back((uint8_t)((crc >> 8) & 0xFF));
    return out;
  }

  std::vector<uint8_t> encodeUIFrame(const String &dest, const String &src, const std::vector<String> &digis, const std::vector<uint8_t> &payload)
  {
    std::vector<uint8_t> out;
    size_t addrCount = 2 + digis.size();
    out.reserve(addrCount * 7 + 2 + payload.size() + 2);
    uint8_t a[7];
    // build address list: dest, src, then digis
    // dest
    packAddressField(dest, a, false);
    for (int i = 0; i < 7; i++)
      out.push_back(a[i]);
    // src: not last if there are digis
    bool srcLast = digis.size() == 0;
    packAddressField(src, a, srcLast);
    for (int i = 0; i < 7; i++)
      out.push_back(a[i]);
    // digis
    for (size_t di = 0; di < digis.size(); di++)
    {
      bool last = (di == digis.size() - 1);
      packAddressField(digis[di], a, last);
      for (int i = 0; i < 7; i++)
        out.push_back(a[i]);
    }

    // Control (UI frame) and PID (no layer 3)
    out.push_back(0x03);
    out.push_back(0xF0);
    // payload
    for (auto b : payload)
      out.push_back(b);
    // compute FCS (CRC-16-CCITT) and append low byte then high byte
    uint16_t crc = crc16_ccitt(out.data(), out.size());
    out.push_back((uint8_t)(crc & 0xFF));
    out.push_back((uint8_t)((crc >> 8) & 0xFF));
    return out;
  }

} // namespace AX25
