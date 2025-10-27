# LoRaTNCX: Because Sometimes You Want to Talk to the World Without the Internet

## Another LoRa KISS TNC

*A LoRa KISS TNC that should work and doesn't make you want to throw your radio out the window*

A KISS Terminal Node Controller (TNC) implementation for LoRa radio communication, built on the Heltec WiFi LoRa 32 V4 ESP32 development board.

Bonus cookie: It can also function as an APRS tracker because WTF why not?

## Caveat Emptor (Buyer Beware, Reader Be Warned)

**Full disclosure time:** I'm not a "good" C++ programmer. Hell, I'm barely a *mediocre* C++ programmer. This entire project is essentially a discovery and research expedition into the brave new world of LLMs that can actually write code - looking at you, Claude, ChatGPT, and Gemini... but definitely not Grok, because apparently it thinks "code" is a type of breakfast cereal and I don't want it claiming I saved $65 billion by switching to AI.

### The AI Confession

Significant portions of this project were birthed from the digital minds of our AI overlords:
- **Documentation**: Much of the verbose explanations and concept breakdowns
- **Complex math stuff**: Battery management algorithms and SmartBeaconing calculations (because who has time to derive formulas when robots can do it faster?)
- **Some functional code**: Various modules where I stared blankly at the screen until an AI offered to help

This is basically a real-world experiment in "Can a human with questionable programming skills and access to AI actually build something useful?" The jury's still out, but at least it compiles!

### The Legal Stuff (Please Don't Sue Me)

If you discover any copyrighted IP, non-free code, or improperly attributed snippets lurking in this codebase, **please let me know immediately**. I'll make accommodations faster than you can say "DMCA takedown notice." This isn't malicious - it's just the inevitable result of learning from AI that learned from the entire internet.

Think of this as "Amateur Radio meets Amateur Programming" - emphasis on the *amateur* part.

### Want to Join the Experimental Fun?

**Contributions welcome!** Seriously, don't be shy. This is amateur radio, not life-saving medical equipment or nuclear reactor control systems. If it breaks, nobody dies - they just can't send APRS beacons for a while (the horror!).

**What We Want in Your Pull Requests:**
- **Quality over "it works"**: We can tell the difference between thoughtful code and "I threw spaghetti at the wall until it compiled"
- **Explanations and rationale**: Tell us *why* you did something, not just *what* you did
- **Clear AI disclosure**: If you used Claude, ChatGPT, Gemini, or whatever digital assistant helped you, just say so! Include which tools/LLMs you used
- **Wetware acknowledgment**: If your brain actually wrote the code without AI assistance, note that too (I promise we'll all be impressed)
- **Experimentation encouraged**: Try new AI coding approaches, test different LLMs, see what works best

**The Truth About AI Detection:**
Yes, we can usually tell when code was AI-generated. But this isn't a "you don't know how to code" shaming session - it's all experimental! We're all learning how to work *with* AI tools, not pretend they don't exist. The goal is building cool stuff and learning along the way.

**Bottom Line:** Whether your code came from your brain, Claude's neural networks, or a fever dream induced by too much coffee and RF exposure, we want to see it. Just be honest about your process, explain your thinking, and let's build something awesome together.

## What the Hell Is This Thing?

Good question! This is a **LoRa KISS Terminal Node Controller (TNC)** for the **Heltec WiFi LoRa 32 V4** board. It's basically a fancy radio modem that speaks the KISS protocol (the packet radio standard that's older than your favorite memes) and can also pretend to be an APRS tracker when you're feeling adventurous.

### Chat Application

Want to test this thing out? Check out the `chat_app/` directory for a simple Python keyboard-to-keyboard chat application! It's perfect for:
- **Testing your TNC setup** - Make sure everything works before diving into complex applications
- **Simple packet chat** - Talk to other LoRa stations using a clean terminal interface
- **Learning KISS protocol** - See how the protocol works in a real application
- **Station discovery** - Automatic hello beacons help you find other stations

The chat app supports both a full-featured version with colors and a simple version with minimal dependencies. See `chat_app/README.md` for details.

### Wait, What's a KISS TNC Anyway?

KISS stands for "Keep It Simple, Stupid" - a protocol designed by people who were tired of complex radio modem interfaces. A TNC (Terminal Node Controller) is basically a translator between your computer and a radio, turning digital data into radio signals and back again.

**What a KISS TNC IS:**
- A digital radio modem that doesn't suck
- A bridge between your computer and radio waves
- The thing that lets packet radio applications talk to actual radios
- A standardized way to send data over amateur radio frequencies
- Your gateway to off-grid digital communications

**What a KISS TNC is NOT:**
- A replacement for the internet (though it works when the internet doesn't)
- A fancy walkie-talkie for your smartphone
- A replacement for some newfangled mesh network (we all know those are just glorified WiFi)
- A way to stream Netflix over radio (please don't try)
- A magic device that ignores physics and propagation
- An excuse to ignore your amateur radio license requirements
- A substitute for learning how radio actually works

This project (so far) implements a dual-mode device that can operate as either:

1. **KISS TNC Mode** - A fully-featured KISS protocol TNC that bridges packet radio applications with LoRa radio hardware
2. **APRS Tracker Mode** - Broadcasts your location to the world because privacy is overrated.

## Hardware Requirements (AKA "What You Need to Buy")
### The Star of the Show
### Primary Platform

- **Heltec WiFi LoRa 32 V4** - Yes, specifically the V4. Don't come crying to me if you bought a V3 or some knockoff from AliExpress that "looks similar"
- **Heltec's GNSS Module** (optional) - integrated GNSS support if you decide to go big spender mode and opt in to the GPS module.

### What Makes This Board Not Suck
### Key Hardware Features

- ESP32-S3 (the good ESP32, not the bargain-bin version)
- SX1262 LoRa radio (433/868/915 MHz - check your local laws before accidentally becoming a pirate)
- OLED display that actually shows useful information
- Easily integrated GPS module because getting lost builds character, but finding yourself builds projects
  - Also provides PPS signal for accurate timing (NTP who?)
- LiPo battery charging circuit (shocking innovation!)
- User programmable button and LED indicators
- USB-C port (welcome to 2020+)

## Features That Actually Matter

### Core Functions

- **Wired and wireless connection options**: (USB, WiFi TCP) because choice is good
- **Battery monitoring**: Know when your device will die before it surprises you
- **Configuration persistence**: Settings stick around after power cycles so you don't have to go crazy reconfiguring every time

### KISS TNC Mode

- **KISS Protocol Support**: We're trying for a full implementation with byte stuffing and all standard commands
- **LoRa Radio Interface**: Complete RadioLib integration with configurable parameters

### APRS Tracker Mode

- **GPS position beaconing**: Tell everyone where you are!
- **Smart beaconing**: Only spam the airwaves when you're actually moving
- **Proper APRS packet formatting**: No amateur hour garbage packets

## Getting This Thing Running (The Fun Part)

### What You Need Before Starting

1. **PlatformIO** Preferably in VS Code, unless you enjoy pain
2. A **USB cable** that actually transfers data (not just power - yes, there's a difference)
3. The **Heltec WiFi LoRa 32 V4 board** (I cannot stress the V4 enough)
4. **Basic understanding of not electrocuting yourself or the hardware**

### Building and Flashing (Follow These Steps or Cry Later)

1. **Get the code**:

- Clone this repo or download it (green button, you can't miss it)

2. **Open in PlatformIO**:

- If you don't have PlatformIO installed: [Go here first](https://platformio.org/install/ide?install=vscode)
- Open the project folder in VS Code with PlatformIO extension

3. **Build it**:

- Build Target: **heltec_wifi_lora_32_V4**
- Click the checkmark (✓) button in the PlatformIO toolbar
- Watch it compile and pray to the compiler gods

4. **Upload it**:
   
- Connect your board via USB-C
- Make sure the correct COM port is selected or go Auto if you want to live dangerously
- Click the arrow (→) button to upload
- If it fails, try holding the USR/PRG button while momentarily pressing the reset button (classic ESP32 ritual)

5. **Verify it works**:
- Open serial monitor (115200 baud)
- You should see startup messages
- If you see garbage, check your baud rate (seriously, it's always the baud rate)

### Initial Setup (Don't Skip This Part)

1. **Connect to serial console** at 115200 baud (yes, this number matters)
- Send `+++` to access the configuration menu
- Configure WiFi settings (optional - defaults to AP mode)
  - **Default AP Mode**: Connect to "LoRaTNCX" network (password: "tncpass123")
  - **Station Mode**: Configure to connect to existing WiFi network


2. **Configure your settings**:
- Set your callsign (required for APRS mode)
- Set radio parameters for your region (don't be that person who violates spectrum regulations)

3. **Save settings** - they stick around between power cycles

### Network Access (Because WiFi is Magic)
- **TCP Services**: Connect applications to ports 8001 (KISS) and 10110 (NMEA)

**Default Setup (No Brain Required)**:

- **WiFi Network**: "LoRaTNCX"
- **Password**: "tncpass123"

## Usage Examples (Because Examples Don't Lie)

The device will automatically transmit APRS position beacons using GPS data with configurable intervals and smart beaconing based on movement.

### APRS Tracker Mode (GPS Stalking Made Easy)

1. Enter config with `+++`
2. Go to "APRS Settings & Operating Mode" (option 4)
3. Enable "APRS Tracker" mode
4. Set your callsign and SSID
5. Configure beacon intervals
6. Save and restart
7. Watch your position appear on APRS-IS maps worldwide

### KISS TNC Mode (For Serious Packet Radio)

- **With APRS software**: Point your app to TCP port 8001
- **With Direwolf**: Add `KISSPORT 8001` to your config
- **With anything else**: If it speaks KISS, it should work over TCP or USB CDC serial

## Applications That Play Nice

This TNC works with any software that isn't completely broken:

- **APRS clients**: APRSIS32, Xastir, APRSdroid, etc.
- **Winlink**: For when email over radio makes sense
- **Custom apps**: Python scripts, homebrew software, etc.
- **GPS applications**: Anything that likes NMEA sentences

## Support

## License and Legal Stuff

For technical issues, configuration questions, or feature requests, please refer to the future troubleshooting documentation or create an issue with detailed system information and configuration details.

This project is provided "as-is" for educational and amateur radio use. Don't blame me if it achieves sentience and takes over your shack. Make sure you comply with your local regulations - getting angry letters from the local regulatory authority is not fun.

## Contributing

Found a bug? Have an improvement? Great! Submit a pull request. Please make sure your code doesn't break existing functionality - we have enough broken things in the world already.

## Support

Before asking for help:
1. **Read this README** (you're doing that now, good job!)
2. **Check the troubleshooting docs** (`docs/TROUBLESHOOTING.md`)
3. **Search existing issues** (your problem might already be solved)

If you're still stuck, create an issue with:
- What you were trying to do
- What actually happened
- Your hardware setup
- Any error messages (the full message, not just "it doesn't work")

---

*Remember: This project is supposed to be fun. If you're not having fun, you're probably doing it wrong. Take a break, grab a coffee, and try again. The electrons will wait for you.*