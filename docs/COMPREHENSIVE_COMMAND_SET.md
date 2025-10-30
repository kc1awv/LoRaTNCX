# LoRaTNCX Comprehensive Command Set

*Making your LoRa TNC as capable as a Kantronics KPC-3 or TAPR TNC-2*

## Overview

This reference mirrors the categories and command descriptions exposed by the firmware `HELP` handler (`command_help.cpp`). Each
section summarises the commands that appear in the on-device help output so the documentation and interactive help stay aligned.

## 1. Basic & Mode Control

| Command       | Description                                  | Notes/Examples |
| ------------- | -------------------------------------------- | -------------- |
| `HELP`        | Show this help overview                      | `HELP FREQ` for detailed help |
| `STATUS`      | Show system status                           |                |
| `VERSION`     | Show firmware version                        |                |
| `MODE`        | Show or set operating mode                   |                |
| `KISS`        | Enter KISS (binary) mode                     |                |
| `CMD`         | Return to command mode                       |                |
| `TERMINAL`    | Switch to terminal/chat mode                 |                |
| `TRANSPARENT` | Switch to transparent connected mode         |                |
| `SIMPLEX`     | Force simplex channel operation              |                |
| `CONNECT`     | Initiate a connection                        |                |
| `DISCONNECT`  | Terminate active connections                 |                |
| `QUIT`        | Exit to command mode without disconnect      |                |

## 2. Interface Settings

| Command   | Description                                     |
| --------- | ----------------------------------------------- |
| `PROMPT`  | Enable or disable the command prompt            |
| `ECHO`    | Control local command echo                      |
| `LINECR`  | Enable/disable carriage return in responses     |
| `LINELF`  | Enable/disable line feed in responses           |

## 3. Station Identification & Beaconing

| Command     | Description                                      |
| ----------- | ------------------------------------------------ |
| `MYCALL`    | Show or set station callsign                     |
| `MYSSID`    | Show or set station SSID                         |
| `BEACON`    | Configure scheduled beaconing                    |
| `BCON`      | Immediate beacon control                         |
| `BTEXT`     | Set beacon message text                          |
| `ID`        | Control station ID beacons                       |
| `CWID`      | Enable or disable CW ID                          |
| `LICENSE`   | Set regulatory license class                     |
| `LOCATION`  | Set GPS coordinates                              |
| `GRID`      | Set Maidenhead grid square                       |
| `APRS`      | Enable and configure APRS features               |

## 4. Radio Configuration

| Command        | Description                                   |
| -------------- | --------------------------------------------- |
| `FREQ`         | Set or show operating frequency               |
| `POWER`        | Set or show transmitter power                 |
| `SF`           | Set or show spreading factor                  |
| `BW`           | Set or show channel bandwidth                 |
| `CR`           | Set or show coding rate                        |
| `SYNC`         | Set or show sync word                         |
| `PREAMBLE`     | Configure LoRa preamble length                |
| `PACTL`        | Control the PA (power amplifier)              |
| `BAND`         | Select amateur radio band plan                |
| `REGION`       | Select regional compliance profile            |
| `COMPLIANCE`   | Show or set compliance options                |
| `EMERGENCY`    | Toggle emergency operating mode               |
| `SENSITIVITY`  | Adjust receiver sensitivity target            |

## 5. Protocol Timing & Link Control

| Command    | Description                         |
| ---------- | ----------------------------------- |
| `TXDELAY`  | Transmit key-up delay               |
| `TXTAIL`   | Transmit tail timing                |
| `PERSIST`  | CSMA persistence value              |
| `SLOTTIME` | CSMA slot time                      |
| `RESPTIME` | Response timeout                    |
| `MAXFRAME` | Maximum outstanding frames          |
| `FRACK`    | Frame acknowledge timeout           |
| `RETRY`    | Retry attempts                      |

## 6. Network & Routing

| Command   | Description                            |
| --------- | -------------------------------------- |
| `DIGI`    | Configure digipeater operation         |
| `ROUTE`   | Show or edit routing table             |
| `NODES`   | List heard stations                    |
| `UNPROTO` | Set unproto destination/path           |
| `UIDWAIT` | Configure UID wait timer               |
| `UIDFRAME`| Control UI frame transmission          |
| `MCON`    | Toggle monitor of connected frames     |
| `USERS`   | Set maximum simultaneous users         |
| `FLOW`    | Control flow-control behaviour         |

## 7. Monitoring & Telemetry

| Command       | Description                           |
| ------------- | ------------------------------------- |
| `STATS`       | Show packet statistics                |
| `RSSI`        | Show last received RSSI               |
| `SNR`         | Show last received SNR                |
| `LOG`         | Show or configure logging             |
| `MONITOR`     | Enable or disable packet monitor      |
| `MHEARD`      | Show heard-station history            |
| `TEMPERATURE` | Read radio temperature                |
| `VOLTAGE`     | Read supply voltage                   |
| `UPTIME`      | Show system uptime                    |
| `LORASTAT`    | Display detailed LoRa statistics      |

## 8. RF Tools & Analysis

| Command   | Description                     |
| --------- | ------------------------------- |
| `RANGE`   | Estimate link range             |
| `TOA`     | Calculate time-on-air           |
| `LINKTEST`| Run link testing utility        |

## 9. Testing & Diagnostics

| Command     | Description                        |
| ----------- | ---------------------------------- |
| `TEST`      | Run system tests                   |
| `CAL`       | Calibration utilities              |
| `CALIBRATE` | Detailed calibration routine       |
| `DIAG`      | System diagnostics                 |
| `PING`      | Send test packet                   |
| `SELFTEST`  | Run self-test suite                |
| `DEBUG`     | Set debug verbosity                |

## 10. Storage & System Management

| Command   | Description                              |
| --------- | ---------------------------------------- |
| `SAVE`    | Save settings to flash                   |
| `SAVED`   | Show settings stored in flash            |
| `LOAD`    | Load settings from flash                 |
| `RESET`   | Reset settings to defaults               |
| `FACTORY` | Perform factory reset                    |
| `DEFAULT` | Restore recommended defaults             |
| `PRESET`  | Apply stored configuration preset        |
| `MEMORY`  | Manage memory profiles                   |

## Command Mode Notes

- Commands are case-insensitive.
- Parameters are separated by spaces; quoted strings are supported where applicable.
- Use `HELP <command>` for additional detail on supported arguments for specific commands (for example, `HELP FREQ`).
- The TNC starts in command mode by default. `CMD`, `KISS`, `TERMINAL`, and `TRANSPARENT` switch between operating interfaces.
- Configuration can be saved to or loaded from flash with `SAVE`, `SAVED`, and `LOAD`; `DEFAULT`, `RESET`, and `FACTORY` reset
  to defined baseline configurations.

