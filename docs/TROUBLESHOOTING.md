# Troubleshooting Guide

This comprehensive guide covers common issues and solutions for the LoRa KISS TNC system, including hardware problems, configuration issues, and network connectivity troubleshooting.

## System Overview

Before troubleshooting specific issues, ensure you understand the system's normal operation:
- Device boots and initializes all subsystems within 10 seconds
- OLED display shows status information and updates every 2 seconds
- WiFi connects automatically or falls back to AP mode
- Serial interface responds to `+++` configuration menu within 2 seconds
- Web interface accessible at device IP address

## Hardware Issues

### OLED Display Problems

**Symptom: Display not working or showing corrupted data**

**Diagnostic Steps:**
1. Check serial console for initialization messages:
   ```
   [OLED] Initializing I2C and display...
   [OLED] Display init result: SUCCESS/FAILED
   ```
2. Look for I2C device detection messages
3. Verify power control messages

**Common Causes and Solutions:**

**No I2C Device Detected:**
- Verify I2C pin connections (SDA=17, SCL=18 for Heltec boards)
- Check display power (VEXT pin should be LOW)
- Test with I2C scanner to detect connected devices
- Confirm display address (typically 0x3C, some use 0x3D)

**I2C Device Found but Initialization Fails:**
- Verify correct display type (SSD1306 128x64)
- Check power supply voltage (3.3V nominal)
- Test display module integrity
- Try different I2C clock rates

**Display Shows Garbled Content:**
- Check I2C signal integrity with oscilloscope
- Verify proper pull-up resistors on I2C lines
- Test for electrical interference
- Confirm correct display library configuration

### Radio Communication Issues

**Symptom: Poor range, high packet loss, or no radio activity**

**Diagnostic Steps:**
1. Check radio initialization messages in serial console
2. Verify antenna connections and specifications
3. Test with known-good stations at close range
4. Monitor RSSI and SNR values

**Antenna Problems:**
- Verify antenna is connected and appropriate for frequency
- Check SWR if using external antenna
- Ensure antenna is not damaged or obstructed
- Confirm antenna orientation (vertical for most applications)

**Radio Configuration Issues:**
- Verify frequency matches other stations
- Check spreading factor and bandwidth compatibility
- Confirm power output is appropriate for range requirements
- Validate coding rate settings

**Hardware Defects:**
- Test with different LoRa module if available
- Check for damaged connectors or traces
- Verify supply voltage to radio module
- Test SPI communication integrity

### GNSS Problems

**Symptom: No satellite fix or NMEA data**

**Diagnostic Steps:**
1. Check GNSS initialization messages
2. Verify power control settings
3. Monitor serial communication to GNSS module
4. Check for NMEA sentence reception

**Common Solutions:**
- Ensure clear sky view (outdoor operation preferred)
- Allow adequate cold start time (up to 15 minutes)
- Verify GNSS power control (VGNSS_CTRL)
- Check for interference from other electronics
- Confirm correct baud rate configuration

### Power Management Issues

**Symptom: High current consumption or unexpected shutdowns**

**Diagnostic Approaches:**
- Monitor power consumption with multimeter
- Check individual power domain current draw
- Verify battery charging circuit operation
- Test thermal performance under load

**Power Optimization:**
- Implement proper sleep modes during inactive periods
- Disable unused peripherals (OLED, GNSS when not needed)
- Optimize transmission duty cycle
- Use appropriate power levels for required range

## Configuration Issues

### Serial Menu Not Responding

**Symptom: `+++` command doesn't activate configuration menu**

**Troubleshooting Steps:**
1. Verify serial terminal settings (115200 baud, 8N1)
2. Check line ending configuration (CR+LF recommended)
3. Ensure proper timing between characters
4. Test different serial terminal software

**Terminal Configuration:**
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Parity:** None  
- **Stop Bits:** 1
- **Flow Control:** None
- **Line Ending:** Both NL & CR (CR+LF)

**Input Method:**
- Type `+++` slowly (1 second between each character)
- Press Enter after menu selections
- Allow 2-second timeout between menu activation attempts

### WiFi Connection Problems

**Symptom: Cannot connect to WiFi network or poor connectivity**

**Station Mode Issues:**
- Verify SSID and password accuracy (case-sensitive)
- Check signal strength (RSSI > -80dBm recommended)
- Confirm network security type compatibility (WPA2/WPA3)
- Test with different 2.4GHz channel

**Access Point Mode Issues:**
- Verify AP is active (check OLED display)
- Confirm client device WiFi settings
- Check for IP address conflicts
- Test with different client devices

**Network Performance:**
- Monitor packet loss and latency
- Check for interference sources
- Verify router configuration and capacity
- Test network bandwidth and stability

### Parameter Configuration Problems

**Symptom: Settings not saving or taking effect**

**Configuration Persistence:**
- Always use "Save & Exit" option (5) in configuration menu
- Verify confirmation messages after saving
- Allow device restart for some changes to take effect
- Check NVS storage status messages

**Parameter Validation:**
- Ensure values are within valid ranges
- Check for hardware limitations
- Verify regional frequency restrictions
- Confirm parameter compatibility between stations

## Network Service Issues

### TCP Service Problems

**Symptom: Cannot connect to KISS (8001) or NMEA (10110) services**

**Connection Diagnostics:**
1. Verify services are active via web interface
2. Test basic network connectivity with ping
3. Check firewall settings on client device
4. Confirm correct IP address and port numbers

**Service-Specific Issues:**

**KISS Service (Port 8001):**
- Verify single client connection limit
- Check KISS frame formatting
- Test with known-good KISS applications
- Monitor connection status on display

**NMEA Service (Port 10110, V4 only):**
- Confirm GNSS is enabled and receiving data
- Verify NMEA sentence format and checksum
- Test with standard NMEA applications
- Check for multiple client compatibility

## Performance Issues

### Slow Response or Timeouts

**System Performance:**
- Monitor CPU utilization and memory usage
- Check for memory leaks or buffer overflows
- Verify adequate power supply capacity
- Test thermal performance under load

**Network Performance:**
- Check WiFi signal strength and quality
- Monitor network congestion and interference
- Verify adequate bandwidth for application needs
- Test with reduced client connection count

### Data Loss or Corruption

**Protocol Issues:**
- Verify KISS frame integrity and checksums
- Check for buffer overflows or underruns
- Monitor error counters and statistics
- Test with lower data rates if necessary

**Hardware Issues:**
- Check signal integrity on critical interfaces
- Verify power supply stability and noise
- Test for electromagnetic interference
- Confirm proper grounding and shielding

## Diagnostic Tools and Procedures

### Built-in Diagnostics

**Serial Console Output:**
- Enable detailed logging for troubleshooting
- Monitor system messages during operation
- Check error codes and status information
- Use debug commands for specific subsystems

**OLED Display Information:**
- Monitor real-time status on all screens
- Check statistics and error counters
- Verify network and service status
- Use display for quick system assessment

**Web Interface Dashboard:**
- Access comprehensive system information
- Monitor real-time statistics and performance
- Check service status and client connections
- Use for remote diagnostics and monitoring

### External Test Tools

**Network Diagnostics:**
- Use ping to test basic connectivity
- Employ telnet for TCP service testing
- Monitor with Wireshark for protocol analysis
- Test with dedicated APRS/packet radio software

**Hardware Testing:**
- Multimeter for voltage and current measurement
- Oscilloscope for signal integrity analysis
- Spectrum analyzer for RF performance
- Logic analyzer for digital bus debugging

### Test Procedures

**System Boot Test:**
1. Connect serial terminal at 115200 baud
2. Power cycle device and monitor boot sequence
3. Verify all subsystem initialization messages
4. Check for error messages or failed components

**Network Connectivity Test:**
1. Verify WiFi connection status
2. Test ping to device IP address
3. Attempt TCP connections to service ports
4. Access web interface and verify functionality

**Radio Communication Test:**
1. Configure for known-good frequency and parameters
2. Test with another LoRa device at close range
3. Monitor transmission and reception statistics
4. Verify packet integrity and error rates

**Configuration Persistence Test:**
1. Access configuration menu and modify settings
2. Save configuration and restart device
3. Verify settings are preserved across power cycle
4. Test functionality with new configuration

## Getting Additional Help

### Information to Collect

When reporting issues, please provide:

**System Information:**
- Hardware model (Heltec V4)
- Firmware version and build date
- Configuration settings (from menu option 4)
- Operating environment details

**Error Details:**
- Complete serial console output including boot sequence
- Specific error messages and codes
- Steps to reproduce the problem
- Expected vs. actual behavior

**Hardware Setup:**
- Antenna type and connection method
- Power supply specifications
- Network configuration details
- Any additional hardware or modifications

### Escalation Process

**Level 1: Self-Diagnosis**
- Follow troubleshooting steps in this guide
- Check documentation for configuration requirements
- Test with minimal configuration
- Verify hardware connections and compatibility

**Level 2: Community Support**
- Search existing documentation and forums
- Report issues with complete diagnostic information
- Provide test results and attempted solutions
- Include hardware and software configuration details

**Level 3: Advanced Diagnostics**
- Detailed signal analysis and measurements
- Custom firmware builds for specific testing
- Hardware modification or repair requirements
- Integration with specialized test equipment

By following these systematic troubleshooting procedures, most issues can be identified and resolved quickly. The key is to gather adequate diagnostic information and approach problems methodically, starting with the most common causes and progressing to more complex scenarios as needed.