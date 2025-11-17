#include "gnss.h"
#include "debug.h"
#include "error_handling.h"

GNSSModule::GNSSModule() 
    : gnssSerial(nullptr), pinRX(-1), pinTX(-1), pinCtrl(-1), 
      pinWake(-1), pinPPS(-1), pinRST(-1), baudRate(GNSS_DEFAULT_BAUD),
      gnssEnabled(false), nmeaIndex(0), nmeaReady(false), inSentence(false) {
    memset(nmeaBuffer, 0, sizeof(nmeaBuffer));
}

GNSSModule::~GNSSModule() {
    stop();
}

Result<void> GNSSModule::begin(int8_t rxPin, int8_t txPin, int8_t ctrlPin, 
                            int8_t wakePin, int8_t ppsPin, int8_t rstPin,
                            uint32_t baud) {
    if (gnssEnabled) {
        DEBUG_PRINTLN("GNSS already initialized");
        return Result<void>();
    }
    
    // Validate required pins
    if (rxPin < 0 || txPin < 0) {
        DEBUG_PRINTLN("GNSS: Invalid RX/TX pins");
        return Result<void>(ErrorCode::GNSS_INIT_FAILED);
    }
    
    // Store pin configuration
    pinRX = rxPin;
    pinTX = txPin;
    pinCtrl = ctrlPin;
    pinWake = wakePin;
    pinPPS = ppsPin;
    pinRST = rstPin;
    baudRate = baud;
    
    // Configure GNSS Vext (GPIO 37) - MUST be done first
    pinMode(GNSS_VEXT_PIN, OUTPUT);
    digitalWrite(GNSS_VEXT_PIN, LOW);  // Enable GNSS power (active LOW)
    delay(GNSS_POWER_ON_DELAY_MS);
    
    // Configure control pins if available
    if (pinCtrl >= 0) {
        pinMode(pinCtrl, OUTPUT);
        digitalWrite(pinCtrl, LOW);  // Enable GNSS control
    }
    
    if (pinRST >= 0) {
        pinMode(pinRST, OUTPUT);
        digitalWrite(pinRST, HIGH);  // Keep GNSS out of reset
    }
    
    if (pinWake >= 0) {
        pinMode(pinWake, OUTPUT);
        digitalWrite(pinWake, HIGH);  // Try keeping wake HIGH
    }
    
    if (pinPPS >= 0) {
        pinMode(pinPPS, INPUT);  // PPS is an input signal
    }
    
    delay(GNSS_STABILIZE_DELAY_MS);  // Give GNSS module time to stabilize after power-on
    
    // Initialize serial port (using Serial1)
    gnssSerial = &Serial1;
    gnssSerial->begin(baudRate, SERIAL_8N1, pinRX, pinTX);
    delay(GNSS_STABILIZE_DELAY_MS);  // Wait for serial to stabilize
    
    gnssEnabled = true;
    DEBUG_PRINTLN("GNSS initialized");
    
    return Result<void>();
}

void GNSSModule::stop() {
    if (!gnssEnabled) {
        return;
    }
    
    // Stop serial
    if (gnssSerial) {
        gnssSerial->end();
        gnssSerial = nullptr;
    }
    
    // Power off module
    powerOff();
    
    gnssEnabled = false;
    DEBUG_PRINTLN("GNSS stopped");
}

void GNSSModule::powerOn() {
    // GNSS module on V4 requires GPIO 37 (GNSS Vext) to be LOW
    // GPIO 34 (VGNSS_CTRL) is for additional control
#ifdef ARDUINO_HELTEC_WIFI_LORA_32_V4
    pinMode(PIN_GNSS_VEXT, OUTPUT);       // GNSS Vext
    digitalWrite(PIN_GNSS_VEXT, LOW);     // Enable GNSS Vext (active LOW)
    delay(10);
#endif
    
    if (pinCtrl >= 0) {
        // For V4, VGNSS_CTRL (GPIO 34) LOW enables additional control
        digitalWrite(pinCtrl, LOW);
        DEBUG_PRINTLN("GNSS power ON (GPIO 37 Vext + GPIO 34 CTRL enabled)");
        delay(GNSS_STABILIZE_DELAY_MS);  // Give GNSS module time to power up
    }
    
    // Release from reset
    if (pinRST >= 0) {
        digitalWrite(pinRST, HIGH);
        delay(10);  // Short delay after reset release
    }
}

void GNSSModule::powerOff() {
    if (pinCtrl >= 0) {
        // For V4, VGNSS_CTRL HIGH disables control
        digitalWrite(pinCtrl, HIGH);
    }
    
    // Turn off GNSS Vext (GPIO 37)
    digitalWrite(37, HIGH);
    DEBUG_PRINTLN("GNSS power OFF");
}

void GNSSModule::update() {
    if (!gnssEnabled || !gnssSerial) {
        return;
    }
    
    // Read all available bytes from GNSS serial
    while (gnssSerial->available() > 0) {
        char c = gnssSerial->read();
        
        // Feed to TinyGPS++ parser
        gps.encode(c);
        
        // Also extract complete NMEA sentences for TCP forwarding
        processNMEAByte(c);
    }
}

void GNSSModule::processNMEAByte(char c) {
    // NMEA sentences start with $ and end with \r\n
    // We capture the entire sentence including $ and checksum but excluding \r\n
    
    if (c == '$') {
        // Start of new sentence
        inSentence = true;
        nmeaIndex = 0;
        nmeaReady = false;
        nmeaBuffer[nmeaIndex++] = c;
    }
    else if (inSentence) {
        if (c == '\r' || c == '\n') {
            // End of sentence
            if (nmeaIndex > 0 && nmeaBuffer[0] == '$') {
                // Null-terminate and mark ready
                nmeaBuffer[nmeaIndex] = '\0';
                nmeaReady = true;
            }
            inSentence = false;
            nmeaIndex = 0;
        }
        else if (nmeaIndex < NMEA_MAX_SENTENCE_LENGTH) {
            // Accumulate sentence
            nmeaBuffer[nmeaIndex++] = c;
        }
        else {
            // Sentence too long, abort
            inSentence = false;
            nmeaIndex = 0;
        }
    }
}

const char* GNSSModule::getNMEASentence() {
    if (nmeaReady) {
        return nmeaBuffer;
    }
    return nullptr;
}
