# KISS SetHardware Commands for LoRa Radio Control

## Overview

Implemented comprehensive KISS SetHardware commands (0x06) to allow host applications to configure LoRa radio parameters through the standard KISS protocol. This enables remote control of all critical LoRa settings via KISS frames.

## Implementation Details

### KISS Command Structure
```
C0 06 <PARAM_ID> [<DATA>...] C0
```
- `C0`: KISS frame delimiter
- `06`: KISS_CMD_SETHARDWARE command
- `<PARAM_ID>`: Hardware parameter identifier (see below)
- `[<DATA>...]`: Parameter-specific data bytes
- `C0`: Frame end delimiter

### Supported LoRa Parameters

#### 1. Frequency Control (0x00)
- **Parameter ID**: `LORA_HW_FREQUENCY` (0x00)
- **Data Format**: 4 bytes, little-endian, frequency in Hz
- **Example**: Set 915.0 MHz = 915,000,000 Hz
  ```
  C0 06 00 40 77 89 36 C0
  ```

#### 2. TX Power Control (0x01)
- **Parameter ID**: `LORA_HW_TX_POWER` (0x01)  
- **Data Format**: 1 byte, signed, power in dBm
- **Range**: Depends on hardware, typically -17 to +22 dBm
- **Example**: Set 10 dBm
  ```
  C0 06 01 0A C0
  ```

#### 3. Bandwidth Control (0x02)
- **Parameter ID**: `LORA_HW_BANDWIDTH` (0x02)
- **Data Format**: 1 byte, bandwidth index
- **Supported Values**:
  - `0x00`: 7.8 kHz
  - `0x01`: 10.4 kHz
  - `0x02`: 15.6 kHz
  - `0x03`: 20.8 kHz
  - `0x04`: 31.25 kHz
  - `0x05`: 41.7 kHz
  - `0x06`: 62.5 kHz
  - `0x07`: 125 kHz (default)
  - `0x08`: 250 kHz
  - `0x09`: 500 kHz
- **Example**: Set 125 kHz bandwidth
  ```
  C0 06 02 07 C0
  ```

#### 4. Spreading Factor Control (0x03)
- **Parameter ID**: `LORA_HW_SPREADING_FACTOR` (0x03)
- **Data Format**: 1 byte, spreading factor value
- **Range**: 6-12 (SF6 to SF12)
- **Example**: Set SF7
  ```
  C0 06 03 07 C0
  ```

#### 5. Coding Rate Control (0x04)
- **Parameter ID**: `LORA_HW_CODING_RATE` (0x04)
- **Data Format**: 1 byte, coding rate denominator
- **Range**: 5-8 (representing 4/5, 4/6, 4/7, 4/8)
- **Example**: Set 4/5 coding rate
  ```
  C0 06 04 05 C0
  ```

## Code Architecture

### KISSProtocol Class Extensions

#### New Header Definitions (`KISSProtocol.h`)
```cpp
// LoRa SetHardware Parameter IDs
#define LORA_HW_FREQUENCY         0x00
#define LORA_HW_TX_POWER          0x01  
#define LORA_HW_BANDWIDTH         0x02
#define LORA_HW_SPREADING_FACTOR  0x03
#define LORA_HW_CODING_RATE       0x04

// Bandwidth index definitions
#define LORA_BW_125_KHZ     0x07    // Most common
#define LORA_BW_250_KHZ     0x08
#define LORA_BW_500_KHZ     0x09
// ... (full range 7.8 kHz to 500 kHz)
```

#### New Methods
```cpp
bool handleSetHardware(uint8_t parameter, const uint8_t* data, size_t dataLen);
void setLoRaRadio(class LoRaRadio* radio);
```

#### Member Variables
```cpp
class LoRaRadio* loraRadio;  // Reference to LoRa radio for control
```

### Implementation (`KISSProtocol.cpp`)

#### Enhanced Command Processing
The existing `KISS_CMD_SETHARDWARE` case now calls the comprehensive handler:
```cpp
case KISS_CMD_SETHARDWARE:
    Serial.printf("KISS: Hardware command parameter 0x%02X\n", parameter);
    handleSetHardware(parameter, nullptr, 0);
    break;
```

#### Complete Parameter Handler
- **Frequency**: Converts Hz to MHz and validates range
- **TX Power**: Direct dBm value with range checking
- **Bandwidth**: Index-to-frequency mapping with validation
- **Spreading Factor**: Range validation (SF6-SF12)
- **Coding Rate**: Range validation (4/5 to 4/8)

### Integration with TNCManager

#### Automatic Connection
```cpp
// In TNCManager::begin()
if (loraRadio) {
    kissProtocol.setLoRaRadio(loraRadio);
    Serial.println("LoRa radio connected to KISS protocol");
}
```

## Usage Examples

### Host Application Integration
```python
import serial

# Connect to TNC
tnc = serial.Serial('/dev/ttyACM0', 9600)

# Enter KISS mode
tnc.write(b'KISS ON\r\n')
tnc.write(b'RESTART\r\n')

# Set frequency to 915.0 MHz
freq_hz = 915000000
freq_bytes = freq_hz.to_bytes(4, 'little')
kiss_frame = b'\xC0\x06\x00' + freq_bytes + b'\xC0'
tnc.write(kiss_frame)

# Set TX power to 10 dBm  
power_frame = b'\xC0\x06\x01\x0A\xC0'
tnc.write(power_frame)

# Set bandwidth to 125 kHz
bw_frame = b'\xC0\x06\x02\x07\xC0'
tnc.write(bw_frame)
```

### Response Handling
The TNC provides verbose feedback via serial console:
```
KISS: Setting frequency to 915.000 MHz
KISS: Setting TX power to 10 dBm
KISS: Setting bandwidth to 125.0 kHz
```

## Error Handling

### Validation Checks
- **Data Length**: Ensures sufficient bytes for each parameter
- **Range Validation**: Checks parameter values are within valid ranges
- **Hardware Connection**: Verifies LoRa radio is available
- **Parameter Recognition**: Reports unknown parameter IDs

### Error Messages
```
KISS: No LoRa radio configured for hardware commands
KISS: Invalid frequency data length
KISS: Invalid spreading factor 13 (must be 6-12)  
KISS: Unknown hardware parameter 0x99
```

## Benefits

### 1. **Standard Compliance**
- Uses official KISS SetHardware command (0x06)
- Compatible with existing KISS applications
- Follows TAPR TNC protocol conventions

### 2. **Complete Control**
- All critical LoRa parameters configurable
- Real-time parameter changes without restart
- Immediate feedback and validation

### 3. **Host Integration**
- Enables sophisticated frequency management
- Supports adaptive modulation schemes
- Allows automated optimization based on conditions

### 4. **Professional Operation**
- Proper error handling and validation
- Verbose diagnostic output
- Maintains parameter persistence during session

## Future Extensions

### Planned Enhancements
- **Sync Word Control** (0x05): Custom sync word configuration
- **Preamble Length** (0x06): Adjustable preamble timing
- **Get Parameter Commands**: Query current settings
- **Parameter Persistence**: Save settings to EEPROM

### Application Integration
- **APRS Applications**: Dynamic frequency switching
- **Mesh Networks**: Adaptive modulation based on link quality
- **Repeater Systems**: Remote configuration capability
- **Test Equipment**: Automated parameter sweeping

This implementation provides a solid foundation for professional LoRa TNC operation with full remote configurability through the standard KISS protocol.