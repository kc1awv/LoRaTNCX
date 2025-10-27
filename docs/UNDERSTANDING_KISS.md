# Understanding KISS — The "Keep It Simple, Stupid" Protocol for Amateur Radio

> *“When in doubt, simplify.”* — pretty much the spirit of KISS.

So, you’re building a KISS TNC. That’s fantastic. You’re about to implement one of the simplest, most wonderfully stubborn protocols ever devised for packet radio. KISS (short for *Keep It Simple, Stupid*) is the de-facto standard way for a computer to talk to a TNC (Terminal Node Controller). It’s not flashy, it doesn’t do handstands, and it certainly doesn’t care about your feelings. But it works, it’s minimal, and it’s everywhere.

---

## What is KISS?

KISS is a **framing protocol** that sits between a host computer (like your laptop running `direwolf`, `xastir`, or `aprsc`) and a **TNC** — a device that actually handles AX.25 packet modulation and demodulation. The TNC is basically the radio’s translator: it converts radio noise into bits and bits into radio noise.

The KISS protocol was introduced by Phil Karn (KA9Q) back in the 1980s when he got tired of TNC firmware doing too much “magic” and getting in the way. So he came up with KISS — a *bare-bones* serial framing layer that removes all the fancy smarts and leaves the host software in control.

It’s essentially: “Let the TNC send and receive raw frames, and let the computer handle the logic.”  
No nonsense, no fluff, no “AI-driven cloud innovation.” Just bytes.

---

## How KISS Works

The KISS protocol runs over a simple serial link — traditionally RS-232, but these days usually USB serial or even TCP. It wraps each AX.25 frame (the radio packet) inside a **KISS frame** so that both sides know where it begins and ends.

### Frame Structure

A single KISS frame looks like this:

`FEND | Port/Command | Data | FEND`


Where:
- **FEND (0xC0)** marks the *start* and *end* of the frame.
- **Port/Command (1 byte)** tells you what kind of frame this is (usually `0x00` for data on port 0).
- **Data** is the raw AX.25 frame or command payload.
- **FEND (again)** closes it.

Simple, right? It’s like wrapping your mail in an envelope that says “From radio port 0, with love.”

### Escaping Rules

To keep the framing from getting confused, KISS replaces certain bytes inside the data:

| Byte   | Meaning             | Escaped As  |
| ------ | ------------------- | ----------- |
| `0xC0` | FEND (Frame End)    | `0xDB 0xDC` |
| `0xDB` | FESC (Frame Escape) | `0xDB 0xDD` |

So if your payload happens to contain `0xC0`, it doesn’t accidentally end the frame early. That’s KISS’s only real party trick.

---

## Command Bytes

The **Port/Command** byte combines a 4-bit port number and a 4-bit command. The command indicates what kind of message is being sent. The most common are:

| Command | Meaning                | Typical Use                       |
| ------- | ---------------------- | --------------------------------- |
| `0x00`  | Data frame             | AX.25 frame for TX/RX             |
| `0x01`  | TX delay               | Time before PTT transmit          |
| `0x02`  | P                      | Persistence (slotted ALOHA param) |
| `0x03`  | Slot time              | Delay between retries             |
| `0x04`  | TX tail                | Time after PTT release            |
| `0x05`  | Full duplex            | Toggle mode                       |
| `0x06`  | Set hardware           | Usually ignored                   |
| `0xFF`  | Return to command mode | For TNCs that support it          |

For 99% of modern KISS implementations, you’ll only care about `0x00` (data frames). The rest are relics from when TNCs were standalone devices with their own timing controls.

---

## A Deeper Dive: The Philosophy of KISS

KISS doesn’t “understand” AX.25, APRS, or anything else you feed it. It’s gloriously stupid by design.

- **No ACKs:** The TNC doesn’t tell you if a packet was transmitted. You just trust it. Like tossing a bottle into the ocean.
- **No flow control:** Your host decides how fast to send packets. Overdo it and you’ll just buffer yourself into oblivion.
- **No protocol awareness:** KISS doesn’t know or care what’s inside your frame. It could be AX.25, Net/ROM, or a pizza recipe in binary form.

This simplicity makes it easy to implement and nearly impossible to break (unless you, you know, *try*).

---

## Example: Sending a Frame

Let’s say your AX.25 frame is this (in hex):

`82 A0 A4 40 40 60 96 88 70 9C 9E 60 03 F0 48 65 6C 6C 6F`

To send it over KISS, you’d wrap it like so:

`C0 00 82 A0 A4 40 40 60 96 88 70 9C 9E 60 03 F0 48 65 6C 6C 6F C0`

Broken down into a table for easy reading:

| FEND | Command Byte | Data                                                     | FEND |
| ---- | ------------ | -------------------------------------------------------- | ---- |
| C0   | 00           | 82 A0 A4 40 40 60 96 88 70 9C 9E 60 03 F0 48 65 6C 6C 6F | C0   |


Boom. One line, no ceremony. The `00` means “port 0, data frame.” `C0` brackets the start and end. The rest is raw AX.25 glory.

---

## Example: Implementing KISS (Pseudocode)

Here’s the gist of what a TNC firmware does:

```c
while (serial.available()) {
    uint8_t byte = serial.read();

    if (byte == FEND) {
        if (frame_in_progress) {
            process_frame(buffer, length);
            length = 0;
            frame_in_progress = false;
        } else {
            frame_in_progress = true;
        }
        continue;
    }

    if (!frame_in_progress) continue;

    if (byte == FESC) {
        next_is_escaped = true;
        continue;
    }

    if (next_is_escaped) {
        if (byte == TFEND) byte = FEND;
        if (byte == TFESC) byte = FESC;
        next_is_escaped = false;
    }

    buffer[length++] = byte;
}
```

If you squint hard enough, that’s basically the whole KISS parser. Not much to it. You can implement it on a microcontroller in about 50 lines of code and still have time to make coffee.

---

## Example: Over TCP

Some modern TNCs (and software modems like direwolf) support KISS over TCP.
In this setup, the KISS frames are sent over a TCP socket instead of a serial port. Nothing else changes — the protocol is identical. So if you’re designing a WiFi-enabled TNC, you can expose a simple TCP port (e.g., 8001) and let clients connect directly.

Example:
Connect with telnet `192.168.4.1 8001`, and you’ll see KISS frames streaming by. It's like byte by byte magic.

---

## Why KISS Still Matters

Even in 2025, KISS remains the universal lingua franca between software packet engines and hardware modems. It’s:

- Easy to debug: You can literally hex-dump it.
- Easy to port: Works on anything that can read a serial port.
- Easy to extend: Some projects add custom command bytes for GPS, telemetry, or other data.

KISS endures because it embodies the best kind of engineering minimalism — just enough to do the job, and no more.

---

## Final Thoughts

If you’re building a KISS TNC, you’re in good company. You’re continuing a decades-long tradition of not overcomplicating things. Implementing KISS correctly is a rite of passage in amateur radio digital communications.

It’s a tiny protocol, but it’s the backbone of packet radio - the thing that lets radios and computers shake hands politely instead of shouting bits at each other.

So go forth, keep it simple, and make Phil Karn proud.

---