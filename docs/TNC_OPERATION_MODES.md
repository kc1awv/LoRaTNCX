# TNC Operation Modes

This document explains the purpose and behavior of the four major operating modes provided by the LoRaTNCX firmware for the Heltec WiFi LoRa 32 V4: **Command**, **Converse**, **Transparent**, and **KISS**. While these modes share common radio and serial subsystems, they target different interaction styles that amateur radio operators rely on when integrating a Terminal Node Controller (TNC) into their stations.

## Command Mode

Command mode is the operator-facing control environment where the TNC interprets ASCII commands entered over the serial console. The LoRaTNCX firmware exposes configuration verbs here to adjust LoRa channel parameters, station identification, timing values, and other run-time behaviors described in the comprehensive command set documentation. In this mode the TNC does **not** automatically forward user text to the radio; instead, each line is parsed as an instruction that updates internal settings, queries status, or toggles features before returning a textual acknowledgement. This mirrors the classic command interface found on legacy packet TNCs, making it familiar to operators who learned to configure devices such as the Kantronics KPC or the MFJ-1270. Entering command mode is typically the first step after connecting the Heltec board so that the station can be configured for the target frequency plan, transmit power profile, and protocol options prior to live operation.

Key characteristics:

- Local serial console interaction with echo and prompt control.
- Commands follow the syntax outlined in `docs/COMPREHENSIVE_COMMAND_SET.md` and update persistent configuration stored by the firmware.
- Radio transmission is inhibited except when explicitly requested (for example, sending a beacon command).

## Converse Mode

Converse mode enables manual keyboard-to-keyboard packet exchanges while retaining the ability to return to command control without power-cycling. When the operator switches from command to converse, the TNC begins treating ASCII text entered on the serial port as payload data. Each carriage-return-terminated line is framed and queued for transmission over LoRa using the currently active modulation and timing parameters. Received packets are immediately displayed to the console with minimal decoration so that live conversations resemble a terminal chat session.

Key characteristics:

- Text typed after the prompt is transmitted as AX.25/LoRa frames; received frames appear as plain text lines.
- Control escapes (typically `CTRL-C` or the command character defined in configuration) allow the operator to return to command mode without resetting the device.
- Useful for testing link quality and verifying configuration changes interactively before automating traffic.

## Transparent Mode

Transparent mode is designed for host systems that expect a TNC to behave like a direct serial-to-radio bridge. In this mode the firmware stops line buffering and command parsing entirely; every byte received on the serial interface is forwarded to the radio link as soon as the MAC layer allows, and incoming radio frames are emitted immediately on the serial side without translation. This makes the TNC appear "invisible" to higher-level software, allowing legacy packet applications that generate their own AX.25 or application framing to operate without modification.

Key characteristics:

- No command interpreter; binary payloads and control characters pass through untouched.
- Timing parameters such as TX delay, persistence, and slot time configured earlier still govern channel access, but the serial stream is otherwise unmodified.
- Ideal for attaching external controllers, data loggers, or network stack implementations that expect deterministic byte-for-byte delivery.

## KISS Mode

KISS (Keep It Simple, Stupid) mode wraps all host communication in the standardized KISS framing protocol so that software such as Dire Wolf, LinBPQ, or UI-View can drive the TNC programmatically. LoRaTNCX implements the full KISS command set documented in `docs/KISS_PROTOCOL_IMPLEMENTATION.md`, allowing the host to set parameters like TX delay (`0x01`), persistence (`0x02`), slot time (`0x03`), and to request raw channel statistics. In this mode the serial link operates on binary frames prefixed with command identifiers and uses the KISS escape mechanism to differentiate control characters from payload data. Outbound data from the host is encapsulated as KISS data frames, while inbound LoRa packets are returned using the same framing so that host applications can decode them reliably.

Key characteristics:

- Enables interoperability with existing packet radio stacks that speak the KISS protocol.
- Supports real-time command and telemetry exchange without leaving KISS mode; hosts can reconfigure timing or modulation parameters on the fly.
- Ensures consistent escaping of special characters (`0xC0` and `0xDB`), preventing accidental mode switches or corrupted frames during binary transfers.

## Mode Selection Workflow

Operators typically initialize the TNC in command mode to verify hardware status and set configuration values, then select either converse or transparent mode depending on whether they are typing manually or attaching an intelligent controller. KISS mode is preferred when integrating with established packet radio software suites. Because all modes share the same underlying radio configuration managed by the firmware, changes made in command mode (or via KISS commands) persist across mode switches, allowing the Heltec-based TNC to adapt quickly to different operating scenarios without reflashing or rebooting.