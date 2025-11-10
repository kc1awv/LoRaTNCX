# 15. Advanced Topics

## Overview

This section covers advanced LoRaTNCX features and optimization techniques for experienced users. Topics include custom network configuration, power management, multi-client operation, and performance tuning.

## Custom Sync Words

### Understanding Sync Words

Sync words are special bit sequences used to identify valid LoRa transmissions. They act like network IDs, allowing multiple independent LoRa networks to coexist on the same frequency without interference.

**Default Sync Word:** `0x1424` (compatible with SX127x `0x12`)

**Purpose:**
- Network isolation (prevent cross-network interference)
- Coexistence with other LoRa devices
- Private network security

### Configuring Custom Sync Words

**Via Command-Line Tool:**
```bash
# Set custom sync word (16-bit value)
python3 tools/loratncx_config.py /dev/ttyACM0 --syncword 0xABCD

# Save configuration
python3 tools/loratncx_config.py /dev/ttyACM0 --save
```

**Via KISS Protocol:**
```python
import serial
import struct

# Open serial connection
ser = serial.Serial('/dev/ttyACM0', 115200)

# Set sync word to 0xABCD
# KISS frame: FEND + CMD_SETHARDWARE + HW_SET_SYNCWORD + syncword_bytes + FEND
syncword = 0xABCD
frame = bytes([0xC0, 0x06, 0x08]) + struct.pack('<H', syncword) + bytes([0xC0])
ser.write(frame)
```

**Via Web Interface:**
1. Navigate to LoRa Configuration page
2. Enter sync word in hex format (e.g., ABCD)
3. Click "Save Configuration"

### Sync Word Best Practices

**Choosing Sync Words:**
- Use values between `0x0001` and `0xFFFF`
- Avoid `0x0000` (reserved)
- Avoid `0x1424` if operating near other LoRa devices
- Document your network's sync word for sharing

**Network Planning:**
```bash
# Example: Different sync words for different applications
# Emergency communications: 0xE911
# Amateur radio: 0xA1A1
# IoT sensors: 0x10T1
# Experimental: 0x1234
```

**Compatibility Notes:**
- SX126x uses 16-bit sync words (0x0000-0xFFFF)
- SX127x uses 8-bit sync words (0x00-0xFF), but 0x1424 maps to 0x12
- Ensure all devices in your network use identical sync words

### Troubleshooting Sync Word Issues

**Symptoms of Wrong Sync Word:**
- Packets not received despite correct frequency/SF/BW
- One-way communication (transmit works, receive doesn't)
- High packet loss rate

**Verification:**
```bash
# Check current sync word
python3 tools/loratncx_config.py /dev/ttyACM0 --get-config

# Test with known good sync word
python3 tools/loratncx_config.py /dev/ttyACM0 --syncword 0x1424
python3 tools/loratncx_config.py /dev/ttyACM0 --save
```

## Power Management

### Display Power Control

**Automatic Power Save:**
The OLED display includes intelligent power management:

- **Active Mode:** Display updates continuously when in use
- **Power Save Mode:** Display blanks after period of inactivity
- **Deep Sleep:** Long button press initiates complete system shutdown

**Display Control:**
```cpp
// Display automatically enters power save mode
// Wake with button press or system activity

// Manual control via code
displayManager.displayOff();  // Power save mode
displayManager.displayOn();   // Active mode
```

**Power Consumption:**
- **Display Active:** ~15-20mA additional current
- **Display Off:** ~1-2mA (power save mode)
- **Deep Sleep:** <1mA (complete shutdown)

### Radio Power Optimization

**Adaptive Power Control:**
```bash
# Set appropriate power for range requirements
# Short range (100m): 5-10dBm
python3 tools/loratncx_config.py /dev/ttyACM0 --power 10

# Medium range (1km): 14-17dBm
python3 tools/loratncx_config.py /dev/ttyACM0 --power 17

# Long range (10km+): 20dBm (maximum)
python3 tools/loratncx_config.py /dev/ttyACM0 --power 20
```

**Power vs Performance Trade-offs:**
- Higher power increases range but reduces battery life
- Lower power improves efficiency but reduces reliability
- Consider local regulations (ETSI, FCC power limits)

### System Sleep Modes

**Deep Sleep Implementation:**
```cpp
// Initiated by long button press on display
// Complete system shutdown with ESP32 deep sleep

// Recovery requires physical reset or power cycle
// All configurations preserved in flash memory
```

**Sleep Mode Considerations:**
- GNSS module powers down automatically
- WiFi disconnects during sleep
- Serial connections terminate
- Recovery time: ~2 seconds from reset

## Battery Operation

### V3 Battery Monitoring

**Hardware Differences:**
- **V3.2+ Boards:** Standard battery voltage reading
- **Original V3 Boards:** Inverted logic requires configuration change

**Configuration for Original V3 Boards:**
```ini
# platformio.ini for original V3 boards
[env:heltec_wifi_lora_32_V3]
board = heltec_wifi_lora_32_V3
board_build.filesystem = spiffs
build_flags = 
    ${env.build_flags}
    -DARDUINO_HELTEC_WIFI_LORA_32_V3
    -DV3_ORIGINAL_BATTERY_LOGIC  # Required for original V3 boards
```

**Battery Voltage Reading:**
```bash
# Check battery voltage
python3 tools/loratncx_config.py /dev/ttyACM0 --get-battery

# Output format:
# Battery Voltage: 3.85V (78% estimated)
```

### Battery Life Optimization

**Power Consumption Breakdown:**
- **ESP32 Active:** 80-100mA
- **LoRa Receive:** +20-30mA
- **LoRa Transmit:** +150-300mA (depends on power setting)
- **GNSS Active:** +25-30mA
- **Display Active:** +15-20mA
- **WiFi Active:** +100-150mA

**Battery Life Calculations:**
```python
# Example calculation for 2000mAh LiPo battery
BATTERY_CAPACITY = 2000  # mAh
BATTERY_VOLTAGE = 3.7    # V

def calculate_life(current_ma, efficiency=0.8):
    """Calculate battery life in hours"""
    available_capacity = BATTERY_CAPACITY * efficiency
    life_hours = available_capacity / current_ma
    return life_hours

# Receive mode (100mA average)
rx_life = calculate_life(100)  # ~16 hours

# With GNSS (125mA average)
gnss_life = calculate_life(125)  # ~12.8 hours

# Transmit duty cycle (20% TX at 200mA, 80% RX at 100mA)
avg_tx_current = 0.2 * 200 + 0.8 * 100  # 120mA
tx_life = calculate_life(120)  # ~13.3 hours
```

### Battery Health Monitoring

**Voltage Ranges:**
- **4.2V:** Fully charged (100%)
- **3.7V:** Nominal voltage (50%)
- **3.3V:** Discharge cutoff (0%)
- **<3.0V:** Deep discharge (damage risk)

**Health Indicators:**
- **Capacity Loss:** Reduced runtime over time
- **Voltage Drop:** Excessive sag under load
- **Internal Resistance:** Increased resistance causes heating

**Maintenance:**
- Store at 3.7-3.8V when not in use
- Avoid deep discharge cycles
- Use proper LiPo charger with balancing
- Replace after 300-500 charge cycles

### External Power Options

**USB Power Bank:**
- Provides 5V regulated power
- Enables continuous operation
- Consider power bank capacity and output current

**Solar Power:**
- Small solar panels (5-10W) for remote operation
- Charge controller required for LiPo safety
- Weather-dependent reliability
- Backup battery for nighttime operation

## Multiple TNC Networks

### TCP KISS Multi-Client Support

**Client Capacity:**
- **Maximum Clients:** 4 simultaneous TCP connections
- **Port:** 8001 (default, configurable)
- **Protocol:** Standard KISS over TCP

**Multi-Client Operation:**
```bash
# Client 1: Dire Wolf iGate
direwolf -c direwolf.conf

# Client 2: APRS Client
yacc

# Client 3: Packet Terminal
minicom -D 192.168.4.1:8001

# Client 4: Custom Application
python3 my_kiss_client.py
```

**Client Management:**
- All clients receive identical packet streams
- No client prioritization (first-come, first-served)
- Automatic cleanup of disconnected clients
- No authentication or access control

### Network Architecture Options

**Single TNC, Multiple Applications:**
```
Internet ↔ Router ↔ LoRaTNCX ↔ Multiple Clients
                              ↘ Dire Wolf (iGate)
                              ↘ YAAC (APRS)
                              ↘ BPQ32 (BBS)
                              ↘ Custom App
```

**Distributed TNC Network:**
```
TNC1 (433.775 MHz) ↔ Client Apps
TNC2 (868.000 MHz) ↔ Client Apps
TNC3 (915.000 MHz) ↔ Client Apps
```

**Redundant TNC Setup:**
```
Primary TNC ↔ Client Apps
Backup TNC ↔ (Standby mode)
```

### Client Isolation

**Sync Word Separation:**
```bash
# Different applications use different sync words
# APRS Network: 0x1424
python3 tools/loratncx_config.py /dev/ttyACM0 --syncword 0x1424

# IoT Network: 0xABCD
python3 tools/loratncx_config.py /dev/ttyACM1 --syncword 0xABCD

# Experimental: 0x1234
python3 tools/loratncx_config.py /dev/ttyACM2 --syncword 0x1234
```

**Frequency Separation:**
```bash
# Different applications on different frequencies
# APRS: 433.775 MHz
python3 tools/loratncx_config.py /dev/ttyACM0 --frequency 433.775

# LoRaWAN: 868.100 MHz
python3 tools/loratncx_config.py /dev/ttyACM1 --frequency 868.100

# Custom: 914.500 MHz
python3 tools/loratncx_config.py /dev/ttyACM2 --frequency 914.500
```

### Load Balancing

**Client Distribution:**
- Spread clients across multiple TNC instances
- Use different ports for different applications
- Implement client-side load balancing

**Performance Considerations:**
- Each client adds processing overhead
- TCP buffering may delay packet delivery
- Monitor CPU usage with many active clients

## Performance Optimization

### LoRa Parameter Tuning

**Range vs Speed Optimization:**

**Maximum Range Configuration:**
```bash
# Slow but long range
python3 tools/loratncx_config.py /dev/ttyACM0 \
  --spreading-factor 12 \
  --bandwidth 7.8 \
  --coding-rate 8 \
  --power 20
# Time on Air: ~30 seconds for 100 bytes
# Range: 10-20 km (line of sight)
```

**Balanced Configuration:**
```bash
# Good balance of speed and range
python3 tools/loratncx_config.py /dev/ttyACM0 \
  --spreading-factor 9 \
  --bandwidth 125 \
  --coding-rate 7 \
  --power 17
# Time on Air: ~1 second for 100 bytes
# Range: 2-5 km (line of sight)
```

**Maximum Speed Configuration:**
```bash
# Fast but short range
python3 tools/loratncx_config.py /dev/ttyACM0 \
  --spreading-factor 7 \
  --bandwidth 250 \
  --coding-rate 5 \
  --power 10
# Time on Air: ~100ms for 100 bytes
# Range: 0.5-1 km (line of sight)
```

### Bandwidth Optimization

**Bandwidth Selection Guide:**
- **7.8 kHz:** Maximum range, minimum data rate, high immunity to interference
- **10.4 kHz:** Good range, moderate data rate
- **15.6 kHz:** Balanced performance
- **20.8 kHz:** Higher data rate, reduced range
- **31.25 kHz:** Fast transmission, shorter range
- **41.7 kHz:** Very fast, limited range
- **62.5 kHz:** High speed, urban ranges
- **125 kHz:** Maximum speed, short range
- **250 kHz:** Ultra-fast, very short range

**Adaptive Bandwidth:**
```bash
# Urban environment (interference): Use narrow bandwidth
python3 tools/loratncx_config.py /dev/ttyACM0 --bandwidth 31.25

# Rural environment (clean spectrum): Use wider bandwidth
python3 tools/loratncx_config.py /dev/ttyACM0 --bandwidth 125
```

### Coding Rate Optimization

**CR Selection:**
- **4/5:** Fastest transmission, least robust
- **4/6:** Good balance
- **4/7:** Default, good robustness
- **4/8:** Maximum robustness, slowest transmission

**Error Environment Adaptation:**
```bash
# Clean RF environment
python3 tools/loratncx_config.py /dev/ttyACM0 --coding-rate 5

# Noisy RF environment
python3 tools/loratncx_config.py /dev/ttyACM0 --coding-rate 8
```

### Duty Cycle Management

**Regulatory Compliance:**
```python
# Calculate safe duty cycle
def calculate_duty_cycle(toa_ms, interval_ms):
    """Calculate duty cycle percentage"""
    return (toa_ms / interval_ms) * 100

# Example: SF12, 100 byte packet = ~2000ms ToA
# For 10% duty cycle: interval = 20000ms (20 seconds)
duty_cycle = calculate_duty_cycle(2000, 20000)  # 10%
```

**Transmission Scheduling:**
```python
import time
import random

class DutyCycleManager:
    def __init__(self, max_duty_cycle=0.1, measurement_period=3600):
        self.max_duty_cycle = max_duty_cycle
        self.period = measurement_period
        self.transmissions = []
    
    def can_transmit(self, toa_ms):
        """Check if transmission is allowed under duty cycle limits"""
        now = time.time()
        
        # Remove old transmissions outside measurement period
        cutoff = now - self.period
        self.transmissions = [t for t in self.transmissions if t > cutoff]
        
        # Calculate current duty cycle
        total_toa = len(self.transmissions) * (toa_ms / 1000)  # Convert to seconds
        current_duty_cycle = total_toa / self.period
        
        return current_duty_cycle + (toa_ms / 1000) / self.period <= self.max_duty_cycle
    
    def record_transmission(self, toa_ms):
        """Record a transmission for duty cycle tracking"""
        self.transmissions.append(time.time())

# Usage
manager = DutyCycleManager(max_duty_cycle=0.1)  # 10% duty cycle

if manager.can_transmit(2000):  # 2 second ToA
    # Transmit packet
    transmit_packet()
    manager.record_transmission(2000)
else:
    # Wait before next transmission attempt
    time.sleep(10)
```

### Interference Mitigation

**Frequency Hopping:**
```python
import random

class FrequencyHopper:
    def __init__(self, base_freq=433.775, hop_range=1.0, channels=10):
        self.base_freq = base_freq
        self.hop_range = hop_range
        self.channels = channels
        self.current_channel = 0
        
    def get_next_frequency(self):
        """Get next frequency in hop sequence"""
        # Simple sequential hopping
        self.current_channel = (self.current_channel + 1) % self.channels
        freq_offset = (self.current_channel / self.channels) * self.hop_range
        return self.base_freq + freq_offset
    
    def randomize_hop(self):
        """Random frequency hopping"""
        freq_offset = random.uniform(0, self.hop_range)
        return self.base_freq + freq_offset

# Usage
hopper = FrequencyHopper(base_freq=433.775, hop_range=1.0, channels=10)

# Set next hop frequency
next_freq = hopper.get_next_frequency()
run_command(f"loratncx_config --frequency {next_freq}")
```

**Channel Monitoring:**
```bash
# Monitor RSSI on different frequencies
python3 tools/loratncx_config.py /dev/ttyACM0 --monitor-rssi

# Scan for interference sources
# Use spectrum analyzer or SDR to identify busy frequencies
```

### Throughput Optimization

**Packet Size Optimization:**
```python
# Calculate optimal packet size for given parameters
def calculate_optimal_packet_size(sf, bw, cr, target_toa=1000):
    """Calculate optimal packet size for target time on air"""
    # Simplified calculation
    # Actual formula involves detailed LoRa timing calculations
    base_toa = 100  # ms for 10 bytes at SF7, 125kHz
    size_factor = 2 ** sf * (4 / (4 + cr - 4))  # Simplified
    optimal_size = (target_toa / base_toa) / size_factor * 10
    return min(optimal_size, 236)  # KISS frame limit

# Example: SF12, 125kHz, CR 4/7, target 1 second
packet_size = calculate_optimal_packet_size(12, 125, 7, 1000)
print(f"Optimal packet size: {packet_size} bytes")
```

**Buffering Strategies:**
- Implement application-level buffering
- Use appropriate timeouts for packet assembly
- Consider compression for text-heavy protocols

### Advanced Monitoring

**Performance Metrics Collection:**
```python
class LoRaPerformanceMonitor:
    def __init__(self):
        self.packets_sent = 0
        self.packets_received = 0
        self.bytes_sent = 0
        self.bytes_received = 0
        self.errors = 0
        self.start_time = time.time()
    
    def record_send(self, bytes_count):
        self.packets_sent += 1
        self.bytes_sent += bytes_count
    
    def record_receive(self, bytes_count):
        self.packets_received += 1
        self.bytes_received += bytes_count
    
    def record_error(self):
        self.errors += 1
    
    def get_stats(self):
        elapsed = time.time() - self.start_time
        return {
            'packets_sent': self.packets_sent,
            'packets_received': self.packets_received,
            'bytes_sent': self.bytes_sent,
            'bytes_received': self.bytes_received,
            'errors': self.errors,
            'packet_loss_rate': (self.errors / max(1, self.packets_sent)) * 100,
            'throughput_bps': self.bytes_sent / elapsed,
            'uptime_seconds': elapsed
        }

# Usage
monitor = LoRaPerformanceMonitor()

# Record packet events
monitor.record_send(100)  # Sent 100 bytes
monitor.record_receive(95)  # Received 95 bytes (some loss)

# Get performance stats
stats = monitor.get_stats()
print(f"Packet loss rate: {stats['packet_loss_rate']:.1f}%")
print(f"Throughput: {stats['throughput_bps']:.1f} bps")
```

---

**Next:** [Section 16: Development and Customization](16-development-customization.md) | **Previous:** [Section 14: Troubleshooting](14-troubleshooting.md) | **Main Index:** [User Manual](../user_manual.md)