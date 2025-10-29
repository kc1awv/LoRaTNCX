# LoRaTNCX: When AI Meets Amateur Radio 🤖📡

*"Because what could possibly go wrong when you let an AI write firmware for your radio?"*

## Preface: The Great AI Coding Experiment

Welcome to LoRaTNCX, where we decided to answer the age-old question: "Can AI actually write decent firmware, or will it just produce expensive electronic paperweights?" Spoiler alert: it's surprisingly competent, but we're keeping our debuggers close.

This project represents a fascinating experiment in human-AI collaboration for embedded systems development. Armed with a couple of shiny LoRa development boards (Heltec WiFi LoRa 32 V4, because we have standards), we set out to see just how far we could push modern AI coding assistants in the realm of amateur radio firmware development.

### Why This Madness?

Amateur radio has always been about experimentation, learning, and pushing boundaries. So naturally, when LLMs started getting scary good at writing code, we thought: "Hold my coffee, let's see if Claude can figure out power amplifier control circuits and KISS protocol implementations."

The results? Surprisingly impressive. The AI:
- ✅ Successfully reverse-engineered factory firmware behavior for PA control
- ✅ Implemented a complete KISS protocol stack from scratch
- ✅ Created proper hardware abstraction layers
- ✅ Built a working TNC that actually talks to real applications
- ❌ Still can't make coffee (yet)
- It also wrote this README, mostly.

### The Amateur Radio Angle

This isn't just "another LoRa project." We're specifically targeting amateur radio applications with proper frequency planning, amateur-appropriate configurations, and integration with existing ham radio software ecosystem. Think APRS, Winlink, packet radio, and all the digital modes we've been using since before the internet was cool.

Plus, we're using the amateur radio bands above 420MHz where bandwidth restrictions don't apply (at least in the US), so we can really open up those LoRa radios and see what they can do. Time-on-air calculations, power management, antenna patterns - all the fun stuff that makes amateur radio more than just "send message, hope it arrives."

### Want to Join the Experimental Fun?

Contributions welcome! Seriously, don't be shy. This is amateur radio, not life-saving medical equipment or nuclear reactor control systems. If it breaks, nobody dies - they just can't send APRS beacons for a while (the horror!).

### What We Want in Your Pull Requests:

- Quality over "it works": We can tell the difference between thoughtful code and "I threw spaghetti at the wall until it compiled"
- Explanations and rationale: Tell us why you did something, not just what you did
- Clear AI disclosure: If you used Claude, ChatGPT, Gemini, or whatever digital assistant helped you, just say so! Include which tools/LLMs you used
- Wetware acknowledgment: If your brain actually wrote the code without AI assistance, note that too (I promise we'll all be impressed)
- Experimentation encouraged: Try new AI coding approaches, test different LLMs, see what works best

#### The Truth About AI Detection:

Yes, we can usually tell when code was AI-generated. But this isn't a "you don't know how to code" shaming session - it's all experimental! We're all learning how to work with AI tools, not pretend they don't exist. The goal is building cool stuff and learning along the way.

#### Bottom Line:

Whether your code came from your brain, Claude's neural networks, or a fever dream induced by too much coffee and RF exposure, we want to see it. Just be honest about your process, explain your thinking, and let's build something awesome together.

## The Technical Challenge

Building a Terminal Node Controller (TNC) for the targeted hardware we have sounds simple until you realize you need to:
- Master the arcane art of power amplifier control (apparently analog writes matter, who knew?)
- Implement KISS protocol without losing your sanity
- Handle real-time packet routing between serial and RF
- Deal with hardware documentation that ranges from "adequate" to "creative fiction"
- Debug timing issues that only show up when you're demonstrating to someone important

The AI handled most of this remarkably well, though it did insist on using `digitalWrite()` for the PA power pin until we had a serious conversation about analog vs digital control.

## What We've Built

### Hardware Platform
- **Main Board:** Heltec WiFi LoRa 32 V4 (ESP32-S3 + SX1262)
- **Frequency:** 915MHz (configurable for your region)
- **Power:** Up to 22dBm output (because sometimes you need to talk to the ISS)
- **Interface:** USB-C for power/data (welcome to the future)

### Software Architecture

#### Core Components

**TNCManager** (`src/TNCManager.cpp`)
The central nervous system that coordinates everything. It's like a traffic controller, but for packets, and with more coffee dependency.

**LoRaRadio** (`src/LoRaRadio.cpp`)
Our LoRa abstraction layer that handles the RF magic. Features proven PA control methods that actually work (after much suffering and factory firmware analysis).

**KISSProtocol** (`src/KISSProtocol.cpp`)
KISS protocol implementation that talks to your favorite amateur radio applications. Supports all the standard KISS commands, plus a few extra for good measure.

**ConfigurationManager** (`src/ConfigurationManager.cpp`)
Handles all the configuration because hardcoding frequencies is for amateurs. Wait, we ARE amateurs. Well, organized amateurs.

**AmateurRadioLoRaConfigs** (`include/AmateurRadioLoRaConfigs.h`)
Pre-calculated LoRa configurations optimized for amateur radio use. Time-on-air calculations included because we actually did the math (okay, the AI did the math, but we checked it).

### Key Features

- **KISS Protocol Support**: Full implementation for seamless integration with packet radio applications
- **Amateur Radio Optimized**: Configurations designed for ham bands with proper time-on-air calculations
- **Reliable LoRa PHY**: 100% packet reception achieved (we're as surprised as you are)
- **Hardware Abstraction**: Clean separation between hardware and protocol layers
- **Real-time Operation**: Low-latency packet routing for responsive communications

## Getting Started

### Prerequisites
- PlatformIO (because Arduino IDE is for masochists)
- A Heltec WiFi LoRa 32 V4 board
- Appropriate antenna for 915MHz (or your local amateur frequency)
- Patience (debugging radio firmware builds character)

### Build and Flash

```bash
# Clone the repository
git clone https://github.com/kc1awv/LoRaTNCX.git
cd LoRaTNCX

# Build the firmware
platformio run -e heltec_wifi_lora_32_V4

# Upload to your board (pray to the embedded gods)
platformio run -e heltec_wifi_lora_32_V4 -t upload
```

### Connect and Test

1. **Hardware Setup**: Connect your board via USB-C
2. **Serial Interface**: TNC appears as a serial device (e.g., `/dev/ttyACM0` on Linux)
3. **Configuration**: Set your application to 115200 baud, 8-N-1, KISS mode
4. **Testing**: Fire up your favorite packet radio application and see if packets flow

### Quick Test with Python
```bash
# Test basic KISS functionality (if you have the test script)
python3 test_kiss_tnc.py
```

## Technical Deep Dive

### LoRa Configuration
- **Frequency:** 915MHz (ISM band, license-free portion)
- **Bandwidth:** Configurable (we have presets for amateur use)
- **Spreading Factor:** 7-12 (depending on range vs speed requirements)
- **Coding Rate:** 4/5 to 4/8 (error correction for when propagation gets weird)
- **Output Power:** Up to 22dBm (adjust for your license class and local regulations)

### KISS Protocol Implementation
Full KISS command support including:
- Data frames (the important stuff)
- TX delay, persistence, slot time (timing is everything)
- Full duplex control (because half-duplex is so last century)
- Hardware configuration (for when you need to tweak things)

### Performance Metrics
- **Packet Reception:** 100% reliability in testing (your mileage may vary with RF)
- **Throughput:** Depends on LoRa settings, up to ~20kbps with aggressive configurations
- **Latency:** Sub-50ms typical (faster than most internet comments sections)
- **Range:** 2-50km depending on configuration, terrain, and luck

## Configuration and Customization

### Amateur Radio Frequency Configurations
We've included pre-calculated configurations in `AmateurRadioLoRaConfigs.h` for various amateur radio scenarios:
- **High Speed**: Short range, high throughput (because sometimes you need speed)
- **Balanced**: Medium range, decent speed (the goldilocks option)
- **Long Range**: Maximum distance, slower speed (for when you absolutely, positively need to reach that distant repeater)

### Hardware Customization
The hardware abstraction makes it relatively easy to port to other LoRa boards. Just update the pin definitions in `HeltecV4Pins.h` and adjust the board configuration.

## Troubleshooting

### Common Issues

**"It doesn't compile!"**
- Check your PlatformIO installation
- Verify you have the RadioLib dependency
- Make sure you're using the custom board definition

**"It compiles but doesn't work!"**
- Check your PA control configuration (analog vs digital writes matter)
- Verify antenna connections (SWR kills radios)
- Confirm frequency settings match your license privileges

**"Packets are being dropped!"**
- Check timing parameters in KISS configuration
- Verify LoRa settings match on both ends
- Ensure adequate power supply (RF amplifiers are hungry)

**"The AI told me to do it this way!"**
- The AI is smart, but not infallible
- When in doubt, check against working examples
- RF engineering still requires human judgment (for now)

## Future Plans

Because every good project needs a roadmap to features that may never exist:

- [ ] **AX.25 Protocol Layer**: Full packet radio protocol stack
- [ ] **APRS Integration**: Because everyone needs to know where your radio is
- [ ] **Mesh Networking**: Turn your TNC into a mesh node
- [ ] **Web Interface**: Configure via browser (because everything needs a web interface)
- [ ] **AI-Powered Auto-Tuning**: Let the AI optimize your radio parameters
- [ ] **Skynet Integration**: Just kidding (we hope)

## Contributing

This is an experiment in AI-assisted development, so contributions are welcome! Whether you're fixing bugs the AI introduced, adding features the AI couldn't figure out, or just improving documentation the AI made too technical, we'd love your help.

Please include details about which AI tools (if any) you used in your development process. We're curious about the workflow and want to document what works best.

## The Legal Stuff (Please Don't Sue Me)

If you discover any copyrighted IP, non-free code, or improperly attributed snippets lurking in this codebase, **please let me know immediately**. I'll make accommodations faster than you can say "DMCA takedown notice." This isn't malicious - it's just the inevitable result of learning from AI that learned from the entire internet.

Think of this as "Amateur Radio meets Amateur Programming" - emphasis on the amateur part.

## License

See the LICENSE file for details. The AI didn't write the license (lawyers haven't been replaced yet).

## Acknowledgments

- **The AI Overlords**: For not making this project sentient (yet)
- **Heltec**: For making decent LoRa boards with adequate documentation
- **The Amateur Radio Community**: For decades of digital experimentation that paved the way
- **Coffee**: The real MVP of this project

---

*"73 de AI Assistant (and human collaborator) - May your packets be error-free and your RF clean."*

**Status:** ✅ Actually Working (we're as surprised as you are)  
**Last Updated:** October 29, 2025  
**AI Confidence Level:** Surprisingly High