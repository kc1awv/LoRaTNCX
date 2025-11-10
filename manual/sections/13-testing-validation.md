# 13. Testing and Validation

## Overview

LoRaTNCX includes comprehensive testing tools and procedures to validate functionality, performance, and reliability. This section covers automated testing, interactive validation, range testing, and performance metrics to ensure your LoRaTNCX deployment works correctly.

## Automated Test Suite

### Bidirectional Communication Test

The primary test suite validates end-to-end communication between two LoRaTNCX devices using the KISS protocol.

**Requirements:**
- Two LoRaTNCX devices (any combination of V3/V4)
- Two USB serial connections
- Python 3 with pyserial library
- Devices within 10 meters for initial testing

**Installation:**
```bash
# Install required Python packages
pip3 install pyserial

# Make test script executable
chmod +x test/kiss_test.py
```

**Basic Test Run:**
```bash
# Test communication between two devices
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0
```

**Expected Output:**
```
============================================================
LoRaTNCX Bidirectional Test Suite
============================================================
Opening TNC1 on /dev/ttyUSB0...
✓ TNC1 opened successfully
Opening TNC2 on /dev/ttyACM0...
✓ TNC2 opened successfully

Reading configurations...
TNC1 Configuration:
  Frequency: 433.775 MHz
  Bandwidth: 125.0 kHz
  Spreading Factor: SF12
  Coding Rate: 4/7
  Output Power: 20 dBm
  Sync Word: 0x1424

TNC2 Configuration:
  Frequency: 433.775 MHz
  Bandwidth: 125.0 kHz
  Spreading Factor: SF12
  Coding Rate: 4/7
  Output Power: 20 dBm
  Sync Word: 0x1424

Test 1: TNC1 → TNC2
----------------------------------------------------------------------
Sending: 'Hello from TNC1 at 14:30:22'
Listening on TNC2...
✓ Received 1 frame(s)
  Frame 1: 'Hello from TNC1 at 14:30:22'
  ✓ Message matches!

Test 2: TNC2 → TNC1
----------------------------------------------------------------------
Sending: 'Hello from TNC2 at 14:30:25'
Listening on TNC1...
✓ Received 1 frame(s)
  Frame 1: 'Hello from TNC2 at 14:30:25'
  ✓ Message matches!

Test 3: Rapid Fire Test
----------------------------------------------------------------------
Sending 5 rapid messages from TNC1...
✓ All 5 messages received on TNC2

============================================================
TEST RESULTS: PASSED (3/3 tests)
============================================================
```

### Configuration Validation

**Verify Device Configuration:**
```bash
# Check configuration using command-line tool
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config
```

**Expected Configuration Match:**
```
============================================================
LoRaTNCX Current Configuration
============================================================
  Frequency:        433.775 MHz
  Bandwidth:        125.0 kHz
  Spreading Factor: SF12
  Coding Rate:      4/7
  Output Power:     20 dBm
  Sync Word:        0x1424
============================================================
```

### Hardware Diagnostics

**Battery Voltage Check (V3 only):**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-battery
```

**Board Information:**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-board
```

**GNSS Status (V4 only):**
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-gnss
```

## Interactive Testing

### Chat Mode Testing

For manual testing and experimentation, use interactive mode:

```bash
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0 --interactive
```

**Interactive Commands:**
```
Interactive Mode - Type commands:
  1:message    Send from TNC1
  2:message    Send from TNC2
  r            Check for received messages
  q            Quit

LoRaTNCX> 1:Hello from device 1!
LoRaTNCX> r
Checking for messages...
TNC2 received: Hello from device 1!
LoRaTNCX> 2:Reply from device 2!
LoRaTNCX> r
Checking for messages...
TNC1 received: Reply from device 2!
```

### Serial Monitor Testing

**Monitor Raw KISS Traffic:**
```bash
# Monitor TNC1
screen /dev/ttyUSB0 115200

# Monitor TNC2 (in another terminal)
screen /dev/ttyACM0 115200
```

**Send Test Frames Manually:**
```bash
# Send a test frame (FEND + data + FEND)
echo -e '\xc0Hello World\xc0' > /dev/ttyUSB0
```

### Web Interface Testing

**Access Web Interface:**
1. Connect to LoRaTNCX WiFi (AP mode)
2. Open browser to `http://192.168.4.1`
3. Navigate through all tabs:
   - Status Dashboard
   - LoRa Configuration
   - WiFi Configuration
   - GNSS Configuration (V4)

**Test REST API:**
```bash
# Get current status
curl http://192.168.4.1/api/status

# Get LoRa configuration
curl http://192.168.4.1/api/lora/config

# Test configuration update
curl -X POST http://192.168.4.1/api/lora/config \
  -H "Content-Type: application/json" \
  -d '{"frequency": 433.775, "spreadingFactor": 12}'
```

## Range Testing

### Setup for Range Testing

**Equipment Needed:**
- Two LoRaTNCX devices
- External antennas (recommended for best range)
- GPS coordinates for distance measurement
- Notebook for logging results

**Pre-Test Configuration:**
```bash
# Set both devices to same parameters
python3 tools/loratncx_config.py /dev/ttyUSB0 --frequency 433.775 --spreading-factor 12 --bandwidth 125
python3 tools/loratncx_config.py /dev/ttyACM0 --frequency 433.775 --spreading-factor 12 --bandwidth 125

# Save configuration
python3 tools/loratncx_config.py /dev/ttyUSB0 --save
python3 tools/loratncx_config.py /dev/ttyACM0 --save
```

### Range Test Procedure

**Step 1: Baseline Test (10 meters)**
```bash
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0
# Should pass all tests
```

**Step 2: Incremental Distance Testing**
- Start at 100 meters
- Send test messages every 50 meters
- Record success rate and signal quality
- Note terrain, obstacles, and weather conditions

**Step 3: Maximum Range Test**
- Continue until communication fails
- Try different locations and times
- Test with different spreading factors

### Range Test Data Collection

**Test Log Template:**
```
Date/Time: 2025-11-10 14:30:00
Location: Grid EM00aa
Device 1: V4, Antenna: Diamond RH795 (5dBi)
Device 2: V3, Antenna: Stock helical
Frequency: 433.775 MHz
SF: 12, BW: 125kHz, CR: 4/7

Distance (km) | Success Rate | Notes
--------------|--------------|--------
0.01          | 100% (10/10) | Line of sight
0.1           | 100% (10/10) | Line of sight
0.5           | 95% (19/20)  | Light tree cover
1.0           | 80% (16/20)  | Rolling terrain
2.0           | 60% (12/20)  | Hill between stations
5.0           | 20% (4/20)   | Mountain ridge
```

### Environmental Factors

**Line-of-Sight Testing:**
- Best case: Direct visual contact, elevated positions
- Typical range: 5-20 km with good antennas
- Record elevation profiles and Fresnel zone clearance

**Obstructed Testing:**
- Urban environments: 1-5 km typical
- Forested areas: 0.5-2 km typical
- Indoor testing: 10-100 meters through walls

**Weather Impact:**
- Rain/fog: Minimal impact on LoRa signals
- Temperature: -40°C to +85°C operational range
- Humidity: No significant effect

## Performance Metrics

### Radio Performance Metrics

**Key Performance Indicators:**

**Time on Air (ToA):**
- SF7, 125kHz: ~50ms for 100 bytes
- SF12, 125kHz: ~2000ms for 100 bytes
- Formula: ToA = (2^SF / BW) * (4/(4+CR)) * (payload_bytes + 8)

**Throughput:**
- Effective data rate depends on SF and payload size
- SF7: ~5-10 kbps effective
- SF12: ~0.2-0.5 kbps effective
- Includes protocol overhead and duty cycle limits

**Receive Sensitivity:**
- SX1262 typical: -148 dBm (SF12, 125kHz)
- Real-world: -135 to -140 dBm with antennas
- Affects maximum range capability

### Latency Measurements

**Round-Trip Time (RTT):**
```bash
# Measure latency with ping test
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0 --latency-test
```

**Typical Latency:**
- SF7: 100-200ms RTT
- SF12: 4-6 seconds RTT
- Includes transmit time, propagation delay, and processing time

### Packet Success Rate (PSR)

**Measuring PSR:**
```bash
# Send 100 test packets and measure success rate
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0 --psr-test 100
```

**PSR Factors:**
- Distance and signal strength
- Interference from other transmitters
- Spreading factor and coding rate
- Antenna quality and placement
- Duty cycle limitations

### Battery Life Testing

**V3 Battery Testing:**
```bash
# Monitor battery voltage during operation
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-battery --monitor
```

**Power Consumption:**
- Receive mode: ~80-100mA
- Transmit mode: 150-300mA (depending on power setting)
- Sleep mode: <1mA
- GNSS active: +25-30mA

**Battery Life Calculation:**
```
Life_hours = (battery_mAh * 0.8) / average_current_mA
# 0.8 = 80% usable capacity
# Example: 2000mAh battery, 100mA average = 16 hours
```

### Error Rate Testing

**Bit Error Rate (BER) Testing:**
```bash
# Test at various signal levels
python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0 --ber-test
```

**Packet Error Rate (PER):**
- Measure percentage of lost or corrupted packets
- Target: <1% PER for reliable communication
- Acceptable: <5% PER for most applications

## Factory Test Procedures

### V4 Factory Test

**Hardware Test Sequence:**
1. **OLED Display Test:** Shows test progress and results
2. **Serial Communication:** Verifies USB/serial interface
3. **LED Test:** Blinks onboard LEDs
4. **WiFi Test:** Connects to network and scans for access points
5. **LoRa Test:** Ping-pong communication test
6. **Timer Test:** Validates timing functions
7. **GNSS Test:** Checks GPS/GLONASS/BDS/QZSS reception

**Running Factory Test:**
```bash
# Upload factory test firmware
pio run --target upload --environment heltec_wifi_lora_32_V4_factory_test

# Monitor test output
pio device monitor
```

### V3 Factory Test

**V3 Test Features:**
- Similar to V4 but without GNSS testing
- Focuses on LoRa, WiFi, and display functions
- Battery voltage monitoring
- External antenna testing

### Test Result Interpretation

**Pass Criteria:**
- ✅ All hardware interfaces functional
- ✅ LoRa communication successful
- ✅ WiFi connectivity working
- ✅ Display and LEDs operational
- ✅ GNSS fix achieved (V4)

**Common Failure Points:**
- Antenna not connected (GNSS test fails)
- Power supply issues (battery test fails)
- LoRa module not responding (communication test fails)
- WiFi interference (connection test fails)

## Troubleshooting Failed Tests

### Communication Test Failures

**No Messages Received:**
```bash
# Check device configurations match
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config
python3 tools/loratncx_config.py /dev/ttyACM0 --get-config

# Verify frequency, SF, BW, CR, sync word are identical
```

**One Direction Works:**
- Check PA control on V4 transmitter
- Verify antenna connections
- Test with devices swapped
- Check for interference on frequency

**Garbled Messages:**
- Reduce distance between devices
- Change to less crowded frequency
- Verify sync word matches
- Check for baud rate issues

### Serial Connection Issues

**Permission Denied:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Logout and login again

# Or run with sudo
sudo python3 test/kiss_test.py /dev/ttyUSB0 /dev/ttyACM0
```

**Device Not Found:**
```bash
# List available ports
ls -l /dev/tty{USB,ACM,S}*

# Check dmesg for device detection
dmesg | tail -20

# Try different USB ports/cables
```

### Web Interface Issues

**Cannot Access Web Interface:**
```bash
# Check WiFi connection
nmcli device wifi list

# Verify IP address
curl http://192.168.4.1/api/status

# Check firewall settings
sudo ufw status
```

**API Returns Errors:**
```bash
# Check JSON format
curl -v http://192.168.4.1/api/lora/config

# Verify firmware version
curl http://192.168.4.1/api/status | jq .version
```

## Performance Optimization

### Parameter Tuning

**Range vs Speed Trade-offs:**
```bash
# Long range, slow (SF12)
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 12 --bandwidth 62.5

# Short range, fast (SF7)
python3 tools/loratncx_config.py /dev/ttyUSB0 --spreading-factor 7 --bandwidth 250
```

**Coding Rate Optimization:**
- CR 4/5: Faster, less robust
- CR 4/8: Slower, more robust
- Default CR 4/7: Good balance

### Duty Cycle Management

**LoRa Duty Cycle Limits:**
- 433 MHz: 10% duty cycle (ETSI)
- 868 MHz: 1% duty cycle (ETSI)
- 915 MHz: 1% duty cycle (FCC)

**Managing Duty Cycle:**
```bash
# Calculate safe transmit interval
# For 10% duty cycle, ToA = 100ms
# Safe interval = ToA * 9 = 900ms between transmissions
```

### Interference Mitigation

**Frequency Hopping:**
```bash
# Use different sync words for multiple networks
python3 tools/loratncx_config.py /dev/ttyUSB0 --syncword 0x1425
python3 tools/loratncx_config.py /dev/ttyACM0 --syncword 0x1426
```

**Channel Monitoring:**
```bash
# Monitor for interference
python3 test/kiss_test.py /dev/ttyUSB0 --monitor-mode
```

## Validation Checklists

### Pre-Deployment Checklist

- [ ] Automated test suite passes
- [ ] Configuration verified on both devices
- [ ] Antennas properly connected and oriented
- [ ] Power sources stable and adequate
- [ ] Serial connections reliable
- [ ] Web interface accessible
- [ ] GNSS functioning (V4)

### Post-Deployment Validation

- [ ] Range testing completed at target distances
- [ ] Packet success rate >95% under normal conditions
- [ ] Latency within acceptable limits
- [ ] Battery life meets requirements
- [ ] Interference levels acceptable
- [ ] Duty cycle compliance verified

### Maintenance Testing

**Monthly Validation:**
- Run automated test suite
- Check battery voltage and health
- Verify antenna connections
- Update firmware if needed
- Backup configuration settings

**Annual Testing:**
- Complete range survey
- Antenna system inspection
- Battery replacement if needed
- Performance benchmarking

---

**Next:** [Section 14: Troubleshooting](14-troubleshooting.md) | **Previous:** [Section 12: Applications and Integration](12-applications-integration.md) | **Main Index:** [User Manual](../user_manual.md)