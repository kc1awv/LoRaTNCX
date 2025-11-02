#include "StationHeard.h"
#include <algorithm>

StationHeard::StationHeard() {
    maxAge = 3600000; // 1 hour default
    stations.reserve(MAX_STATIONS);
}

void StationHeard::addStation(const String& callsign, int16_t rssi, float snr, 
                             uint8_t sf, float bandwidth, const String& message) {
    // Check if station already exists
    StationInfo* existing = findStation(callsign);
    if (existing) {
        updateStation(callsign, rssi, snr, sf, bandwidth, message);
        return;
    }
    
    // Create new station entry
    StationInfo newStation;
    newStation.callsign = callsign;
    newStation.lastHeard = millis();
    newStation.lastRSSI = rssi;
    newStation.lastSNR = snr;
    newStation.lastSF = sf;
    newStation.lastBandwidth = bandwidth;
    newStation.packetCount = 1;
    newStation.lastMessage = message;
    newStation.isDigipeater = callsign.indexOf('-') == -1; // Simple heuristic
    
    // Remove oldest if at maximum capacity
    if (stations.size() >= MAX_STATIONS) {
        removeOldStations(0); // Force removal of oldest
        if (stations.size() >= MAX_STATIONS) {
            removeOldestStation();
        }
    }
    
    stations.push_back(newStation);
}

void StationHeard::updateStation(const String& callsign, int16_t rssi, float snr, 
                                uint8_t sf, float bandwidth, const String& message) {
    StationInfo* station = findStation(callsign);
    if (station) {
        station->lastHeard = millis();
        station->lastRSSI = rssi;
        station->lastSNR = snr;
        station->lastSF = sf;
        station->lastBandwidth = bandwidth;
        station->packetCount++;
        if (!message.isEmpty()) {
            station->lastMessage = message;
        }
    }
}

StationInfo* StationHeard::findStation(const String& callsign) {
    for (auto& station : stations) {
        if (station.callsign.equalsIgnoreCase(callsign)) {
            return &station;
        }
    }
    return nullptr;
}

void StationHeard::removeOldStations(unsigned long maxAgeMs) {
    if (maxAgeMs == 0) {
        maxAgeMs = maxAge;
    }
    
    unsigned long now = millis();
    stations.erase(
        std::remove_if(stations.begin(), stations.end(),
            [now, maxAgeMs](const StationInfo& station) {
                return (now - station.lastHeard) > maxAgeMs;
            }
        ),
        stations.end()
    );
}

void StationHeard::removeOldestStation() {
    if (stations.empty()) return;
    
    auto oldest = std::min_element(stations.begin(), stations.end(),
        [](const StationInfo& a, const StationInfo& b) {
            return a.lastHeard < b.lastHeard;
        }
    );
    
    if (oldest != stations.end()) {
        stations.erase(oldest);
    }
}

void StationHeard::clearAll() {
    stations.clear();
}

void StationHeard::printHeardList(bool showTimestamp, bool showDetails) {
    removeOldStations(); // Clean up old entries first
    
    if (stations.empty()) {
        Serial.println("No stations heard");
        return;
    }
    
    // Sort by last heard time (most recent first)
    std::sort(stations.begin(), stations.end(),
        [](const StationInfo& a, const StationInfo& b) {
            return a.lastHeard > b.lastHeard;
        }
    );
    
    Serial.println();
    Serial.println("Stations Heard:");
    if (showDetails) {
        Serial.println("Callsign   Last Heard   RSSI   SNR   SF  BW   Count");
        Serial.println("-------------------------------------------------------");
        
        for (const auto& station : stations) {
            String timeStr = showTimestamp ? 
                formatTimestamp(station.lastHeard, true) : 
                formatTimestamp(station.lastHeard, false);
            
            Serial.printf("%-10s %-12s %4ddBm %4.1fdB SF%d %3.0fk %4d\n",
                          formatCallsign(station.callsign, 10).c_str(),
                          timeStr.c_str(),
                          station.lastRSSI,
                          station.lastSNR,
                          station.lastSF,
                          station.lastBandwidth,
                          station.packetCount);
        }
    } else {
        printCompactList();
    }
    Serial.printf("\nTotal: %d stations\n", stations.size());
}

void StationHeard::printCompactList() {
    Serial.println("Callsign   Age    RSSI  Count");
    Serial.println("-----------------------------");
    
    for (const auto& station : stations) {
        String timeStr = formatTimestamp(station.lastHeard, false);
        Serial.printf("%-10s %-6s %4ddBm %4d\n",
                      formatCallsign(station.callsign, 10).c_str(),
                      timeStr.c_str(),
                      station.lastRSSI,
                      station.packetCount);
    }
}

void StationHeard::printStationDetails(const String& callsign) {
    StationInfo* station = findStation(callsign);
    if (!station) {
        Serial.printf("Station %s not found in heard list\n", callsign.c_str());
        return;
    }
    
    Serial.printf("\nStation Details: %s\n", station->callsign.c_str());
    Serial.println("========================");
    Serial.printf("Last Heard:   %s\n", formatTimestamp(station->lastHeard, true).c_str());
    Serial.printf("Last RSSI:    %d dBm\n", station->lastRSSI);
    Serial.printf("Last SNR:     %.1f dB\n", station->lastSNR);
    Serial.printf("Last SF:      SF%d\n", station->lastSF);
    Serial.printf("Bandwidth:    %.0f kHz\n", station->lastBandwidth);
    Serial.printf("Packet Count: %d\n", station->packetCount);
    Serial.printf("Type:         %s\n", station->isDigipeater ? "Digipeater" : "Station");
    if (!station->lastMessage.isEmpty()) {
        Serial.printf("Last Message: %s\n", station->lastMessage.c_str());
    }
    Serial.println();
}

void StationHeard::printFilteredList(const String& pattern) {
    std::vector<StationInfo> filtered = getStationsMatching(pattern);
    
    if (filtered.empty()) {
        Serial.printf("No stations matching pattern: %s\n", pattern.c_str());
        return;
    }
    
    Serial.printf("Stations matching '%s':\n", pattern.c_str());
    Serial.println("Callsign   Last Heard   RSSI   Count");
    Serial.println("------------------------------------");
    
    for (const auto& station : filtered) {
        String timeStr = formatTimestamp(station.lastHeard, false);
        Serial.printf("%-10s %-12s %4ddBm %4d\n",
                      formatCallsign(station.callsign, 10).c_str(),
                      timeStr.c_str(),
                      station.lastRSSI,
                      station.packetCount);
    }
}

std::vector<StationInfo> StationHeard::getStationsMatching(const String& pattern) {
    std::vector<StationInfo> matches;
    String upperPattern = pattern;
    upperPattern.toUpperCase();
    
    for (const auto& station : stations) {
        String upperCall = station.callsign;
        upperCall.toUpperCase();
        
        // Simple wildcard matching (* at end)
        if (upperPattern.endsWith("*")) {
            String prefix = upperPattern.substring(0, upperPattern.length() - 1);
            if (upperCall.startsWith(prefix)) {
                matches.push_back(station);
            }
        } else if (upperCall.indexOf(upperPattern) != -1) {
            matches.push_back(station);
        }
    }
    
    return matches;
}

StationInfo StationHeard::getBestSignalStation() {
    if (stations.empty()) {
        StationInfo empty;
        empty.callsign = "";
        return empty;
    }
    
    auto best = std::max_element(stations.begin(), stations.end(),
        [](const StationInfo& a, const StationInfo& b) {
            return a.lastRSSI < b.lastRSSI;
        }
    );
    
    return *best;
}

float StationHeard::getAverageRSSI() {
    if (stations.empty()) return 0.0;
    
    float sum = 0.0;
    for (const auto& station : stations) {
        sum += station.lastRSSI;
    }
    return sum / stations.size();
}

float StationHeard::getAverageSNR() {
    if (stations.empty()) return 0.0;
    
    float sum = 0.0;
    for (const auto& station : stations) {
        sum += station.lastSNR;
    }
    return sum / stations.size();
}

String StationHeard::formatTimestamp(unsigned long timestamp, bool fullFormat) {
    unsigned long now = millis();
    unsigned long age = now - timestamp;
    
    if (fullFormat) {
        // Return actual time if RTC available, otherwise relative time
        // For now, just return relative time
        if (age < 60000) { // Less than 1 minute
            return String(age / 1000) + " sec ago";
        } else if (age < 3600000) { // Less than 1 hour
            return String(age / 60000) + " min ago";
        } else if (age < 86400000) { // Less than 24 hours
            return String(age / 3600000) + " hr ago";
        } else {
            return String(age / 86400000) + " day ago";
        }
    } else {
        // Compact format
        if (age < 60000) { // Less than 1 minute
            return String(age / 1000) + "s";
        } else if (age < 3600000) { // Less than 1 hour
            return String(age / 60000) + "m";
        } else if (age < 86400000) { // Less than 24 hours
            return String(age / 3600000) + "h";
        } else {
            return String(age / 86400000) + "d";
        }
    }
}

String StationHeard::formatCallsign(const String& call, int width) {
    if (call.length() >= width) {
        return call.substring(0, width);
    }
    return call;
}

void StationHeard::dumpToSerial() {
    Serial.printf("StationHeard dump (%d stations):\n", stations.size());
    for (size_t i = 0; i < stations.size(); i++) {
        const auto& s = stations[i];
        Serial.printf("%d: %s, %dms ago, %ddBm, %.1fdB, SF%d, %d packets\n",
                      i, s.callsign.c_str(), 
                      (int)(millis() - s.lastHeard),
                      s.lastRSSI, s.lastSNR, s.lastSF, s.packetCount);
    }
}