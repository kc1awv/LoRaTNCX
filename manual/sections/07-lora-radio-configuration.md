# 7. LoRa Radio Configuration

## Overview

LoRaTNCX provides extensive configuration options for the LoRa radio parameters. These settings control the fundamental characteristics of your LoRa communication link, including range, speed, reliability, and network compatibility. Understanding these parameters is crucial for optimizing your LoRa network for specific applications.

## Key LoRa Parameters

### Frequency (150-960 MHz)

The operating frequency determines which radio band your device uses. This is the most critical parameter as all devices in your network must use the same frequency.

**Default:** 915.0 MHz (US ISM band)

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --frequency 915.0`
- **Web Interface:** Frequency field in Radio Configuration section
- **KISS Protocol:** `SETHARDWARE 0x01 <frequency_bytes>` (4-byte float, little-endian)

**Regional Considerations:**
- **US/Canada:** 902-928 MHz ISM band
- **Europe:** 863-870 MHz or 433 MHz
- **Australia:** 915-928 MHz
- **Asia:** 433 MHz or 470-510 MHz

**Important:** Always verify local regulations before selecting frequencies. Some bands require licenses.

### Bandwidth (125/250/500 kHz)

Bandwidth affects data rate and receiver sensitivity. Wider bandwidth provides higher data rates but requires stronger signals.

**Default:** 125 kHz

**Available Options:**
- **125 kHz:** Best sensitivity, longest range, slower data rate
- **250 kHz:** Medium sensitivity and data rate
- **500 kHz:** Fastest data rate, shortest range

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --bandwidth 125`
- **Web Interface:** Bandwidth dropdown (125/250/500 kHz)
- **KISS Protocol:** `SETHARDWARE 0x02 <index>` (0=125kHz, 1=250kHz, 2=500kHz)

**Trade-offs:**
- Narrower bandwidth = longer range, slower speed
- Wider bandwidth = shorter range, faster speed

### Spreading Factor (7-12)

Spreading factor determines how much the signal is "spread" across time and frequency. Higher spreading factors provide better sensitivity but slower data rates.

**Default:** 12 (maximum sensitivity)

**Available Options:** 7, 8, 9, 10, 11, 12

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --spreading-factor 12`
- **Web Interface:** Spreading Factor dropdown (7-12)
- **KISS Protocol:** `SETHARDWARE 0x03 <value>` (7-12)

**Trade-offs:**
- **SF7:** Fastest (~5.5 kbps), shortest range
- **SF12:** Slowest (~293 bps), longest range (~20x SF7)

**Data Rates (approximate, 125kHz BW):**
- SF7: 5469 bps
- SF8: 3125 bps
- SF9: 1758 bps
- SF10: 977 bps
- SF11: 537 bps
- SF12: 293 bps

### Coding Rate (5-8)

Coding rate adds error correction to the data. Higher coding rates provide better reliability but reduce effective data rate.

**Default:** 7 (4/7 coding)

**Available Options:** 5, 6, 7, 8 (representing 4/5, 4/6, 4/7, 4/8)

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --coding-rate 7`
- **Web Interface:** Coding Rate dropdown (5-8)
- **KISS Protocol:** `SETHARDWARE 0x04 <value>` (5-8)

**Trade-offs:**
- Lower coding rate = higher data rate, less error correction
- Higher coding rate = lower data rate, better error correction

### Transmit Power (2-20 dBm)

Output power controls transmission strength. Higher power increases range but consumes more battery.

**Default:** 20 dBm (100 mW)

**Available Range:** 2-20 dBm (V3 board), 2-22 dBm (V4 board with PA control)

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --power 20`
- **Web Interface:** Power slider (2-20/22 dBm)
- **KISS Protocol:** `SETHARDWARE 0x05 <value>` (dBm value)

**Power Considerations:**
- **Legal Limits:** Check local regulations (US FCC: 30 dBm EIRP max for 915 MHz)
- **Battery Life:** Each 3dB increase doubles transmit power consumption
- **Heat:** High power may require cooling on V4 boards

### Sync Word (0x0000-0xFFFF)

Sync word allows multiple networks to coexist on the same frequency. Only devices with matching sync words can communicate.

**Default:** 0x1424 (private network, SX127x compatible)

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --sync-word 0x1424`
- **Web Interface:** Sync Word field (hex value)
- **KISS Protocol:** `SETHARDWARE 0x08 <sync_bytes>` (2 bytes, big-endian)

**Recommended Values:**
- **0x1424:** Private network (recommended, compatible with SX127x 0x12)
- **0x3444:** Public/LoRaWAN network (compatible with SX127x 0x34)

**Important:** All devices in your network must use the same sync word.

## Advanced Configuration

### Deaf Period (0-5000 ms)

The deaf period prevents the radio from receiving immediately after transmitting, avoiding hearing its own echo when radios are close together.

**Default:** 2000 ms

**Configuration Methods:**
- **Command Line:** `loratncx_config.py --deaf-period 2000`
- **Web Interface:** Deaf Period field (milliseconds)

**When to Adjust:**
- **Increase:** If experiencing self-interference at close range
- **Decrease:** For faster turnaround in half-duplex applications
- **Set to 0:** Disable (not recommended unless you know what you're doing)

## Configuration Examples

### Long Range, Slow Speed (Default)
```
Frequency: 915.0 MHz
Bandwidth: 125 kHz
Spreading Factor: 12
Coding Rate: 7
Power: 20 dBm
```
**Use Case:** Maximum range, weather stations, remote monitoring

### Medium Range, Balanced
```
Frequency: 915.0 MHz
Bandwidth: 250 kHz
Spreading Factor: 9
Coding Rate: 7
Power: 17 dBm
```
**Use Case:** General purpose, mobile applications

### Short Range, High Speed
```
Frequency: 915.0 MHz
Bandwidth: 500 kHz
Spreading Factor: 7
Coding Rate: 5
Power: 14 dBm
```
**Use Case:** Local networks, high-throughput applications

## Performance Optimization

### Range vs Speed Trade-offs

LoRa communication involves fundamental trade-offs between range, speed, and reliability:

1. **Maximum Range:** SF12, 125kHz BW, CR8, high power
2. **Balanced Performance:** SF9-10, 250kHz BW, CR7, medium power
3. **Maximum Speed:** SF7, 500kHz BW, CR5, any power

### Link Budget Calculation

Received signal strength depends on:
- Transmit power
- Antenna gain (transmit and receive)
- Free space path loss
- Spreading factor gain
- Coding rate overhead

**Basic Range Estimation (line-of-sight):**
- SF12, 20dBm: ~10-20 km
- SF9, 17dBm: ~2-5 km
- SF7, 14dBm: ~0.5-1 km

### Testing Your Configuration

1. **Start with defaults** for initial testing
2. **Test range** with your specific equipment and environment
3. **Monitor RSSI** (Received Signal Strength Indicator) via KISS commands
4. **Adjust parameters** based on real-world performance
5. **Document settings** for each link in your network

## Troubleshooting

### No Communication
- **Check frequency:** All devices must use identical frequency
- **Verify sync word:** Must match across all devices
- **Test power:** Ensure adequate transmit power
- **Check antennas:** Proper antenna type and connection

### Poor Range
- **Increase spreading factor:** SF12 provides maximum sensitivity
- **Reduce bandwidth:** 125kHz provides best sensitivity
- **Increase power:** If within legal limits
- **Check antenna performance:** SWR, gain, orientation

### Slow Data Rate
- **Increase bandwidth:** 500kHz provides fastest data rate
- **Reduce spreading factor:** SF7 provides fastest transmission
- **Lower coding rate:** CR5 provides highest throughput

### Interference Issues
- **Change frequency:** Move to less crowded channel
- **Adjust sync word:** Use private network sync word
- **Modify bandwidth:** Wider bandwidth may avoid narrowband interference

---

**Next:** [Section 8: WiFi and Networking](08-wifi-networking.md) | **Previous:** [Section 6: Configuration](06-configuration.md) | **Main Index:** [User Manual](../user_manual.md)