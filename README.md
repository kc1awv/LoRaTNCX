# LoRaTNCX: An AI-Assisted Amateur Radio Experiment

## Or: "I Asked GitHub Copilot to Build a KISS TNC and You Won't Believe What Happened Next"

Welcome to LoRaTNCX, a totally-not-terrifying experiment in using AI tools to write Amateur Radio software. Because what could possibly go wrong when you let a large language model write your radio firmware? 

Spoiler alert: It actually works. Most of the time. We're as surprised as you are.

## What Even Is This?

LoRaTNCX is a **KISS TNC (Terminal Node Controller)** firmware for cheap ESP32-based LoRa boards. It's the strong, silent type - implements the KISS protocol with LoRa-specific extensions, speaks only in binary, and makes it compatible with any KISS-mode packet radio application.

**The Goal**: Create an inexpensive KISS modem for LoRa packet radio applications that works with standard KISS software (Dire Wolf, APRS clients, BPQ32, packet terminals, etc.). Because vintage TNCs cost more than this entire board.

**The Method**: Human provides requirements, AI writes code, hilarity and/or functionality ensues.

**The Result**: A fully functional KISS TNC with LoRa radio support and a command-line configuration tool. Your move, Skynet.

## Hardware Requirements

- **Heltec WiFi LoRa 32 V3** or **V4** board (ESP32-S3 + SX1262 LoRa radio)
- A USB cable (yes, really, we have to mention this)
- An Amateur Radio license (required for operation in the US and most countries - the FCC is not amused by unlicensed experimentation)
- Unrealistic optimism about AI-generated code (optional but recommended)

The boards cost about $25-35 USD shipped. That's less than a fancy coffee subscription, and unlike coffee, this actually transmits packets.

Frequency Support: **433 MHz to 928 MHz** (tested: 433, 868, 902, 915, 928 MHz) - It's like having multiple radios for the price of one!

## Features (That Actually Work)

- ✅ **Full KISS Protocol** - Binary frame protocol for programs that gave up on human readability
- ✅ **SETHARDWARE Commands** - LoRa-specific configuration via KISS extensions (because standards are more like guidelines)
- ✅ **GETHARDWARE Commands** - Query radio config, battery voltage, and board info (new and shiny!)
- ✅ **Battery Voltage Monitoring** - Keep tabs on your power situation (before it becomes critical)
- ✅ **LoRa Radio Support** - Long range, low power, high coolness factor
- ✅ **Persistent Settings** - Configuration survives power cycles (unlike our sanity during debugging)
- ✅ **Configuration Tool** - Command-line utility for easy setup (the AI wrote this too)
- ✅ **WiFi Web Interface** - Configure from any device with a browser (because it's 2025, not 1995)
- ✅ **Multiple WiFi Modes** - Access Point, Station, both, or neither (flexibility is key)
- ✅ **RESTful API** - Full JSON API for automation enthusiasts (curl your way to victory)
- ✅ **TCP KISS Server** - Network access to TNC over WiFi (because cables are so last decade)
- ✅ **OLED Display** - Real-time status on built-in screen (finally, something visible!)
- ✅ **Interrupt-Driven Reception** - Efficient packet handling with no echo loops (took a few tries to get this right)
- ✅ **Configurable Deaf Period** - Prevents receiving own transmissions (because talking to yourself is awkward)
- ✅ **Wide Frequency Range** - Supports 433-928 MHz ISM bands (way more versatile than we expected)
- ✅ **GNSS Support** - Optional GPS module for location tracking and NMEA data (V4 only, because V3 doesn't have the fancy connector)
- ✅ **NMEA over TCP** - Stream GPS data to network clients (because serial ports are so last decade)
- ✅ **GNSS Serial Passthrough** - Forward NMEA sentences to USB serial for debugging (see where you are without opening a web page)

### KISS Protocol Implementation

LoRaTNCX implements the standard KISS protocol with extensions for LoRa-specific configuration. It's like KISS, but with LoRa steroids:

**Standard KISS Commands:**
- `0x00` - DATA frame (send/receive packets)
- `0xFF` - RETURN (exit KISS mode - not implemented because we're ALWAYS in KISS mode, deal with it)

**SETHARDWARE Extensions (Command `0x06`):**
Because the original KISS spec predates LoRa by a few decades, we added some modern conveniences:
- `0x01` - Set frequency (MHz, 4-byte float)
- `0x02` - Set bandwidth (kHz, 4-byte float) 
- `0x03` - Set spreading factor (1 byte: 7-12)
- `0x04` - Set coding rate (1 byte: 5-8 for CR 4/5 through 4/8)
- `0x05` - Set output power (1 signed byte: 2-20 dBm)
- `0x06` - Get current configuration (returns all settings via GETHARDWARE)
- `0x07` - Save configuration to NVS (make it stick)
- `0x08` - Set sync word (2 bytes for SX126x)
- `0xFF` - Reset to factory defaults (when all else fails)

**GETHARDWARE Queries (Command `0x07`):**
New! Query hardware status without changing anything:
- `0x01` - Query radio configuration (frequency, bandwidth, SF, CR, power, sync word)
- `0x02` - Query battery voltage (4-byte float in volts - know before you go!)
- `0x03` - Query board information (board type and name)
- `0xFF` - Query everything (config + battery + board in one shot)

See `tools/README.md` for detailed configuration examples and the AI's thorough documentation.

### WiFi Web Interface (New!)

Because typing KISS commands and hex values is so 1990s, the TNC now includes a web interface accessible from any browser. Finally, configuration that doesn't require a PhD in hexadecimal.

**Quick Start:**
1. Upload the SPIFFS filesystem (contains the web interface):
   ```bash
   # For V3
   platformio run --environment heltec_wifi_lora_32_V3 --target uploadfs
   
   # For V4
   platformio run --environment heltec_wifi_lora_32_V4 --target uploadfs
   ```
   Or use the VS Code tasks: "Upload Filesystem (SPIFFS) V3/V4"

2. The TNC creates a WiFi access point by default:
   - **SSID**: `LoRaTNCX-XXXX` (XXXX = unique device ID)
   - **Password**: `loratncx` (yes, you should change this immediately)
   - **IP**: `192.168.4.1`

3. Connect to the WiFi network and open `http://192.168.4.1` in your browser

**Web Interface Features:**
- 📊 Real-time status display (WiFi, battery, uptime - all the important stuff)
- 🎛️ Configure all LoRa parameters (frequency, bandwidth, spreading factor, etc.)
- 📡 WiFi configuration (AP mode, Station mode, or both simultaneously)
- 🔍 WiFi network scanner (finds networks so you don't have to type SSIDs)
- 💾 Save/load configuration from flash (make it stick or start fresh)
- 🔄 Remote reboot (because finding the reset button is hard)
- 📱 Responsive design (works on phones, tablets, and those ancient desktops)

**WiFi Modes:**
- **Off (0)**: WiFi disabled - saves power, increases battery life
- **AP Only (1)**: Creates access point - good for field use, no existing network needed
- **STA Only (2)**: Connects to existing WiFi - lower power than AP mode
- **AP+STA (3)**: Both modes simultaneously - maximum flexibility, maximum power consumption

**REST API** (because some of you love `curl`):
- `GET /api/status` - Current status and battery voltage
- `GET /api/lora/config` - Current LoRa configuration
- `POST /api/lora/config` - Update LoRa settings (JSON body)
- `POST /api/lora/save` - Save LoRa config to flash
- `GET /api/wifi/config` - Current WiFi configuration
- `POST /api/wifi/config` - Update WiFi settings (JSON body)
- `GET /api/wifi/scan` - Scan for available networks
- `POST /api/reboot` - Reboot the device

**Power Consumption Note**: WiFi is power-hungry (150-200 mA in AP mode). For battery operation, configure your settings via WiFi, then switch WiFi mode to Off. Your battery will thank you.

### GNSS Support (V4 Only - Because Location Matters)

The V4 board has a connector for Heltec's optional GPS/GNSS module. If you've bolted one on, the TNC can now make use of it. Because knowing where you are is occasionally useful in radio communications.

**What It Does:**
- Receives and parses NMEA sentences from GPS module
- Extracts location, altitude, speed, heading, satellite info
- Forwards NMEA data over TCP to network clients (default port 10110)
- Optional USB serial passthrough for debugging and monitoring
- Web interface for configuration and real-time status
- Supports multiple simultaneous TCP connections (share the GPS love)

**Why You Care:**
- APRS integration - add position to your packets (future feature, but the groundwork is laid)
- Field operations - know exactly where your portable station is
- Mobile operations - track position while driving around
- Maritime/Aviation - NMEA is the universal language of navigation
- Debugging - watch NMEA sentences scroll by to verify GPS is working
- Remote monitoring - stream GPS data to multiple applications simultaneously

**Web Interface Features:**
- Enable/disable GNSS module (save power when you don't need it)
- Configure TCP port for NMEA streaming (default 10110, but be different if you want)
- Enable/disable serial passthrough to USB (watch NMEA sentences in real-time)
- Real-time status display (satellites visible, fix quality, position)
- All settings persist across reboots (set it and forget it)

**NMEA over TCP:**
- Standard NMEA-0183 format over TCP socket
- Default port 10110 (the standard NMEA-over-TCP port)
- Supports up to 4 simultaneous clients
- Works with navigation software, mapping apps, APRS tools
- Raw NMEA sentences, no processing or filtering

**Serial Passthrough:**
- Forwards NMEA sentences to USB serial at 115200 baud
- Useful for debugging and verification
- See real-time position updates without web interface
- Works with any serial terminal (PuTTY, screen, minicom, etc.)
- Includes CR+LF line endings (because terminals are picky)

**Hardware Requirements:**
- Heltec WiFi LoRa 32 V4 board (V3 doesn't have the GNSS connector - sorry!)
- Heltec GNSS module (part #6931, attaches to V4's GNSS port)
- GPS antenna (preferably one that can see the sky)
- Clear view of the sky (GPS doesn't work indoors or in Faraday cages)
- Patience (GPS cold start can take 30-60 seconds)

**Pin Configuration** (handled automatically, but for the curious):
- GPIO 37: GNSS Vext (power supply, active LOW)
- GPIO 34: VGNSS_CTRL (module control, active LOW)
- GPIO 40: Wake signal (keeps module awake)
- GPIO 42: Reset signal (not used, kept HIGH)
- GPIO 39: GNSS RX (Serial1)
- GPIO 38: GNSS TX (Serial1)
- Baud rate: 9600 (GNSS module default, not configurable)

**Example NMEA Client Usage:**
```bash
# Connect with netcat and watch NMEA sentences
nc 192.168.4.1 10110

# Or use telnet
telnet 192.168.4.1 10110

# Feed to gpsd (if you're into that sort of thing)
gpsd -N -n tcp://192.168.4.1:10110

# OpenCPN or other navigation software
# Add network NMEA connection to 192.168.4.1:10110
```

**Caveats:**
- V3 boards: No GNSS support (no hardware connector)
- V4 boards without GNSS module: Enable it anyway if you want, nothing bad happens
- Indoor operation: GPS doesn't work indoors (satellites can't see through buildings)
- Cold start: Takes 30-60 seconds to acquire satellites (hot start is faster)
- Power consumption: GNSS adds ~30-40 mA when active (disable when not needed)
- Passthrough spam: NMEA outputs multiple sentences per second (your serial terminal will scroll fast)

See `docs/GNSS_Support.md` for detailed GNSS configuration and `docs/Web_Interface_GNSS.md` for web interface details. Or just click around the web interface - it's pretty self-explanatory.

### TCP KISS Server (Network-Enabled TNC!)

Why limit yourself to USB serial when you have WiFi? The TNC includes a TCP KISS server that lets KISS applications connect over the network. It's like having a network-attached TNC without buying expensive Ethernet modules.

**How It Works:**
- When WiFi is enabled, optionally enable the TCP KISS server (default port 8001)
- KISS applications connect via TCP instead of serial port
- Full bidirectional KISS protocol over TCP - same as serial, but wireless
- Supports multiple simultaneous TCP clients (up to 4)
- Works with any KISS application that supports TCP/IP connections

**Why This Is Useful:**
- Run your KISS application on a different computer than the TNC
- Place the TNC near a window for better RF, control it from your desk
- Multiple programs can access the TNC simultaneously
- No USB cable clutter (minimalism approved)
- Integrate into network-based monitoring or control systems

**Example Usage:**
```bash
# Dire Wolf with TCP KISS (instead of serial port)
# Connect to TNC's IP address and TCP KISS port
direwolf -t 0 -k 192.168.4.1:8001

# BPQ32 TCP KISS configuration
# Add to bpq32.cfg:
# PORT
#   PORTNUM=1
#   ID=LoRa TNC
#   TYPE=ASYNC
#   PROTOCOL=KISS
#   TCPPORT=8001
#   IPADDRESS=192.168.4.1
```

**Configuration:**
- Enable/disable TCP KISS server via web interface
- Change port number (default 8001) if you want to be different
- Works in AP mode (connect to TNC's network) or STA mode (TNC on your network)
- Settings persist across reboots

**Caveats:**
- TCP adds a tiny bit of latency compared to direct serial (negligible for packet radio)
- Make sure your firewall allows the port if using STA mode
- If multiple clients send simultaneously, chaos ensues (just like with shared serial ports)

See `docs/WiFi_WebInterface.md` for web interface configuration details.

### OLED Display (Because Blinking LEDs Are So 1980s)

Both V3 and V4 boards have built-in OLED displays, and yes, they actually work now. The display shows real-time status without needing to open a web browser or connect a serial terminal.

**What's On The Screen:**
- **Boot Screen**: Shows LoRaTNCX logo and version during startup (2 seconds of glory)
- **Status Screen**: Current LoRa configuration, WiFi status, battery voltage, uptime
- **Radio Info**: Frequency, bandwidth, spreading factor, TX power - all the important numbers
- **Packet Activity**: Visual indication when transmitting or receiving (so you know it's alive)

**Screen Rotation:**
The display automatically cycles through different views, or you can configure it via the web interface. It's like having a tiny oscilloscope, but for packet radio.

**Why This Matters:**
- Quick field checks without pulling out your phone
- Verify configuration at a glance
- Battery monitoring (before it's too late)
- Looks more professional than just blinking LEDs
- Impress your ham radio friends with actual status information

**Display Management:**
- Automatically manages power to the OLED (Vext control)
- Tested on both V3 and V4 hardware (different pin configurations, same awesome display)
- Low power impact when not actively updating
- Can be disabled via configuration if you prefer the mysterious black rectangle aesthetic

No configuration needed - the display just works™ out of the box. One less thing to worry about.

## AI-Generated Caveats

⚠️ **Disclaimer**: This codebase was written collaboratively between human and AI. The AI:
- Has never held an Amateur Radio license (but aced the written exam we never gave it)
- Doesn't understand RF propagation (but learned LoRa surprisingly well)
- Successfully implemented KISS protocol without becoming self-aware
- Writes documentation better than most humans (which is either impressive or concerning)
- Fixed bugs efficiently when given proper debugging information (and coffee, metaphorically speaking)

⚠️ **The AI Insisted On**: Clean architecture, separation of concerns, comprehensive documentation, and proper error handling. This is either impressive or a sign of the impending robot uprising. We're going with "impressive" for now.

⚠️ **What Could Go Wrong**: Not much - it's been tested on actual hardware with bidirectional LoRa communication. The worst case is you need to reconfigure it. The best case is you have a working KISS TNC for under $30. Risk/reward ratio seems reasonable, unless you count the existential questions about AI-generated code.

## Quick Start (For the Impatient)

### Prerequisites

1. **PlatformIO** - Install the VS Code extension or CLI
   - VS Code: Search for "PlatformIO IDE" in extensions
   - CLI: `pip install platformio` (if you're into that sort of thing)

2. **Python 3** - For the configuration tool
   - Install pyserial: `pip3 install pyserial`
   - (Yes, you need Python to configure a device. It's 2025, deal with it.)

3. **An Amateur Radio License** - Required for legal operation (the FCC has no sense of humor about this)

### Building & Flashing

```bash
# Clone this magnificent specimen
git clone https://github.com/kc1awv/LoRaTNCX.git
cd LoRaTNCX

# Build for Heltec V3
platformio run --environment heltec_wifi_lora_32_V3

# Build for Heltec V4
platformio run --environment heltec_wifi_lora_32_V4

# Upload to V3 (adjust port as needed - typically /dev/ttyUSB0 on Linux)
platformio run --environment heltec_wifi_lora_32_V3 --target upload --upload-port /dev/ttyUSB0

# Upload to V4 (adjust port as needed - typically /dev/ttyACM0 on Linux)
platformio run --environment heltec_wifi_lora_32_V4 --target upload --upload-port /dev/ttyACM0
```

**Or Use VS Code Tasks**: Because clicking is easier than typing
- "Build LoRaTNCX V3" or "Build LoRaTNCX V4"
- Connect your board and use PlatformIO's upload button (the one with the arrow)

### Configuration

**Option 1: Web Interface (The Easy Way)**

The TNC includes a web interface that makes configuration point-and-click simple:

1. **Upload the filesystem** (first time only):
   ```bash
   # For V3
   platformio run --environment heltec_wifi_lora_32_V3 --target uploadfs
   
   # For V4  
   platformio run --environment heltec_wifi_lora_32_V4 --target uploadfs
   ```

2. **Connect to the TNC's WiFi network**:
   - Default SSID: `LoRaTNCX-XXXX` (XXXX = unique ID)
   - Default password: `loratncx`

3. **Open your browser** to `http://192.168.4.1`

4. **Configure everything** through the friendly web interface:
   - LoRa settings (frequency, bandwidth, spreading factor, etc.)
   - WiFi settings (change that password, please)
   - View system status and battery voltage
   - Save configuration to flash

No hex values, no command-line tools, no Python required. Just click buttons like a civilized human. See `docs/WiFi_WebInterface.md` for detailed web interface documentation.

**Option 2: Command-Line Tool (The Traditional Way)**

The TNC operates in KISS mode and produces no serial output except KISS frames. It's the strong, silent type - no chatty debug messages, no verbose logging, just pure binary communication. To configure it via the serial port, use the included command-line tool (which the AI also wrote):

```bash
# View current configuration (look at all those pretty numbers!)
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-config

# Check battery voltage (so you know when to charge)
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-battery

# Get all hardware info (config + battery + board)
python3 tools/loratncx_config.py /dev/ttyUSB0 --get-all

# Configure for US 33cm band (902-928 MHz)
# This is the default, but hey, maybe you changed it
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 125 \
  --spreading-factor 12 \
  --coding-rate 7 \
  --power 20 \
  --save

# Configure for long range (slower, more reliable, for when you really need those packets to arrive)
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 62.5 \
  --spreading-factor 12 \
  --save

# Configure for faster data rate (shorter range, for impatient people)
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 250 \
  --spreading-factor 7 \
  --save

# Reset to factory defaults (when you've made questionable configuration choices)
python3 tools/loratncx_config.py /dev/ttyUSB0 --reset
```

See `tools/README.md` for complete documentation of the configuration tool and more examples than you probably need.

### Default Configuration

Out of the box, the TNC is configured for maximum range (because who doesn't want maximum range?):
- **Frequency**: 915.0 MHz (US 33cm band - adjust for your location if you're not in the US)
- **Bandwidth**: 125 kHz (the Goldilocks setting - not too wide, not too narrow)
- **Spreading Factor**: SF12 (longest range, slowest speed - patience is a virtue)
- **Coding Rate**: 4/7 (good error correction)
- **Output Power**: 20 dBm (legally questionable in some places, adjust accordingly)
- **Sync Word**: 0x1424 (LoRaWAN public network - we're friendly like that)

### Using with KISS Applications

Once configured, the TNC works with any KISS-compatible application via serial port or TCP network connection. It doesn't care what you use it with:

**Serial Port (Traditional Method):**
```bash
# Dire Wolf (APRS digipeater/iGate) - because everyone loves APRS
direwolf -t 0 -p /dev/ttyUSB0 -b 115200

# BPQ32 (Packet BBS) - for those who remember the glory days
# Add to bpq32.cfg:
# PORT
#   PORTNUM=1
#   ID=LoRa TNC
#   TYPE=ASYNC
#   PROTOCOL=KISS
#   COMPORT=/dev/ttyUSB0
#   SPEED=115200

# Or use with any packet terminal that supports KISS mode
# (Your favorite ancient DOS program might even work!)
```

**TCP Network Connection (When WiFi is Enabled):**
```bash
# Dire Wolf with TCP KISS - no USB cable required!
direwolf -t 0 -k 192.168.4.1:8001

# BPQ32 with TCP KISS
# Add to bpq32.cfg:
# PORT
#   PORTNUM=1
#   ID=LoRa TNC
#   TYPE=ASYNC
#   PROTOCOL=KISS
#   TCPPORT=8001
#   IPADDRESS=192.168.4.1
#   # (Use TNC's actual IP address in STA mode)
```

**Important Note**: Most KISS applications don't support LoRa-specific parameters (because they were written in the 1990s), which is why you should pre-configure the TNC using the configuration tool before launching your KISS application. Set it and forget it!

## Configuration Examples

### Long Range Configuration
Optimized for maximum range with slower data rate:
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 62.5 \
  --spreading-factor 12 \
  --coding-rate 8 \
  --power 20 \
  --save
```
- **Range**: Maximum (~10-15 km line of sight)
- **Speed**: ~250 bps
- **Air time (50 bytes)**: ~1400 ms

### Balanced Configuration
Good balance of range and speed:
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 125 \
  --spreading-factor 9 \
  --coding-rate 7 \
  --power 17 \
  --save
```
- **Range**: Good (~5-8 km line of sight)
- **Speed**: ~2000 bps
- **Air time (50 bytes)**: ~200 ms

### Fast Data Configuration
Optimized for speed with reduced range:
```bash
python3 tools/loratncx_config.py /dev/ttyUSB0 \
  --frequency 915.0 \
  --bandwidth 250 \
  --spreading-factor 7 \
  --coding-rate 5 \
  --power 14 \
  --save
```
- **Range**: Moderate (~2-4 km line of sight)
- **Speed**: ~12500 bps
- **Air time (50 bytes)**: ~30 ms

## Architecture (The AI's Masterpiece)

The firmware is organized into clean, focused modules (because the AI read the SOLID principles and took them seriously):

- **main.cpp**: Main loop, KISS frame processing, SETHARDWARE/GETHARDWARE command handling (the brain)
- **kiss.cpp/h**: KISS protocol implementation - frame encoding/decoding, escaping (the diplomat)
- **radio.cpp/h**: LoRa radio interface - RadioLib wrapper for SX1262 (the talker)
- **config_manager.cpp/h**: Configuration management - NVS persistence, defaults (the librarian)
- **board_config.cpp/h**: Hardware-specific definitions, battery voltage reading for V3/V4 boards (the hardware whisperer)
- **wifi_manager.cpp/h**: WiFi management - AP/STA modes, connection handling (the network administrator)
- **web_server.cpp/h**: Web interface and REST API - because browsers are universal (the friendly face)
- **tcp_kiss.cpp/h**: TCP KISS server - network access to TNC, supports multiple clients (the wireless bridge)
- **display.cpp/h**: OLED display support - boot screen, status display, radio info (the informative one)
- **gnss.cpp/h**: GNSS module interface - GPS data acquisition, NMEA parsing (the navigator)
- **nmea_server.cpp/h**: NMEA-over-TCP server - streams GPS data to network clients (the GPS broadcaster)

**Design Principles** (according to the AI):
- Single Responsibility Principle (each module does one thing and does it well)
- Separation of Concerns (no mixing business with pleasure)
- Interrupt-driven packet reception (because polling is so 1980s)
- NVS-backed persistent configuration (because reboots happen)
- Async web server (because blocking is for traffic jams, not code)


## Resource Usage (Surprisingly Reasonable)

- **Flash**: ~450-470 KB (varies by board version) - WiFi and web interface take up space, who knew?
- **RAM**: ~50-80 KB (varies with WiFi activity) - Async web server is memory-efficient
- **SPIFFS**: ~30 KB (web interface files) - HTML/CSS/JS for your browsing pleasure
- **Build Time**: ~15-20 seconds - Enough time to contemplate WiFi password choices
- **Upload Time**: ~10-12 seconds - Faster than troubleshooting serial KISS commands


## Development Status

**Current**: Fully functional KISS TNC ✅  
**Configuration**: Command-line tool complete ✅  
**Testing**: Bidirectional LoRa communication verified ✅  
**AI Sentience Level**: Undetermined 🤖

### What Works
- Full KISS protocol implementation (tested with actual applications!)
- SETHARDWARE configuration commands (all subcommands working)
- GETHARDWARE query commands (radio config, battery voltage, board info - NEW!)
- Battery voltage monitoring (with proper V3.2/V4 support and ADC calibration)
- LoRa transmission and reception (interrupt-driven, because we're fancy)
- NVS configuration persistence (survives power cycles, unlike our debugging session memories)
- Command-line configuration tool (Python script that actually works, now with battery monitoring!)
- WiFi web interface (modern HTML/CSS/JS that works on phones and desktops - NEW!)
- Multiple WiFi modes (AP, STA, both, or off - choose your own adventure - NEW!)
- RESTful API (JSON-based configuration for automation enthusiasts - NEW!)
- WiFi network scanner (finds networks so you don't have to type SSIDs - NEW!)
- TCP KISS server (network access to TNC over WiFi, supports up to 4 simultaneous clients - NEW!)
- OLED display support (boot screen, status display, radio info on V3 and V4 boards - NEW!)
- GNSS support (GPS module for V4 boards with optional hardware - NEWEST!)
- NMEA over TCP (stream GPS data to network clients on port 10110 - NEWEST!)
- GNSS serial passthrough (forward NMEA to USB serial for debugging - NEWEST!)
- V3/V3.2 and V4 board support (both versions love us equally, with auto-detection)
- Wide frequency range (433-928 MHz tested - it's like a frequency buffet)
- Configurable deaf period (prevents echo loops and existential conversations with yourself)

### What's Planned (When We Feel Ambitious)
- AX.25 frame parsing/generation (for digipeating, APRS, and making this even more useful)
- APRS position beaconing with GNSS integration (because what's GPS without APRS?)
- Over-the-air firmware updates (for when you're too lazy to find a USB cable)
- Web interface authentication (because security should be more than optional)
- Additional KISS extensions (because we can never have enough features)

### What the AI Suggested We Add
- Unit tests (adorable)
- CI/CD pipeline (overachiever)
- Formal verification (now it's just showing off)

## Contributing

Contributions welcome! Whether you're:
- A human who writes code
- An AI that writes code
- A human using AI to write code
- A very intelligent parrot with typing skills

**Guidelines**:
1. Follow existing code style (the AI has opinions, and surprisingly good ones)
2. Test on actual hardware when possible (virtual LoRa doesn't quite work yet)
3. Document your changes (yes, really - future you will thank present you)
4. Note whether code was human, AI, or collaborative effort (for science, and future AI historians)

## Technical Details (For the Nerds)

**Platform**: ESP32-S3 (Xtensa dual-core @ 240MHz) - Because dual-core is twice as nice  
**Framework**: Arduino ESP32 3.20017.241212 - Because it just works™  
**Radio**: SX1262 LoRa (via RadioLib 7.4.0) - Making LoRa not painful since... well, recently  
**GNSS**: TinyGPSPlus 1.1.0 - NMEA parsing without the headache (V4 with optional GPS module)  
**Storage**: NVS (ESP32 non-volatile storage) - ESP32's fancy answer to EEPROM  
**Filesystem**: SPIFFS - For web interface files and future expansion  
**Web Server**: ESPAsyncWebServer - Non-blocking, efficient, modern  
**Display**: U8g2 library with SSD1306 OLED (128x64) - Tiny screen, big impact  
**Build System**: PlatformIO - Better than Arduino IDE, fight me  
**Protocol**: KISS with SETHARDWARE extensions - Old school meets new school  
**Frequency Range**: 433-928 MHz (hardware verified) - Surprisingly versatile little radios

**Key Implementation Details** (that you might actually care about):
- Interrupt-driven packet reception (efficient and elegant)
- Configurable deaf period (default 2000ms - prevents awkward self-conversations)
- FEND/FESC character escaping (proper KISS etiquette)
- Binary KISS frame format (because text is overrated)
- Float-packed configuration data (compact and efficient)
- Async web server (non-blocking, handles multiple connections gracefully)
- JSON REST API (ArduinoJson for serialization/deserialization)
- WiFi mode persistence (survives reboots, unlike your patience during debugging)
- TCP KISS server (supports up to 4 simultaneous clients, default port 8001)

**Lines of Code**: Several thousand (the AI wrote most of them and only needed a few debugging hints)  
**Code Reviews**: Performed by humans who trust AI (and tested on real hardware, because we're not completely reckless)  
**WiFi Reliability**: Surprisingly good (the ESP32 knows its networking)  
**Test Coverage**: Improving (we actually test on hardware, which counts for something)  

## Troubleshooting (Because Things Always Go Wrong)

**Q**: The TNC doesn't respond to the configuration tool!  
**A**: Make sure you're using the correct serial port and 115200 baud rate. On Linux, check `dmesg | tail` after plugging in the board to see which port it's assigned. If it's still not responding, try unplugging it, counting to 10, and plugging it back in. (IT's oldest trick, but it works.)

**Q**: I can transmit but not receive!  
**A**: Check that both TNCs are configured with identical frequency, bandwidth, spreading factor, coding rate, and sync word. ALL of them must match. LoRa is picky like that. One wrong parameter and you're shouting into the void.

**Q**: Packets are being received repeatedly!  
**A**: The deaf period should prevent this (default is 2000ms). If you're still seeing echo, the issue is likely two TNCs with different configurations or external interference. Or gremlins. Probably gremlins.

**Q**: How do I know what configuration to use?  
**A**: Start with the default (915 MHz, 125 kHz BW, SF12). This gives maximum range at the cost of speed. If you need faster throughput and have good line of sight, try SF7 with 250 kHz BW. There's always a tradeoff - you can't have maximum range AND maximum speed. Physics is annoying like that.

**Q**: Can I use this with Dire Wolf for APRS?  
**A**: Yes! Configure the TNC first with `loratncx_config.py`, then launch Dire Wolf with `-t 0 -p /dev/ttyUSB0 -b 115200`. Note that LoRa parameters differ from traditional VHF/UHF APRS, so don't expect to hit the existing APRS network. You're building your own LoRa APRS network. Embrace it!

**Q**: What's the range?  
**A**: Depends on configuration, antennas, and environment. With default settings (SF12, 125 kHz BW, 20 dBm) and good antennas, 10-15 km line of sight is achievable. In urban environments with obstacles, expect 1-3 km. Your mileage may vary. Literally.

**Q**: Is this legal to use?  
**A**: In the US, operation in the 33cm band (902-928 MHz) requires an Amateur Radio license. Other countries have different regulations. Check your local laws before transmitting. The FCC has no sense of humor about unlicensed operation, and neither does your local spectrum authority.

**Q**: How do I know if the AI wrote this part?  
**A**: If it's well-documented, probably the AI. If it's a clever hack, probably human. If it's this troubleshooting section's sarcasm, definitely human with AI assistance.

**Q**: The battery voltage reading is way off!  
**A**: Make sure you've uploaded the latest firmware. The V3 boards are often actually V3.2 revision (which Heltec still labels as "V3") and use inverted control logic. The latest firmware handles this automatically. If you're getting very low readings (like 0.2V), you might have an actual V3 board - open an issue and we'll add a config option.

**Q**: Can I trust AI-generated code?  
**A**: You're reading a README that was partially written by AI. You've already made your choice. (But yes, it's been tested on real hardware and actually works.)

**Q**: Can't access the web interface!  
**A**: First, make sure you uploaded the SPIFFS filesystem (`pio run --target uploadfs`). Then verify you're connected to the TNC's WiFi network (default: `LoRaTNCX-XXXX`). Open `http://192.168.4.1` in your browser. If still nothing, check WiFi mode isn't set to Off. When all else fails: unplug it, count to 10, plug it back in.

**Q**: The WiFi network doesn't appear!  
**A**: Check that WiFi mode is set to AP (1) or AP+STA (3). Default is AP mode, so if you can't see it, someone probably changed it via serial KISS commands or the config tool. You'll need to reconfigure via serial, or do a factory reset.

**Q**: WiFi is killing my battery!  
**A**: Yes, it does that. WiFi draws 150-200 mA in AP mode, which is 3-4x more than LoRa-only operation. For battery use: configure via WiFi, then switch WiFi mode to Off (0). Your battery life will improve dramatically.

**Q**: Changed the WiFi password and now I'm locked out!  
**A**: This is why we have serial ports. Connect via USB and use the Python configuration tool to reset WiFi settings. Or do a full factory reset. Consider this a valuable lesson in password management.

**Q**: Station mode won't connect to my network!  
**A**: Use the network scanner in the web interface to verify the network is visible. Double-check your SSID and password (special characters can be tricky). Make sure you're in range. If your network is 5 GHz only, bad news: ESP32 only does 2.4 GHz. Time to enable that legacy band on your router.

**Q**: Can't connect to the TCP KISS server!  
**A**: First, verify TCP KISS is enabled in the WiFi settings (check via web interface). Make sure you're using the correct port (default 8001). If in AP mode, connect to the TNC's WiFi first. If in STA mode, verify the TNC actually connected to your network (check the web interface for its IP). Firewalls can block connections - check both your computer's firewall and router settings. And yes, you need WiFi enabled for TCP KISS to work (obvious but worth stating).

**Q**: TCP KISS disconnects randomly!  
**A**: WiFi can be temperamental. Check signal strength - weak WiFi = unstable connections. If using STA mode, make sure your router isn't kicking the TNC off for inactivity. Power-saving features on the router can be problematic. Also check if you have multiple clients trying to transmit simultaneously - the TNC handles this, but applications might not handle collisions gracefully.

**Q**: GNSS not working on my V3 board!  
**A**: V3 boards don't have a GNSS connector. Only V4 boards support the optional GPS module. Check the label on your board - if it says "WiFi LoRa 32 V3", you're out of luck. Consider upgrading to V4 for GPS goodness.

**Q**: GNSS enabled but no position data!  
**A**: First, make sure you actually have the GNSS module installed (it's optional hardware). Second, GPS needs to see the sky - it doesn't work indoors or under heavy cover. Third, cold start takes 30-60 seconds to acquire satellites. Check the web interface for satellite count - if you see 0 satellites after a minute outdoors, check your antenna connection.

**Q**: NMEA data looks like gibberish!  
**A**: NMEA sentences are text-based but cryptic. `$GPRMC` and `$GPGGA` are the main ones you care about. If you're seeing random characters instead of `$GP...`, your serial port settings might be wrong (should be 115200 baud for USB passthrough). If you want human-readable data, use the web interface instead.

**Q**: Can I use GNSS for APRS position reporting?  
**A**: Not yet, but it's on the roadmap. Right now the GNSS data is available via TCP and serial passthrough. Future versions will integrate position into APRS packets. For now, you can write your own integration using the NMEA TCP stream or serial passthrough.

**Q**: GNSS draining my battery!  
**A**: GPS modules use 30-40 mA when active. If you don't need position tracking, disable GNSS via the web interface. The module will be powered off completely, saving that precious battery life for more LoRa packets.

## License

MIT License - Because sharing is caring, and the AI can't hold copyright anyway.

See LICENSE file for the boring legal details.

## Credits

**Human Operator**: Requirements, testing, hardware verification, and existential questions (KC1AWV)  
**AI Assistant**: Code generation, debugging, documentation, and surprising competence (GitHub Copilot)  
**RadioLib**: For making LoRa not painful (seriously, this library is great)  
**ESPAsyncWebServer**: For async web serving that doesn't block everything (thank you, me-no-dev/mathieucarbou)  
**ESP32**: For being a capable and affordable platform with WiFi built-in (and not bursting into flames)  
**Amateur Radio Community**: For keeping experimental radio alive in the 21st century  

## Final Thoughts

This project demonstrates that AI-assisted development can produce functional, well-documented embedded systems code. The collaborative approach between human expertise and AI capabilities resulted in a working KISS TNC implementation in a remarkably short development time.

The addition of WiFi and web interface capabilities shows that iterative development with AI assistance can continuously expand project scope without sacrificing code quality. From a simple serial KISS TNC to a full-featured web-enabled device - all through conversation and collaboration.

Key takeaways:
- AI can write functional embedded code when given clear requirements (and frequent debugging hints)
- Iterative debugging with AI assistance is highly effective (and occasionally hilarious)
- Human hardware knowledge + AI coding skills = productive combination
- Proper testing on real hardware is still essential (virtual LoRa doesn't exist yet)
- Web interfaces make embedded devices accessible to non-command-line folks (revolutionary concept!)
- The future of coding might involve more conversation with AI than Stack Overflow

Things we learned:
- AI is really good at documentation (better than most humans, tbh)
- AI needs help understanding hardware quirks (like "why is this frequency failing?")
- The combination of human domain knowledge and AI implementation skills is powerful
- Adding WiFi to things makes them infinitely more user-friendly
- AsyncWebServer is actually pretty great for embedded web interfaces
- OLED displays make projects feel complete (and look way cooler)
- U8g2 library handles different hardware revisions gracefully
- We're not quite at "Skynet" levels yet (probably)
- JSON APIs make everyone happy (except those who prefer XML, but who are we kidding?)

## Support

- **Issues**: GitHub issues for bug reports (be nice, we're all learning here)
- **Discussions**: GitHub discussions for questions and ideas (AI welcome!)
- **Pull Requests**: Contributions welcome (from any sentient or semi-sentient being)

73 de KC1AWV

---

*Built with AI assistance and validated on real hardware.*  
*No AI models were harmed in the making of this firmware. Some humans were mildly confused.*  
*If this TNC becomes self-aware, please power cycle immediately.*
