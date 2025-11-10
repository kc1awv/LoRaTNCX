# LoRaTNCX User Manual

## Table of Contents

1. [Introduction](sections/01-introduction.md)
   1.1. What is LoRaTNCX?
   1.2. Key Features
   1.3. Supported Hardware
   1.4. System Requirements

2. [Getting Started](sections/02-getting-started.md)
   2.1. Unboxing and Initial Setup
   2.2. Firmware Installation
   2.3. First Power-On
   2.4. Basic Configuration

3. [Hardware Overview](sections/03-hardware-overview.md)
   3.1. Heltec WiFi LoRa 32 V3 Board
   3.2. Heltec WiFi LoRa 32 V4 Board
   3.3. Power Amplifier Control Differences
   3.4. Antenna Connections
   3.5. GNSS Module (V4 Only)
   3.6. OLED Display

4. [Software Architecture](sections/04-software-architecture.md)
   4.1. KISS Protocol Implementation
   4.2. SETHARDWARE and GETHARDWARE Commands
   4.3. Interrupt-Driven Reception
   4.4. Configuration Management
   4.5. Web Server and REST API

5. [Installation and Setup](sections/05-installation-setup.md)
   5.1. PlatformIO Environment Setup
   5.2. Building the Firmware
   5.3. Uploading Firmware
   5.4. Uploading Filesystem (SPIFFS)
   5.5. Factory Reset and Erase Flash

6. [Configuration](sections/06-configuration.md)
   6.1. Using the Command-Line Tool
   6.2. Web Interface Configuration
   6.3. KISS Protocol Configuration
   6.4. Persistent Settings

7. [LoRa Radio Configuration](sections/07-lora-radio-configuration.md)
   7.1. Frequency Settings
   7.2. Bandwidth Configuration
   7.3. Spreading Factor
   7.4. Coding Rate
   7.5. Output Power
   7.6. Sync Word
   7.7. Deaf Period

8. [WiFi and Networking](sections/08-wifi-networking.md)
   8.1. WiFi Modes (Off, AP, STA, AP+STA)
   8.2. Access Point Configuration
   8.3. Station Mode Setup
   8.4. Network Scanning
   8.5. Security Considerations

9. [Web Interface](sections/09-web-interface.md)
   9.1. Accessing the Web Interface
   9.2. Status Dashboard
   9.3. LoRa Configuration Page
   9.4. WiFi Configuration Page
   9.5. GNSS Configuration Page
   9.6. REST API Usage

10. [KISS Protocol Usage](sections/10-kiss-protocol-usage.md)
    10.1. Connecting via Serial
    10.2. TCP KISS Server
    10.3. Data Frame Format
    10.4. Hardware Commands
    10.5. Integration with Applications

11. [GNSS Support](sections/11-gnss-support.md)
    11.1. Hardware Requirements
    11.2. Enabling GNSS
    11.3. NMEA over TCP
    11.4. Serial Passthrough
    11.5. Configuration Options

12. [Applications and Integration](sections/12-applications-integration.md)
    12.1. Dire Wolf Integration
    12.2. APRS Clients
    12.3. BPQ32 Configuration
    12.4. Packet Radio Software
    12.5. Custom Applications

13. [Testing and Validation](sections/13-testing-validation.md)
    13.1. Automated Test Suite
    13.2. Interactive Testing
    13.3. Range Testing
    13.4. Performance Metrics

14. [Troubleshooting](sections/14-troubleshooting.md)
    14.1. Common Issues
    14.2. Firmware Update Problems
    14.3. Communication Failures
    14.4. Power Issues
    14.5. Configuration Problems
    14.6. GNSS Issues

15. [Advanced Topics](sections/15-advanced-topics.md)
    15.1. Custom Sync Words
    15.2. Power Management
    15.3. Battery Operation
    15.4. Multiple TNC Networks
    15.5. Performance Optimization

16. [Development and Customization](sections/16-development-customization.md)
    16.1. Source Code Overview
    16.2. Adding New Features
    16.3. Modifying Configurations
    16.4. Contributing to the Project

17. [Appendices](sections/17-appendices.md)
    17.1. KISS Protocol Reference
    17.2. SETHARDWARE Command Reference
    17.3. GETHARDWARE Command Reference
    17.4. REST API Reference
    17.5. Pin Configurations
    17.6. Factory Test Procedures
    17.7. Changelog
    17.8. License and Credits

---

## About This Manual

This user manual is organized into separate files for easier maintenance and contribution. Each section focuses on a specific aspect of LoRaTNCX usage and configuration.

This manual assumes a basic understanding of amateur radio, LoRa technology, and general computing concepts.

### Caveat

- The manual was initially written mostly by various LLMs with human oversight and editing by S. Miller KC1AWV. Omissions and errors should be reported via the repository's issue tracker. No LLM or automated tool can perfectly understand all the nuances of amateur radio regulations, best practices, or specific user scenarios. Users should verify critical information independently and offer their own expertise where applicable.

### Reading the Manual

- Start with [Section 1: Introduction](sections/01-introduction.md) if you're new to LoRaTNCX
- Follow [Section 2: Getting Started](sections/02-getting-started.md) for initial setup
- Use the table of contents above to navigate to specific topics

### Contributing

This manual is written in Markdown and welcomes contributions. Each section is in its own file under the `sections/` directory. To contribute:

1. Edit the appropriate section file
2. Ensure links and references remain valid
3. Test that the manual renders correctly

### Building a PDF

To create a single PDF from all sections:

```bash
# Using Pandoc (install with: pip install pandoc)
pandoc manual/user_manual.md manual/sections/*.md -o manual/LoRaTNCX_User_Manual.pdf
```

### Online Documentation

For the latest version of this manual, visit the [LoRaTNCX GitHub repository](https://github.com/kc1awv/LoRaTNCX).