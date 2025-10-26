#pragma once
#include <Arduino.h>
#include <TinyGPSPlus.h>

extern void updateNmeaMonitor(const String &sentence);

class GNSSDriver
{
public:
    void begin(uint32_t baud = 9600, int rxPin = 15, int txPin = 16);
    void poll();
    bool hasFreshSentence();
    const String &lastSentence() const;

    bool locationValid() const;
    double lat() const;
    double lng() const;

    bool speedValid() const;
    double speedKmph() const;
    bool courseValid() const;
    double courseDeg() const;
    bool altitudeValid() const;
    double altitudeMeters() const;
    uint32_t satellitesInUse() const;

    void loadConfig();
    void setEnabled(bool enabled);
    bool isEnabled() const { return gnssEnabled; }

    void routeSentences();
    bool isGnssSilent() const;
    void synthesizeSentences();

    void testGnssModule();

private:
    TinyGPSPlus gps;
    String line, last;
    bool fresh = false;
    HardwareSerial *uart = &Serial1;
    uint32_t cfgBaud = 9600;
    int cfgRx = 38;
    int cfgTx = 39;
    bool gnssEnabled = true;

    unsigned long lastValidGnssTime = 0;
    unsigned long lastSynthesisTime = 0;
    double lastValidLat = 0.0;
    double lastValidLng = 0.0;
    bool hasValidPosition = false;

    String generateGPRMC();
    String generateGPGGA();
    uint8_t calculateNmeaChecksum(const String &sentence);
    void routeToOutputs(const String &sentence);
};