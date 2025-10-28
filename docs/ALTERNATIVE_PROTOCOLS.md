# Alternative TNC Control Protocols

## Overview

This document explores alternatives to KISS protocol for host-to-TNC communication, addressing limitations encountered with the current KISS implementation.

## Current KISS Issues

1. **Limited visibility**: No built-in received packet indication
2. **No flow control**: Can overrun buffers with rapid commands
3. **Minimal error reporting**: Hard to diagnose issues
4. **No statistics**: Missing RX/TX counters, error rates
5. **Simple framing**: Vulnerable to data corruption

## Protocol Alternatives

### 1. 6PACK Protocol

**Origin**: Developed by Tomi Manninen (OH2BNS) for high-speed packet radio.

**Key Features**:
```
Frame Format: SYNC CMD SEQ DATA... CRC16
- SYNC: 0x00 (frame start marker)
- CMD: Command byte with channel and priority
- SEQ: Sequence number for flow control
- DATA: Variable length payload
- CRC16: Error detection
```

**Advantages**:
- Multiple virtual channels (0-15)
- Built-in flow control with ACK/NACK
- Priority levels for different traffic types
- CRC error detection
- Automatic retransmission on errors
- Built-in statistics and monitoring

**Implementation Complexity**: Medium-High

**Example Implementation**:
```cpp
class SixPackProtocol {
    enum Commands {
        DATA_FRAME = 0x00,
        ACK_FRAME = 0x01,
        NACK_FRAME = 0x02,
        STATUS_REQ = 0x03,
        CONFIG_REQ = 0x04
    };
    
    struct Frame {
        uint8_t sync = 0x00;
        uint8_t cmd;        // Channel (4 bits) + Command (4 bits)
        uint8_t seq;        // Sequence number
        uint8_t data[256];  // Payload
        uint16_t crc;       // CRC-16
    };
};
```

### 2. Enhanced KISS (EKISS) - Custom Protocol

**Design Goals**: Keep KISS simplicity while adding missing features.

**Frame Format**:
```
FEND CMD [PORT] [SEQ] DATA... [CRC] FEND
```

**New Command Types**:
```cpp
enum EKISSCommands {
    // Standard KISS commands (0x00-0x0F)
    DATA_FRAME = 0x00,
    TX_DELAY = 0x01,
    PERSISTENCE = 0x02,
    SLOT_TIME = 0x03,
    
    // Enhanced commands (0x10-0x1F)
    STATUS_REQUEST = 0x10,      // Get TNC status
    STATISTICS = 0x11,          // Get RX/TX stats
    ERROR_REPORT = 0x12,        // Error notification
    RX_INDICATION = 0x13,       // Received packet notification
    FLOW_CONTROL = 0x14,        // Flow control commands
    
    // Configuration commands (0x20-0x2F)
    RADIO_CONFIG = 0x20,        // Extended radio configuration
    PROTOCOL_VERSION = 0x21,    // Protocol version negotiation
};
```

**Key Enhancements**:
1. **RX Indication**: Separate command for received packets with metadata
2. **Statistics**: Built-in counters and error reporting
3. **Flow Control**: Buffer status and throttling
4. **Error Reporting**: Detailed error codes and descriptions
5. **Sequence Numbers**: Optional packet ordering

**Example RX Indication Frame**:
```
C0 13 [RSSI] [SNR] [TIMESTAMP] [DATA...] C0
```

### 3. JSON-Based Protocol

**Concept**: Use JSON messages for human-readable debugging and flexibility.

**Advantages**:
- Self-describing messages
- Easy to debug and extend
- Platform independent
- Built-in type safety

**Disadvantages**:
- Higher overhead than binary protocols
- Parsing complexity
- Not suitable for high-speed applications

**Example Messages**:
```json
// Transmit request
{
    "type": "tx_request",
    "data": "base64_encoded_packet",
    "priority": 1,
    "id": 12345
}

// Receive indication
{
    "type": "rx_indication",
    "data": "base64_encoded_packet",
    "rssi": -95,
    "snr": 8.5,
    "timestamp": 1698765432,
    "crc_valid": true
}

// Status response
{
    "type": "status",
    "frequency": 915.0,
    "tx_power": 20,
    "packets_tx": 142,
    "packets_rx": 87,
    "crc_errors": 3
}
```

### 4. HDLC-Based Protocol

**Concept**: Use HDLC framing with custom protocol on top.

**Advantages**:
- Robust framing with bit stuffing
- Built-in CRC and error detection
- Industry standard
- Good error recovery

**Disadvantages**:
- Complex bit-level operations
- Higher implementation complexity
- Overhead for simple operations

## Recommendations

### Immediate Fix for KISS Issues

Before switching protocols, try these fixes to your current KISS implementation:

1. **Add RX packet logging**:
```cpp
void onRadioFrameRx(const uint8_t *frame, size_t len, int16_t rssi, float snr) {
    // Add explicit logging
    Serial.printf("[RX-INDICATION] %d bytes, RSSI: %d dBm, SNR: %.1f dB\n", 
                  len, rssi, snr);
    
    // Your existing KISS frame transmission
    kiss.writeFrame(frame, len);
    if (kissClient.connected()) {
        kiss.writeFrameTo(kissClient, frame, len);
    }
}
```

2. **Add packet counters**:
```cpp
// In RadioHAL class
struct Statistics {
    uint32_t packets_tx = 0;
    uint32_t packets_rx = 0;
    uint32_t crc_errors = 0;
    uint32_t tx_failures = 0;
    int16_t last_rssi = -999;
    float last_snr = -99.9;
    uint32_t last_rx_timestamp = 0;
    uint32_t last_tx_timestamp = 0;
};
```

3. **Enhanced debug KISS command**:
```cpp
case KISS::SET_HARDWARE:
    switch (data[0]) {
        case 0xFF: // Debug/statistics request
            Serial.printf("[KISS-STATS] TX: %u, RX: %u, Errors: %u\n",
                         radio_stats.packets_tx, 
                         radio_stats.packets_rx, 
                         radio_stats.crc_errors);
            break;
    }
```

### Protocol Migration Strategy

If KISS continues to be problematic:

1. **Phase 1**: Enhanced KISS (EKISS) - minimal changes, maximum compatibility
2. **Phase 2**: Consider 6PACK if you need advanced features
3. **Phase 3**: Custom protocol optimized for LoRa characteristics

### For Your Specific Use Case

Given that you're building a LoRa TNC, I recommend:

1. **Short term**: Fix KISS with better debugging and statistics
2. **Medium term**: Implement Enhanced KISS (EKISS) for better visibility
3. **Long term**: Consider a LoRa-optimized protocol that accounts for:
   - Long airtime (optimize for fewer, larger packets)
   - Duty cycle limitations
   - LoRa-specific metadata (SF, BW, CR)

## Implementation Priorities

1. **Debug Current KISS Issues**:
   - Add comprehensive logging to `onRadioFrameRx()`
   - Verify interrupt handling is working
   - Check radio configuration matches between stations

2. **Enhance KISS Protocol**:
   - Add statistics reporting
   - Add RX indication frames
   - Improve error reporting

3. **Consider Alternatives**:
   - Evaluate if enhanced KISS meets needs
   - Implement 6PACK if advanced features are required
   - Design custom protocol for LoRa-specific optimizations

The key is to first ensure your radio reception is actually working, then improve the protocol layer for better visibility and control.