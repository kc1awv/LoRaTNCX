# LoRa Ping/Pong Test

A simple ping/pong communication test for Heltec WiFi LoRa 32 V4 devices using the RadioLib library.

## Overview

This test allows you to verify LoRa communication between two Heltec V4 devices by having one device send "ping" messages and the other respond with "pong" messages.

## Hardware Requirements

- 2x Heltec WiFi LoRa 32 V4 boards
- USB cables for programming and power
- Antennas (915MHz or 868MHz depending on your region)

## Software Requirements

- PlatformIO
- Python 3.x (for the monitor application)
- pyserial library: `pip install pyserial`

## Configuration

### Device Role Selection

You can configure device roles in three ways:

1. **Compile-time configuration** (recommended for permanent setup):
   - Edit `src/main.cpp`
   - Uncomment `#define PING_DEVICE` for the sender
   - Uncomment `#define PONG_DEVICE` for the responder

2. **Auto-detection at startup**:
   - Leave both defines commented out
   - Device will default to PING mode
   - Hold the USER button (GPIO 0) during startup to set as PONG device

3. **Manual selection**:
   - Use the serial monitor to see device role assignment

### LoRa Parameters

Adjust these parameters in `src/main.cpp` as needed:

```cpp
#define FREQUENCY       915.0   // MHz (915 for US, 868 for EU)
#define BANDWIDTH       125.0   // kHz
#define SPREADING_FACTOR   7    // 7-12 (higher = longer range, slower)
#define CODING_RATE        5    // 5-8 (higher = more error correction)
#define OUTPUT_POWER      14    // dBm (max 22 for SX1262)
```

## Building and Uploading

1. **Build the firmware**:
   ```bash
   platformio run -e heltec_wifi_lora_32_V4
   ```

2. **Upload to first device** (PING):
   - Make sure `#define PING_DEVICE` is uncommented
   - Connect first device via USB
   - Upload: `platformio run -e heltec_wifi_lora_32_V4 -t upload`

3. **Upload to second device** (PONG):
   - Change to `#define PONG_DEVICE` in main.cpp
   - Connect second device via USB
   - Upload: `platformio run -e heltec_wifi_lora_32_V4 -t upload`

## Testing

### Using Serial Monitor

1. **Connect to PING device**:
   ```bash
   platformio device monitor -e heltec_wifi_lora_32_V4
   ```

2. **Connect to PONG device** (in another terminal):
   ```bash
   platformio device monitor -e heltec_wifi_lora_32_V4 --port /dev/ttyUSB1
   ```

### Using Python Monitor (Recommended)

The included Python monitor provides enhanced statistics and logging:

1. **Start monitoring PING device**:
   ```bash
   python lora_pingpong_monitor.py
   ```

2. **Start monitoring PONG device** (in another terminal):
   ```bash
   python lora_pingpong_monitor.py /dev/ttyUSB1
   ```

## Expected Output

### PING Device Output
```
=== LoRa Ping/Pong Test ===
Hardware: Heltec WiFi LoRa 32 V4
Device Role: PING (sender)
✓ Radio initialized successfully!
Sending pings every 2 seconds...
----------------------------------------
Sending ping #1... sent!
Received: PONG:1:12345
RSSI: -45.5 dBm
SNR: 8.2 dB
Success rate: 100.0%
```

### PONG Device Output
```
=== LoRa Ping/Pong Test ===
Hardware: Heltec WiFi LoRa 32 V4
Device Role: PONG (responder)
✓ Radio initialized successfully!
Listening for pings...
----------------------------------------
Received: PING:1:12340
RSSI: -44.2 dBm
SNR: 9.1 dB
Sending pong for ping #1... sent!
```

## LED Indicators

- **Heartbeat**: Slow blink every second (device is alive)
- **Ping transmission**: Quick single flash
- **Pong transmission**: Quick double flash
- **Radio error**: Rapid blinking (10 times) during startup

## Troubleshooting

### No Communication
1. Check antenna connections
2. Verify frequency settings match your region
3. Ensure devices are within range (start close, ~1 meter apart)
4. Check serial output for radio initialization errors

### Poor Performance
1. Increase spreading factor for better sensitivity
2. Reduce bandwidth for better sensitivity
3. Increase output power (up to legal limits)
4. Check for interference sources

### Build Errors
1. Ensure RadioLib library is installed
2. Check that `HardwareConfig.h` exists in include directory
3. Verify PlatformIO configuration

## Range Testing

For range testing:
1. Start with devices close together
2. Gradually increase distance
3. Monitor RSSI and SNR values
4. Note where communication becomes unreliable

Typical ranges:
- **Urban environment**: 1-5 km
- **Suburban environment**: 5-15 km  
- **Rural/open field**: 15-40+ km

## Advanced Configuration

### Custom Message Format

Messages follow this format:
- PING: `"PING:<id>:<timestamp>"`
- PONG: `"PONG:<id>:<timestamp>"`

### Timing Parameters

```cpp
#define PING_INTERVAL   2000    // ms between pings
#define RX_TIMEOUT      5000    // ms to wait for response
```

## Next Steps

Once basic communication is working:
1. Test different LoRa parameters
2. Measure range vs. settings
3. Add GPS coordinates for range mapping
4. Implement packet error rate testing
5. Add OLED display support