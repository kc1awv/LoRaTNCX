# Things To Work On

- ✅ **Implement web interface for device configuration and monitoring** - COMPLETED! 
> *Claude Sonnet 4 - KC1AWV - October 2025*
  - Modern Bootstrap-based interface with WebSocket real-time communication
  - ESP32 firmware integration with AsyncWebServer and WebSocket support
  - Complete WebSocket message routing for all device subsystems
  - Real-time bidirectional communication between web client and device
  - Uses only 199KB of 4MB SPIFFS (4.8%) for compressed web assets
  
  **Features Implemented:**
  - ✅ Real-time dashboard with live charts and status monitoring
  - ✅ Radio configuration with parameter adjustment via WebSocket
  - ✅ APRS message sending and monitoring interface
  - ✅ System management and configuration panels
  - ✅ Responsive Bootstrap design for mobile/desktop
  - ✅ Automated build pipeline with gzip compression
  - ✅ WebSocket server integration in ESP32 firmware
  - ✅ Message handlers for Radio, APRS, Config, GNSS, Battery systems
  - ✅ WiFi AP mode fallback with web interface at 192.168.4.1
  - ✅ Successfully tested and deployed on Heltec WiFi LoRa 32 V4
  
  **Documentation:**
  - See `/docs/dev-notes/web-interface-implementation.md` for full technical details
  - Web interface source in `/web/` directory with automated build tools
  
---

- Add support for additional modulation schemes that the SX126x supports (e.g., FSK).
  - NOT POSSIBLE WITH HELTEC WIFI LORA 32 V4 - No DIO2 pin wired to GPIO

---

- Implement IGate functionality to forward APRS packets to the internet.
- Better OLED graphics and status indicators, possibly using a graphics library.
- Better OLED power management to reduce consumption when idle.
- Implement OTA (Over-The-Air) firmware updates via WiFi. (I think it works now, but needs testing and documentation).
- Add support and/or documentation for additional LoRa frequency bands (e.g., 433MHz, 868MHz) to make the device more versatile.
- Implement advanced logging features, such as logging to an SD card or sending logs over WiFi.
- Eventually, consider adding Bluetooth LE support for configuration, monitoring, and applications via mobile apps.
- Implement adaptive data rate (ADR) for LoRa communication to optimize performance based on link conditions.
- Add support for additional sensors (e.g., temperature, humidity) and display their readings on the OLED, web interface, and over APRS.
- Implement a more user-friendly configuration interface, possibly with a web-based GUI (see above).
- Explore power-saving modes for the ESP32 to extend battery life in portable applications.
- Add support for mesh networking capabilities using LoRa. (oh god, the horror)
- Implement security features for LoRa communication, such as encryption and authentication.
- Implement a more robust error handling and recovery mechanism for LoRa communication.
- Add support for additional display types (e.g., TFT, e-ink) for more versatile user interfaces.
- Implement a more modular code structure to facilitate future development and maintenance.
- Improve documentation and add more examples for users and developers.
- Implement unit tests and integration tests to ensure code quality and reliability.
