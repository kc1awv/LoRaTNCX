#ifndef GNSS_H
#define GNSS_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "config.h"
#include "error_handling.h"

// Default GNSS settings
#define GNSS_DEFAULT_BAUD           9600
#define GNSS_DEFAULT_PORT           10110   // TCP port for NMEA data (standard NMEA-over-TCP port)
#define GNSS_SERIAL_BUFFER_SIZE     256
#define NMEA_MAX_SENTENCE_LENGTH    82      // Standard NMEA sentence max length

// GNSS Hardware Pins (GPIO numbers)
#define GNSS_VEXT_PIN               37      // GPIO 37 - GNSS power control (active LOW)

// GNSS module class
class GNSSModule {
public:
    GNSSModule();
    ~GNSSModule();
    
    // Initialize GNSS module
    Result<void> begin(int8_t rxPin, int8_t txPin, int8_t ctrlPin = -1, 
                       int8_t wakePin = -1, int8_t ppsPin = -1, int8_t rstPin = -1,
                       uint32_t baudRate = GNSS_DEFAULT_BAUD);
    
    // Stop GNSS module
    void stop();
    
    // Check if GNSS is initialized
    bool isRunning() const { return gnssEnabled; }
    
    // Update - read from GNSS serial and process data
    void update();
    
    // Power control
    void powerOn();
    void powerOff();
    
    // Get GPS data
    // Note: These methods are not const because TinyGPS++ methods modify internal state
    bool hasValidFix() { return gps.location.isValid(); }
    double getLatitude() { return gps.location.lat(); }
    double getLongitude() { return gps.location.lng(); }
    double getAltitude() { return gps.altitude.meters(); }
    double getSpeed() { return gps.speed.knots(); }
    double getCourse() { return gps.course.deg(); }
    uint8_t getSatellites() { return gps.satellites.value(); }
    uint32_t getHDOP() { return gps.hdop.value(); }
    
    // Time functions
    bool hasValidTime() { return gps.time.isValid(); }
    uint8_t getHour() { return gps.time.hour(); }
    uint8_t getMinute() { return gps.time.minute(); }
    uint8_t getSecond() { return gps.time.second(); }
    
    // Date functions  
    bool hasValidDate() { return gps.date.isValid(); }
    uint8_t getDay() { return gps.date.day(); }
    uint8_t getMonth() { return gps.date.month(); }
    uint16_t getYear() { return gps.date.year(); }
    
    // Get last complete NMEA sentence for forwarding
    bool hasNMEASentence() const { return nmeaReady; }
    const char* getNMEASentence();
    void clearNMEASentence() { nmeaReady = false; }
    
    // Statistics
    uint32_t getCharsProcessed() const { return gps.charsProcessed(); }
    uint32_t getFailedChecksums() const { return gps.failedChecksum(); }
    uint32_t getPassedChecksums() const { return gps.passedChecksum(); }
    
    // TinyGPS++ object for direct access if needed
    TinyGPSPlus& getGPS() { return gps; }
    
private:
    HardwareSerial* gnssSerial;
    TinyGPSPlus gps;
    
    // Pin definitions
    int8_t pinRX;
    int8_t pinTX;
    int8_t pinCtrl;     // VEXT/Power control
    int8_t pinWake;     // Wake pin
    int8_t pinPPS;      // PPS (pulse per second) pin
    int8_t pinRST;      // Reset pin
    
    // Configuration
    uint32_t baudRate;
    bool gnssEnabled;
    
    // NMEA sentence buffering for TCP forwarding
    char nmeaBuffer[NMEA_MAX_SENTENCE_LENGTH + 1];
    uint8_t nmeaIndex;
    bool nmeaReady;
    bool inSentence;
    
    // Process single byte for NMEA extraction
    void processNMEAByte(char c);
};

#endif // GNSS_H
