# KISS Command Testing Guide

This guide helps you test the new LoRa-specific KISS commands implemented in LoRaTNCX.

## Prerequisites

1. LoRaTNCX device flashed and connected via USB
2. Serial terminal software (PuTTY, Arduino Serial Monitor, etc.)
3. Connection at 115200 baud, 8N1

## Test Procedure

### 1. Verify Console Mode Works

Connect to device and verify basic functionality:
```
> help
> lora config
> lora freq 915.0
> lora power 10
```

### 2. Enter KISS Mode

```
> kiss
```

The device should enter KISS mode silently (no response). You're now ready to send raw KISS frames.

### 3. Test Basic KISS Commands

Send these hex sequences (ensure your terminal can send raw hex):

#### Test 1: Set TX Power to 12 dBm
Send: `C0 06 12 0C C0`
Expected: Response frame `C0 06 12 01 C0` (success = 0x01)

#### Test 2: Set Spreading Factor to SF9  
Send: `C0 06 14 09 C0`
Expected: Response frame `C0 06 14 01 C0` (success = 0x01)

#### Test 3: Set Bandwidth to 250 kHz
Send: `C0 06 13 08 C0`
Expected: Response frame `C0 06 13 01 C0` (success = 0x01)

#### Test 4: Get Configuration
Send: `C0 06 1A C0`
Expected: Multiple response frames showing current configuration

#### Test 5: Set Frequency to 433.5 MHz (433500000 Hz = 0x19D5B500)
Send: `C0 06 10 19 D5 B5 00 C0`  
Expected: Response frame `C0 06 10 01 C0` (success = 0x01)

### 4. Test Data Transmission

#### Send a data frame:
Send: `C0 00 48 65 6C 6C 6F C0` (sends "Hello")

If you have a second device in receive mode, it should receive the message.

### 5. Exit KISS Mode

Send: `C0 FF C0`
Expected: Device returns to command mode showing `cmd:` prompt

### 6. Verify Changes Were Applied

```
cmd: lora config
```

Check that the configuration changes from KISS commands are still active:
- TX Power should be 12 dBm
- Spreading Factor should be SF9  
- Bandwidth should be 250 kHz
- Frequency should be 433.5 MHz

## Troubleshooting

### No Response to KISS Commands
- Verify device is in KISS mode (no prompt after `kiss` command)
- Check hex frame format (must start and end with 0xC0)
- Ensure correct command bytes

### Invalid Parameter Responses  
- Response will be `C0 06 XX 00 C0` (failure = 0x00)
- Check parameter ranges:
  - TX Power: -17 to +22 dBm
  - Spreading Factor: 6-12
  - Coding Rate: 5-8 (4/5 to 4/8)
  - Bandwidth: 0-9 (see encoding table)

### Device Not Responding
- Power cycle the device
- Verify serial connection
- Try entering KISS mode again

## Command Reference

| Command | Hex | Parameters | Description |
|---------|-----|------------|-------------|
| SET_TXPOWER | 0x12 | 1 byte (dBm) | Set TX power |
| SET_BANDWIDTH | 0x13 | 1 byte (index) | Set bandwidth |
| SET_SF | 0x14 | 1 byte (6-12) | Set spreading factor |
| SET_CR | 0x15 | 1 byte (5-8) | Set coding rate |
| SET_PREAMBLE | 0x16 | 2 bytes (length) | Set preamble length |
| SET_SYNCWORD | 0x17 | 1 byte | Set sync word |
| SET_CRC | 0x18 | 1 byte (0/1) | Enable/disable CRC |
| SET_FREQ_LOW | 0x10 | 4 bytes (Hz) | Set frequency |
| SELECT_BAND | 0x19 | 1 byte (index) | Select frequency band |
| GET_CONFIG | 0x1A | None | Get configuration |
| SAVE_CONFIG | 0x1B | None | Save to flash |
| RESET_CONFIG | 0x1C | None | Reset to defaults |

## Frame Format

All LoRa commands use the SETHARDWARE format:
```
C0 06 <cmd> [params...] C0
```

Where:
- `C0` = FEND (Frame End marker)
- `06` = SETHARDWARE command
- `<cmd>` = LoRa sub-command
- `[params...]` = Command parameters (if any)
- `C0` = FEND (end marker)

## Success Indicators

- Configuration commands return: `C0 06 <cmd> 01 C0` (success) or `C0 06 <cmd> 00 C0` (failure)
- GET_CONFIG returns multiple frames with current settings
- Data frames are transmitted over LoRa without response