# Keyboard Chat for KISS TNC

Simple cross-platform keyboard-to-keyboard chat client for communicating with a KISS TNC (LoRaTNCX project).

Features
- Interactive setup: callsign, serial port, baud
- View/update radio parameters (simple placeholder commands sent to device)
- GNSS enable/disable prompt for V4 devices (sends toggle command)
- Toggle beacons and manual beacon command
- Commands: connect, disconnect, beacon, ping, ack
- Save/load configuration to JSON

Dependencies
- Python 3.8+
- See `requirements.txt` (pyserial, prompt_toolkit)

Run

Install dependencies (PowerShell):
```powershell
python -m pip install -r tools/keyboard_chat/requirements.txt
```

Start the chat client:
```powershell
python tools/keyboard_chat/main.py
```

Notes and assumptions
- This tool communicates with the TNC over a serial port using KISS framing. It implements a minimal KISS encoder/decoder.
- Device configuration commands are sent as simple text commands (e.g. `RADIOCFG key=value`). If your device expects a different protocol, update `main.py` send logic accordingly.
- The program is intentionally simple; proposed improvements: richer GUI, secure/authenticated messages, automatic device discovery, integrated logging.
