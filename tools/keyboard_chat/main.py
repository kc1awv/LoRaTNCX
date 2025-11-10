"""Keyboard-to-keyboard chat client for KISS TNC devices.

This script provides a simple interactive CLI to connect to a KISS-based TNC over serial,
set up callsign and radio configuration, and run a chat session with commands.

Assumptions:
- Device accepts simple text configuration commands sent as KISS payloads (e.g. "RADIOCFG SF=7 BW=125").
- Device will send a small identification string including 'V4' if it's a V4 device.
If your device uses different control message formats, adapt `send_device_command`.
"""

import threading
import time
import json
from pathlib import Path
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout
import serial
from serial import SerialException
import struct

from kiss import (
    encode_data,
    encode_command,
    decode_kiss,
    CMD_DATA,
    CMD_GETHARDWARE,
    CMD_SETHARDWARE,
    HW_QUERY_CONFIG,
    HW_QUERY_BATTERY,
    HW_QUERY_BOARD,
    HW_QUERY_ALL,
    HW_SET_FREQUENCY,
    HW_SET_BANDWIDTH,
    HW_SET_SPREADING,
    HW_SET_CODINGRATE,
    HW_SET_POWER,
    HW_SAVE_CONFIG,
    HW_SET_SYNCWORD,
    HW_RESET_CONFIG,
)
from config_manager import load_config, save_config


DEFAULT_BAUD = 115200


class KeyboardChat:
    def __init__(self):
        self.config = load_config()
        self.callsign = self.config.get("callsign", "")
        self.port = None
        self.serial = None
        self.reader_thread = None
        self.running = False
        self.accept_pings = True
        # logical link state (connection to a remote user)
        self.link_connected = False
        self.link_peer = None
        # pending incoming connection requests (callsign -> timestamp)
        self.pending_incoming = {}
        # pending outgoing connection events: callsign -> threading.Event
        self.pending_outgoing_events = {}
        self._lock = threading.Lock()
        # auto-accept incoming connection requests if set in config
        self.auto_accept = self.config.get("auto_accept_connections", False)
        # pending incoming request timeout (seconds)
        self.pending_timeout = float(self.config.get("connection_request_timeout", 30))
        # how often to check pending requests (seconds)
        self._pending_check_interval = 1.0
        self._last_pending_check = time.time()

    def ask_setup(self):
        session = PromptSession()
        print("--- Keyboard Chat Setup ---")
        if not self.callsign:
            self.callsign = session.prompt("Enter your callsign: ").strip()
        default_port = self.config.get("serial_port", "COM3" if Path("C:").exists() else "/dev/ttyUSB0")
        serial_port = session.prompt(f"Serial port [{default_port}]: ") or default_port
        baud = session.prompt(f"Baud [{DEFAULT_BAUD}]: ") or str(DEFAULT_BAUD)
        self.config["serial_port"] = serial_port
        self.config["baud"] = int(baud)
        # Placeholder: ask radio config display/change
        print("\nCurrent radio configuration will be queried after connecting. You can change parameters later.")
        # GNSS question - we will ask and store preference if user has a V4 device later
        self.config.setdefault("gnss_enabled", None)
        # Beacons
        beacon_default = self.config.get("beacons", True)
        b = session.prompt(f"Enable beacons? [{'Y' if beacon_default else 'N'}]: ") or ("Y" if beacon_default else "N")
        self.config["beacons"] = True if b.strip().upper().startswith("Y") else False
        # Save config choice
        save_choice = session.prompt("Save configuration for future use? [Y]: ") or "Y"
        if save_choice.strip().upper().startswith("Y"):
            self.config["callsign"] = self.callsign
            saved = save_config(self.config)
            print("Configuration saved." if saved else "Failed to save configuration.")
        # store serial settings locally
        self.config["serial_port"] = serial_port
        self.config["baud"] = int(baud)
        # automatically open serial connection to the TNC after setup
        print("Opening serial connection to device...")
        self.connect()

    def connect(self):
        if self.serial and self.serial.is_open:
            print("Already connected")
            return
        port = self.config.get("serial_port")
        baud = self.config.get("baud", DEFAULT_BAUD)
        try:
            self.serial = serial.Serial(port, baud, timeout=0.2)
            print(f"Connected to {port} @ {baud}")
            self.running = True
            self.reader_thread = threading.Thread(target=self.reader_loop, daemon=True)
            self.reader_thread.start()
            # Query device for radio config
            time.sleep(0.2)
            self.send_device_command("GET_RADIOCFG")
            # ask for device identification
            self.send_device_command("ID")
        except SerialException as e:
            print(f"Serial error: {e}")

    def disconnect(self):
        self.running = False
        if self.serial and self.serial.is_open:
            try:
                self.serial.close()
            except Exception:
                pass
        print("Disconnected")

    def reader_loop(self):
        buffer = bytearray()
        while self.running:
            try:
                data = self.serial.read(1024)
                if data:
                    buffer.extend(data)
                    # decode frames using rolling buffer support; decode_kiss returns (frames, leftover)
                    frames, leftover = decode_kiss(bytes(buffer))
                    # keep leftover bytes in the buffer for next read
                    buffer = bytearray(leftover)
                    for payload in frames:
                        self.handle_payload(payload)
                    # periodically expire old pending incoming connection requests
                    now = time.time()
                    if now - self._last_pending_check >= self._pending_check_interval:
                        self._last_pending_check = now
                        try:
                            self._expire_pending()
                        except Exception:
                            pass
                else:
                    time.sleep(0.05)
            except Exception as e:
                print(f"Reader error: {e}")
                time.sleep(0.2)

    def handle_payload(self, payload: bytes):
        # payload is the unescaped frame payload; first byte is the KISS command byte
        if not payload:
            return
        cmd = payload[0]
        if cmd == CMD_DATA:
            # data frame: human-readable messages start at payload[1:]
            try:
                text = payload[1:].decode("utf-8", errors="replace")
            except Exception:
                text = str(payload[1:])

            # chat/text protocol handling
            # addressed messages (only used when peers are logically connected)
            if text.startswith("MSGTO:"):
                # MSGTO:from:to:message
                parts = text.split(":", 3)
                if len(parts) >= 4:
                    frm = parts[1]
                    to = parts[2]
                    body = parts[3]
                    if to == self.callsign:
                        print(f"[MSG] {frm}: {body}")
                    else:
                        # not for us
                        print(f"[MSGTO other] {text}")
                else:
                    print(f"[MSGTO] malformed: {text}")
            elif text.startswith("MSG:"):
                # legacy/broadcast messages
                print(f"[MSG] {text}")
            elif text.startswith("RADIOCFG:"):
                print(f"[RADIOCFG] {text[len('RADIOCFG:'):]}")
            elif text.startswith("ID:"):
                idstr = text[len('ID:'):].strip()
                print(f"[DEVICE ID] {idstr}")
                if "V4" in idstr and self.config.get("gnss_enabled") is None:
                    session = PromptSession()
                    ans = session.prompt("This appears to be a V4 device — enable GNSS? [Y/N]: ")
                    enable = ans.strip().upper().startswith("Y")
                    self.config["gnss_enabled"] = enable
                    cmdtxt = f"GNSS_ENABLE={1 if enable else 0}"
                    # leave GNSS toggle as a text command for now
                    self.send_kiss(cmdtxt.encode("utf-8"))
            elif text.startswith("PING:"):
                parts = text.split(":")
                if len(parts) >= 3:
                    frm = parts[1]
                    to = parts[2]
                    if to == self.callsign:
                        print(f"[PING] ping from {frm}")
                        if self.accept_pings:
                            resp = f"PONG:{self.callsign}:{frm}"
                            self.send_kiss(resp.encode("utf-8"))
                    else:
                        print(f"[PING other] {text}")
                else:
                    print(f"[PING] {text}")
            elif text.startswith("PONG:"):
                print(f"[PONG] {text}")
            elif text.startswith("CONNREQ:"):
                # incoming connection request: CONNREQ:from:to
                parts = text.split(":")
                if len(parts) >= 3:
                    frm = parts[1]
                    to = parts[2]
                    if to == self.callsign:
                        print(f"[CONNREQ] connection request from {frm}")
                        # auto-accept if configured
                        if self.auto_accept:
                            print(f"Auto-accepting connection from {frm}")
                            self.send_connack(frm)
                            with self._lock:
                                self.link_connected = True
                                self.link_peer = frm
                        else:
                            with self._lock:
                                # record timestamp for pending request
                                self.pending_incoming[frm] = time.time()
                            print(f"Use /accept {frm} to accept or /reject {frm} to reject")
                    else:
                        print(f"[CONNREQ other] {text}")
                else:
                    print(f"[CONNREQ] malformed: {text}")
            elif text.startswith("CONNACK:"):
                # CONNACK:from:to
                parts = text.split(":")
                if len(parts) >= 3:
                    frm = parts[1]
                    to = parts[2]
                    # if this is acknowledging our outgoing request
                    if to == self.callsign:
                        print(f"[CONNACK] connection accepted by {frm}")
                        with self._lock:
                            ev = self.pending_outgoing_events.get(frm)
                            if ev:
                                ev.set()
                            self.link_connected = True
                            self.link_peer = frm
                    else:
                        print(f"[CONNACK other] {text}")
                else:
                    print(f"[CONNACK] malformed: {text}")
            elif text.startswith("DISCONN:"):
                parts = text.split(":")
                if len(parts) >= 3:
                    frm = parts[1]
                    to = parts[2]
                    if to == self.callsign and self.link_peer == frm:
                        print(f"[DISCONN] {frm} disconnected")
                        with self._lock:
                            self.link_connected = False
                            self.link_peer = None
                    else:
                        print(f"[DISCONN other] {text}")
            else:
                print(f"[RX] {text}")

        elif cmd == CMD_GETHARDWARE:
            # Hardware query responses: payload[1] is subcommand
            if len(payload) < 2:
                return
            sub = payload[1]
            if sub == HW_QUERY_CONFIG and len(payload) >= 15:
                # Format: CMD_GETHARDWARE, HW_QUERY_CONFIG, freq(4), bw(4), sf(1), cr(1), pwr(1), sync(2)
                freq = struct.unpack('<f', payload[2:6])[0]
                bw = struct.unpack('<f', payload[6:10])[0]
                sf = payload[10]
                cr = payload[11]
                pwr = struct.unpack('<b', bytes([payload[12]]))[0]
                sync = struct.unpack('<H', payload[13:15])[0]
                print(f"[HW CONFIG] Frequency={freq} MHz, BW={bw} kHz, SF={sf}, CR=4/{cr}, PWR={pwr} dBm, SYNC=0x{sync:04X}")
            elif sub == HW_QUERY_BATTERY and len(payload) >= 6:
                voltage = struct.unpack('<f', payload[2:6])[0]
                print(f"[BATTERY] {voltage:.2f} V")
            elif sub == HW_QUERY_BOARD and len(payload) >= 3:
                btype = payload[2]
                name = payload[3:].decode('ascii', errors='ignore')
                print(f"[BOARD] Type=V{btype}, Name={name}")
            else:
                print(f"[HW] Unknown hardware response: sub={sub} payload={payload[2:]}")

        else:
            # Unknown command frame - print raw
            print(f"[RAW CMD {cmd:#02x}] {payload}")

    def send_kiss(self, payload: bytes):
        if not (self.serial and self.serial.is_open):
            print("Not connected")
            return
        frame = encode_data(payload)
        self.send_raw(frame)

    def send_raw(self, frame: bytes):
        """Write a raw KISS frame to the serial port and log the hex for debugging."""
        if not (self.serial and self.serial.is_open):
            print("Not connected")
            return
        try:
            written = self.serial.write(frame)
            # show a compact hex preview
            print(f"TX ({written} bytes): {frame.hex()}")
            return written
        except Exception as e:
            print(f"Send error: {e}")
            return 0

    def send_device_command(self, cmd: str):
        """Map human-friendly device commands to binary KISS command frames when possible.

        Supported inputs:
        - "GET_RADIOCFG" -> GETHARDWARE / HW_QUERY_CONFIG
        - "GET_BATTERY" -> GETHARDWARE / HW_QUERY_BATTERY
        - "GET_BOARD" -> GETHARDWARE / HW_QUERY_BOARD
        - "GET_ALL" -> GETHARDWARE / HW_QUERY_ALL
        - "RADIOCFG: KEY=VAL ..." -> maps keys like FREQ,BW,SF,CR,PWR,SYNC to SETHARDWARE subcommands
        - "ID" -> mapped to GET_BOARD
        Otherwise, falls back to sending a text DATA frame.
        """
        if not (self.serial and self.serial.is_open):
            print("Not connected")
            return

        uc = cmd.strip().upper()
        try:
            if uc == "GET_RADIOCFG":
                frame = encode_command(CMD_GETHARDWARE, HW_QUERY_CONFIG)
                self.send_raw(frame)
                return
            if uc == "GET_BATTERY":
                frame = encode_command(CMD_GETHARDWARE, HW_QUERY_BATTERY)
                self.send_raw(frame)
                return
            if uc == "GET_BOARD" or uc == "ID":
                frame = encode_command(CMD_GETHARDWARE, HW_QUERY_BOARD)
                self.send_raw(frame)
                return
            if uc == "GET_ALL":
                frame = encode_command(CMD_GETHARDWARE, HW_QUERY_ALL)
                self.send_raw(frame)
                return

            # RADIOCFG: parsing
            if uc.startswith("RADIOCFG:"):
                body = cmd.split(":", 1)[1].strip()
                # parse tokens like SF=7 BW=125 FREQ=915.0 CR=7 PWR=20 SYNC=0x1424
                tokens = body.split()
                for tok in tokens:
                    if "=" in tok:
                        k, v = tok.split("=", 1)
                        k = k.strip().upper()
                        v = v.strip()
                        if k in ("FREQ", "FREQUENCY"):
                            data = struct.pack('<f', float(v))
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_FREQUENCY, data))
                        elif k in ("BW", "BANDWIDTH"):
                            data = struct.pack('<f', float(v))
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_BANDWIDTH, data))
                        elif k in ("SF", "SPREADING"):
                            data = bytes([int(v)])
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_SPREADING, data))
                        elif k in ("CR", "CODINGRATE"):
                            data = bytes([int(v)])
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_CODINGRATE, data))
                        elif k in ("PWR", "POWER"):
                            data = struct.pack('<b', int(v))
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_POWER, data))
                        elif k in ("SYNC", "SYNCWORD"):
                            if v.startswith("0X") or v.startswith("0x"):
                                val = int(v, 16)
                            else:
                                val = int(v)
                            data = struct.pack('<H', int(val))
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SET_SYNCWORD, data))
                        elif k in ("SAVE",) and v in ("1", "Y", "YES"):
                            self.send_raw(encode_command(CMD_SETHARDWARE, HW_SAVE_CONFIG))
                        else:
                            # unknown token -> send as text fallback
                            self.send_kiss(cmd.encode('utf-8'))
                return

            # Save or reset commands
            if uc == "SAVE" or uc == "SAVE_CONFIG":
                self.send_raw(encode_command(CMD_SETHARDWARE, HW_SAVE_CONFIG))
                return
            if uc == "RESET" or uc == "RESET_CONFIG":
                self.send_raw(encode_command(CMD_SETHARDWARE, HW_RESET_CONFIG))
                return

        except Exception as e:
            print(f"Error sending device command: {e}")

        # Fallback - send plain text inside a DATA frame
        self.send_kiss(cmd.encode("utf-8"))

    # --- logical connection helpers ---
    def send_connreq(self, target: str, timeout: float = 8.0) -> bool:
        """Send a connection request to target and wait for CONNACK (returns True if connected)."""
        if not (self.serial and self.serial.is_open):
            print("Not connected to device (serial)")
            return False
        if target == self.callsign:
            print("Cannot connect to yourself")
            return False
        # prepare event to wait on
        ev = threading.Event()
        with self._lock:
            self.pending_outgoing_events[target] = ev
        req = f"CONNREQ:{self.callsign}:{target}"
        self.send_kiss(req.encode('utf-8'))
        print(f"Connection request sent to {target}, waiting for reply...")
        ok = ev.wait(timeout)
        with self._lock:
            # cleanup event
            self.pending_outgoing_events.pop(target, None)
        if ok:
            print(f"Connected to {target}")
            with self._lock:
                self.link_connected = True
                self.link_peer = target
            return True
        else:
            print(f"No response from {target}")
            return False

    def send_connack(self, target: str):
        if not (self.serial and self.serial.is_open):
            return
        ack = f"CONNACK:{self.callsign}:{target}"
        self.send_kiss(ack.encode('utf-8'))

    def send_disconnect(self, target: str):
        if not (self.serial and self.serial.is_open):
            return
        d = f"DISCONN:{self.callsign}:{target}"
        self.send_kiss(d.encode('utf-8'))

    def _expire_pending(self):
        """Expire pending incoming connection requests older than pending_timeout.

        For each expired request we send a DISCONN to notify the peer and remove it
        from the pending list.
        """
        now = time.time()
        expired = []
        with self._lock:
            for caller, ts in list(self.pending_incoming.items()):
                if now - ts >= self.pending_timeout:
                    expired.append(caller)
                    self.pending_incoming.pop(caller, None)
        for caller in expired:
            print(f"[CONNREQ] auto-rejecting connection request from {caller} (timeout {self.pending_timeout}s)")
            try:
                self.send_disconnect(caller)
            except Exception:
                pass

    def interactive_loop(self):
        session = PromptSession()
        print("Type /help for commands. Enter chat messages to transmit to channel.")
        with patch_stdout():
            while True:
                try:
                    text = session.prompt("> ")
                except (KeyboardInterrupt, EOFError):
                    print("Exiting")
                    self.disconnect()
                    break
                if not text:
                    continue
                if text.startswith("/"):
                    self.handle_command(text[1:].strip())
                else:
                    # send as chat message only when logically connected to a peer
                    if not self.link_connected or not self.link_peer:
                        print("Not connected to a peer. Use /connect <callsign> to request a connection before sending chat messages.")
                    else:
                        # send addressed message to the connected peer
                        msg = f"MSGTO:{self.callsign}:{self.link_peer}:{text}"
                        self.send_kiss(msg.encode("utf-8"))

    def handle_command(self, cmdline: str):
        parts = cmdline.split()
        cmd = parts[0].lower()
        args = parts[1:]
        if cmd == "help":
            print("Commands:\n  /connect <callsign>  /disconnect  /accept <callsign>  /reject <callsign>  /beacon  /ping <callsign>  /acceptping on|off  /cfg KEY=VAL  /selftest  /exit")
            return
        if cmd == "connect":
            # logical connect to a remote user (sends CONNREQ and waits for CONNACK)
            if not args:
                print("Usage: /connect <callsign>")
                return
            tgt = args[0]
            if self.link_connected:
                print(f"Already connected to {self.link_peer}. Disconnect first.")
                return
            ok = self.send_connreq(tgt)
            if not ok:
                print("Connection failed or timed out")
            return
        if cmd == "disconnect":
            # logical disconnect from remote peer
            if self.link_connected and self.link_peer:
                self.send_disconnect(self.link_peer)
                with self._lock:
                    print(f"Disconnecting from {self.link_peer}")
                    self.link_connected = False
                    self.link_peer = None
            else:
                print("Not connected to any peer")
            return
        if cmd == "beacon":
            # send beacon once
            beacon = f"BEACON:{self.callsign}"
            self.send_kiss(beacon.encode("utf-8"))
            print("Beacon sent")
            return
        if cmd == "ping":
            if len(args) < 1:
                print("Usage: /ping TARGET_CALLSIGN")
                return
            tgt = args[0]
            ping = f"PING:{self.callsign}:{tgt}"
            self.send_kiss(ping.encode("utf-8"))
            print(f"Ping to {tgt} sent")
            return
        if cmd == "acceptping":
            if not args:
                print(f"acceptping is {'on' if self.accept_pings else 'off'}")
            else:
                self.accept_pings = args[0].lower() in ("on", "1", "yes", "y")
                print(f"accept_pings set to {self.accept_pings}")
            return
        if cmd == "cfg":
            if not args:
                print("Usage: /cfg KEY=VAL (example: /cfg SF=7)")
                return
            cfg = " ".join(args)
            # send radio configuration command
            cmdstr = f"RADIOCFG:{cfg}"
            self.send_device_command(cmdstr)
            print("Radio config command sent")
            return
        if cmd == "id":
            self.send_device_command("ID")
            return
        if cmd == "accept":
            if not args:
                print("Usage: /accept <callsign>")
                return
            caller = args[0]
            with self._lock:
                if caller in self.pending_incoming:
                    # remove pending request entry
                    self.pending_incoming.pop(caller, None)
                    self.send_connack(caller)
                    self.link_connected = True
                    self.link_peer = caller
                    print(f"Accepted connection from {caller}")
                else:
                    print(f"No pending request from {caller}")
            return
        if cmd == "reject":
            if not args:
                print("Usage: /reject <callsign>")
                return
            caller = args[0]
            with self._lock:
                if caller in self.pending_incoming:
                    # remove pending request entry
                    self.pending_incoming.pop(caller, None)
                    # send a DISCONN as explicit rejection
                    self.send_disconnect(caller)
                    print(f"Rejected connection from {caller}")
                else:
                    print(f"No pending request from {caller}")
            return
        if cmd == "selftest":
            # send a short test DATA frame and show serial status
            if not (self.serial and self.serial.is_open):
                print("Not connected")
                return
            print(f"Serial: port={self.serial.port} open={self.serial.is_open} rts={self.serial.rts} dtr={self.serial.dtr}")
            testmsg = f"TEST:{self.callsign}:ping"
            self.send_kiss(testmsg.encode('utf-8'))
            print("Self-test frame sent")
            return
        if cmd == "exit":
            self.disconnect()
            raise EOFError()
        print("Unknown command. Type /help")


def main():
    app = KeyboardChat()
    app.ask_setup()
    try:
        app.interactive_loop()
    except EOFError:
        pass


if __name__ == "__main__":
    main()
