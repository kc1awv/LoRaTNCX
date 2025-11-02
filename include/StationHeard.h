#ifndef STATIONHEARD_H
#define STATIONHEARD_H

#include <Arduino.h>
#include <vector>

struct StationInfo {
    String callsign;
    unsigned long lastHeard;
    int16_t lastRSSI;
    float lastSNR;
    uint8_t lastSF;
    float lastBandwidth;
    uint16_t packetCount;
    String lastMessage;
    bool isDigipeater;
};

class StationHeard {
private:
    std::vector<StationInfo> stations;
    static const size_t MAX_STATIONS = 50;
    unsigned long maxAge;  // Maximum age in milliseconds
    
    // Helper methods
    String formatTimestamp(unsigned long timestamp, bool fullFormat = false);
    String formatCallsign(const String& call, int width = 10);
    void removeOldestStation();
    
public:
    StationHeard();
    
    // Station management
    void addStation(const String& callsign, int16_t rssi, float snr, 
                   uint8_t sf, float bandwidth = 125.0, const String& message = "");
    void updateStation(const String& callsign, int16_t rssi, float snr, 
                      uint8_t sf, float bandwidth = 125.0, const String& message = "");
    StationInfo* findStation(const String& callsign);
    
    // List management
    void removeOldStations(unsigned long maxAge = 3600000); // 1 hour default
    void clearAll();
    void setMaxAge(unsigned long age) { maxAge = age; }
    
    // Display methods
    void printHeardList(bool showTimestamp = false, bool showDetails = true);
    void printCompactList();
    void printStationDetails(const String& callsign);
    
    // Statistics
    size_t getCount() const { return stations.size(); }
    StationInfo getBestSignalStation();
    float getAverageRSSI();
    float getAverageSNR();
    
    // Filtering
    void printFilteredList(const String& pattern);
    std::vector<StationInfo> getStationsMatching(const String& pattern);
    
    // Export/Import (for debugging)
    void dumpToSerial();
};

#endif