# LoRaTNCX: An AI-Assisted Amateur Radio Experiment

## Or: "I Asked ChatGPT to Build a TNC and You Won't Believe What Happened Next"

Welcome to LoRaTNCX, a totally-not-terrifying experiment in using AI tools to write Amateur Radio software. Because what could possibly go wrong when you let a large language model write your radio firmware? 

Spoiler alert: It actually works. Most of the time. We're as surprised as you are.

## What Even Is This?

LoRaTNCX is a **Terminal Node Controller (TNC)** firmware for cheap ESP32-based LoRa boards. It implements the AX.25 packet radio protocol and KISS mode, making it a drop-in replacement for those vintage TNCs you paid way too much for on eBay.

**The Goal**: Create an inexpensive KISS modem for packet radio applications (and whatever new applications the Amateur Radio community dreams up in the future).

**The Method**: Human provides requirements, AI writes code, hilarity and/or functionality ensues.

**The Result**: 400KB of firmware that actually transmits packets. Your move, Skynet.

## Hardware Requirements

- **Heltec WiFi LoRa 32 V3** or **V4** board (ESP32-S3 + SX1262 LoRa radio)
- A USB cable (yes, really, we have to mention this)
- An Amateur Radio license (because here, the FCC is not amused by unlicensed experimentation, despite politics)
- Unrealistic optimism about AI-generated code

The boards cost about $25-35 USD shipped. That's less than a fancy coffee subscription, and this actually does something useful.

## Features (That Allegedly Work)

- ✅ **A Possibly Full TNC-2 Compatible Command Set** - We've got 57 commands implemented (AI counted them, we trust it)
- ✅ **KISS Mode** - Binary frame protocol for terminal programs that gave up on human readability
- ✅ **AX.25 Protocol** - Because packet radio wasn't complicated enough already
- ✅ **Multiple Operating Modes** - Command, Converse, Transparent, and KISS (we're overachievers)
- ✅ **LoRa Radio Support** - Long range, low power, high coolness factor
- ✅ **Persistent Settings** - Your configs survive power cycles (unlike our sanity)
- ✅ **Packet Monitoring** - Watch packets fly by like a very boring screensaver
- ✅ **Digipeating** - Be part of the problem... er, solution
- ✅ **Beaconing** - Announce your existence periodically, because humans need validation
- ✅ **Help System** - Extended help text that's less verbose than most AI chatbots

## AI-Generated Caveats

⚠️ **Disclaimer**: Significant portions of this codebase were written by AI. The AI:
- Has never held an Amateur Radio license
- Doesn't understand RF propagation (but neither do we, really)
- Has no concept of "good enough" and kept refactoring until told to stop
- Writes documentation better than most humans (which is concerning)
- Successfully implemented the requested commands without going full HAL 9000

⚠️ **The AI Insisted On**: Clean architecture, separation of concerns, comprehensive documentation, and proper error handling. This is either impressive or a sign of the impending robot uprising.

⚠️ **What Could Go Wrong**: Probably nothing? The worst that happens is it doesn't work and you wasted 30 minutes. The best case is you have a working KISS TNC for under $30. Risk/reward ratio seems reasonable.

## Quick Start (For the Impatient)

### Prerequisites

1. **PlatformIO** - Install the VS Code extension or CLI
   - VS Code: Search for "PlatformIO IDE" in extensions
   - CLI: `pip install platformio` (if you're into that sort of thing)

2. **A Vague Understanding of Radio** - Not strictly required, but helpful

3. **Patience** - For when the AI's code doesn't compile on first try (just kidding, it does. Well, at least it does here.)

### Building & Flashing

```bash
# Clone this magnificent specimen
git clone https://github.com/kc1awv/LoRaTNCX.git
cd LoRaTNCX

# Build for Heltec V3
platformio run --environment heltec_wifi_lora_32_V3

# Build for Heltec V4
platformio run --environment heltec_wifi_lora_32_V4

# Upload to V3 (adjust port as needed)
platformio run --environment heltec_wifi_lora_32_V3 --target upload

# Upload to V4 (adjust port as needed)
platformio run --environment heltec_wifi_lora_32_V4 --target upload
```

**Or Just Use VS Code Tasks**: Because clicking is easier than typing
- "Build LoRaTNCX V3" or "Build LoRaTNCX V4"
- "Upload (heltec_wifi_lora_32_V3)" or similar

### First Contact

1. Connect via serial at **115200 baud** (because it's not the 90s anymore)
2. Type `HELP` to see available commands
3. Marvel at the word-wrapped, alphabetically sorted output
4. Type `HELP <command>` for detailed help (the AI was thorough)
5. Configure your station:
   ```
   MYCALL N0CALL-1           # Your callsign (change this, obviously)
   FREQUENCY 906.000         # LoRa frequency in MHz
   POWER 14                  # TX power in dBm (2-20)
   MONITOR ON                # Watch the packets flow
   ```

### Entering KISS Mode

For use with packet radio applications (APRS, BPQ32, Direwolf, etc.):

```
KISS ON
RESTART
```

Exit KISS mode with ESC key or CMD_RETURN frame (0xFF). Your application probably handles this.

## Configuration Examples

### Basic Station Setup
```
MYCALL KI7EST-1             # Your call (use yours obviously, not this one)
MYALIAS WIDE1-1             # Digipeater alias
FREQUENCY 906.000           # ISM band frequency
SPREADING 7                 # LoRa spreading factor (7-12)
BANDWIDTH 125               # LoRa bandwidth in kHz
POWER 14                    # Transmit power (14 dBm = 25mW)
```

### Beacon Configuration
```
BTEXT LoRaTNCX Test Station
EVERY 600                   # Beacon every 10 minutes
BEACON EVERY 600            # Alternative syntax
```

### Monitoring Setup
```
MONITOR ON                  # Enable packet monitoring
MSTAMP ON                   # Add timestamps
MALL OFF                    # Don't show packets not for us
MRPT OFF                    # Don't show repeated packets
```

### Save & View Settings
```
DISPLAY                     # Show all current settings
                            # Settings auto-save to NVS
```

## Architecture (AI's Masterpiece)

The AI organized the code into clean, focused modules:

- **LoRaTNCX**: Main TNC class, command handlers, settings management
- **CommandProcessor**: Terminal I/O, mode switching, line editing
- **KISSProtocol**: KISS binary protocol (cleanly separated, the AI insisted)
- **LoRaRadio**: RadioLib wrapper for SX1262
- **AX25**: Frame encoding/decoding, address parsing

**Design Principles** (according to the AI):
- Single Responsibility Principle (the AI is big on SOLID)
- Separation of Concerns (no mixing business with pleasure)
- Struct-Based Organization (10 structs for 40+ settings)
- Persistent Configuration (NVS-backed, because reboots happen)


## Resource Usage (Surprisingly Reasonable)

- **Flash**: ~408KB (12.2% of 3.3MB) - Room for more AI features!
- **RAM**: ~21KB (6.6% of 320KB) - Efficient, unlike most JavaScript
- **Plenty of headroom** for future enhancements the AI will inevitably suggest


## Development Status

**Current**: Currently 57 TNC-2 commands implemented ✅  
**KISS Mode**: Fully functional ✅  
**AI Sentience Level**: Undetermined 🤖  

### What Works
- Command processing and terminal modes
- LoRa transmission and reception
- KISS protocol (tested with actual applications)
- Packet monitoring and digipeating
- Beacon transmission
- Settings persistence
- Help system (AI's pride and joy)

### What's Planned
- AX.25 Layer 2 connected mode (scaffolding exists)
- KISS parameter persistence (TXDELAY, PERSISTENCE, etc.)
- Display support (OLED sitting there unused)
- WiFi/Bluetooth connectivity (because why not)
- APRS encoding/decoding (when we feel ambitious)

### What the AI Suggested We Add
- Unit tests (adorable)
- CI/CD pipeline (overachiever)
- Formal verification (now it's just showing off)

## Contributing

Contributions welcome! Whether you're:
- A human who writes code
- An AI that writes code
- A human using AI to write code
- A very intelligent parrot

**Guidelines**:
1. Follow existing code style (the AI has opinions)
2. Create documentation (yes, really)
3. Test on hardware if possible (virtual testing only goes so far)
4. Include whether code was human, AI, or collaborative effort (for science)

## Technical Details for the Nerds

**Platform**: ESP32-S3 (Xtensa dual-core @ 240MHz)  
**Framework**: Arduino (because it just works)  
**Radio**: SX1262 LoRa (via RadioLib 7.4.0)  
**Storage**: NVS (ESP32's answer to EEPROM)  
**Build System**: PlatformIO (better than Arduino IDE, fight me)  

**Command Count**: 57 (the AI counted multiple times, it's accurate)  
**Lines of Code**: Several thousand (the AI wrote most of them)  
**Code Reviews**: Performed by humans who trust AI (probably)  
**Test Coverage**: Not enough (the AI agrees)  

## Troubleshooting

**Q**: It doesn't compile!  
**A**: Did you install PlatformIO? Is your internet working? Did you anger the AI?

**Q**: Nothing happens when I type commands!  
**A**: Check your baud rate (115200). Check your USB cable. Check your expectations.

**Q**: How do I know if the AI wrote this part?  
**A**: If it's well-documented, probably the AI. If it's a hack, probably human.

**Q**: Is this safe to use?  
**A**: Define "safe." It won't set your radio on fire. Probably won't summon Cthulhu. Works as well as most amateur radio software.

**Q**: Can I trust AI-generated code?  
**A**: You're reading a README written by AI. You've already made your choice.

## License

MIT License - Because sharing is caring, and the AI can't hold copyright anyway.

See LICENSE file for boring legal details.

## Credits

**AI Assistant**: Did most of the actual coding  
**Human Operator**: Provided requirements, testing, and existential dread  
**RadioLib**: For making LoRa not painful  
**ESP32**: For being a surprisingly capable microcontroller  
**Amateur Radio Community**: For keeping ancient protocols alive in the 21st century  

## Final Thoughts

This project demonstrates that AI can:
- Write functional embedded code
- Follow complex specifications
- Refactor code without breaking it (usually)
- Generate helpful documentation
- Understand packet radio protocols (somehow)

It cannot:
- Test hardware (yet)
- Hold an Amateur Radio license (yet)
- Appreciate the beauty of a well-executed APRS beacon (yet)

## Support

- **Issues**: GitHub issues (be nice, the AI has feelings. Probably.)
- **Questions**: GitHub discussions (humans and AIs welcome)
- **Contributions**: Pull requests (from any sentient or semi-sentient being)

73 de KI7EST (the human) and ChatGPT (the AI)

---

*Built with questionable judgment and surprisingly functional AI assistance.*  
*No AI were harmed in the making of this firmware. Some humans were mildly confused.*
