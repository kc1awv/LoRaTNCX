# LoRaTNCX — simple device test

This repository contains a small test firmware to verify Heltec WiFi LoRa 32 V3 and V4 boards in this project.

What the test does
- Blinks the onboard LED (uses `LED_BUILTIN`, defined in the board variant)
- Prints status messages to Serial at 115200 baud

Build & upload (PlatformIO)
1. Open the project in VS Code with the PlatformIO extension.
2. Build for V3:

   - Use the task "Build LoRaTNCX V3" or run PlatformIO with the environment `heltec_wifi_lora_32_V3`.

3. Build for V4:

   - Use the task "Build LoRaTNCX V4" or run PlatformIO with the environment `heltec_wifi_lora_32_V4`.

Upload and run
1. Connect the board via USB.
2. Upload using PlatformIO (select the correct environment and hit Upload).
3. Open a serial monitor at 115200 baud (e.g., PlatformIO Monitor).

Expected serial output (example):

```
LoRaTNCX test: Board: heltec_wifi_lora_32_V3
LED_BUILTIN pin: 35
millis: 500  LED: ON
millis: 1000 LED: OFF
...
```

Notes
- This test intentionally avoids initializing LoRa or the display — it only verifies the board basic I/O and serial.
- If the LED pin doesn't match your hardware, the variant files at `variants/` define `LED_BUILTIN`.
