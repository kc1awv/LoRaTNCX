#include "APRS.h"
#include "Radio.h"
#include "GNSS.h"
#include "Config.h"
#include <math.h>

APRSDriver aprs;

APRSDriver::APRSDriver() : radio(nullptr), gnss(nullptr)
{
    currentBeaconInterval = 300;
}

bool APRSDriver::begin(RadioHAL *radioPtr, GNSSDriver *gnssPtr)
{
    if (!radioPtr || !gnssPtr)
    {
        Serial.println("[APRS] Error: Invalid radio or GNSS pointer");
        return false;
    }

    radio = radioPtr;
    gnss = gnssPtr;

    loadConfig();

    Serial.println("[APRS] APRS Driver initialized");

    return true;
}

void APRSDriver::loadConfig()
{
    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    currentBeaconInterval = aprsConfig.beaconInterval;

    Serial.printf("[APRS] Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    Serial.printf("[APRS] Beacon interval: %lu seconds\n", aprsConfig.beaconInterval);
    Serial.printf("[APRS] Smart beaconing: %s\n", aprsConfig.smartBeaconing ? "Enabled" : "Disabled");

    if (!isValidCallsign(aprsConfig.callsign))
    {
        Serial.printf("[APRS] Warning: Invalid callsign '%s'\n", aprsConfig.callsign);
    }

    if (aprsConfig.ssid > 15)
    {
        Serial.printf("[APRS] Warning: Invalid SSID %d\n", aprsConfig.ssid);
    }
}

void APRSDriver::saveConfig()
{
    Serial.println("[APRS] Configuration updated");
}

void APRSDriver::poll()
{
    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    stats.uptime = millis() / 1000;

    if (aprsConfig.smartBeaconing)
    {
        updateSmartBeaconing();
    }

    if (shouldBeacon())
    {
        sendBeacon();
    }
}

bool APRSDriver::shouldBeacon()
{
    if (lastBeaconTime == 0)
    {
        return true;
    }

    unsigned long timeSinceLastBeacon = millis() - lastBeaconTime;
    return timeSinceLastBeacon >= (currentBeaconInterval * 1000UL);
}

void APRSDriver::sendBeacon()
{
    if (!gnss || !radio)
    {
        Serial.println("[APRS] Error: Radio or GNSS not initialized");
        return;
    }

    if (!gnss->locationValid())
    {
        Serial.println("[APRS] No GPS fix, skipping beacon");
        return;
    }

    String packet = createPositionReport();
    if (packet.length() == 0)
    {
        Serial.println("[APRS] Failed to create position report");
        return;
    }

    Serial.printf("[APRS] Sending beacon: %s\n", packet.c_str());

    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    String ax25Packet = createAX25Packet(
        String(aprsConfig.callsign) + "-" + String(aprsConfig.ssid),
        "APRS",
        aprsConfig.path,
        packet);

    bool success = radio->send((const uint8_t *)ax25Packet.c_str(), ax25Packet.length());

    if (success)
    {
        lastBeaconTime = millis();
        stats.beaconsSent++;
        stats.positionReports++;
        stats.lastBeacon = lastBeaconTime / 1000;

        lastBeaconLat = gnss->lat();
        lastBeaconLng = gnss->lng();
        hasValidLastPosition = true;

        Serial.printf("[APRS] Beacon sent successfully (#%lu)\n", stats.beaconsSent);
    }
    else
    {
        Serial.println("[APRS] Failed to transmit beacon");
    }
}

void APRSDriver::sendStatusBeacon(const String &status)
{
    if (!radio)
    {
        Serial.println("[APRS] Error: Radio not initialized");
        return;
    }

    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    String packet;
    if (status.length() > 0)
    {
        packet = createStatusMessage(status);
    }
    else
    {
        packet = createStatusMessage(aprsConfig.comment);
    }

    if (packet.length() == 0)
    {
        Serial.println("[APRS] Failed to create status message");
        return;
    }

    Serial.printf("[APRS] Sending status: %s\n", packet.c_str());

    String ax25Packet = createAX25Packet(
        String(aprsConfig.callsign) + "-" + String(aprsConfig.ssid),
        "APRS",
        aprsConfig.path,
        packet);

    bool success = radio->send((const uint8_t *)ax25Packet.c_str(), ax25Packet.length());

    if (success)
    {
        stats.statusMessages++;
        Serial.println("[APRS] Status beacon sent successfully");
    }
    else
    {
        Serial.println("[APRS] Failed to transmit status beacon");
    }
}

String APRSDriver::createPositionReport()
{
    if (!gnss || !gnss->locationValid())
    {
        return "";
    }

    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    double lat = gnss->lat();
    double lng = gnss->lng();

    String packet = "!";
    packet += encodeLatitude(lat);
    packet += aprsConfig.symbol.table;
    packet += encodeLongitude(lng);
    packet += aprsConfig.symbol.symbol;

    if ((aprsConfig.includeSpeed && gnss->speedValid()) || (aprsConfig.includeCourse && gnss->courseValid()))
    {
        int course = gnss->courseValid() ? (int)gnss->courseDeg() : 0;
        int speed = gnss->speedValid() ? (int)(gnss->speedKmph() * 0.539957) : 0;

        char courseSpeedBuf[8];
        snprintf(courseSpeedBuf, sizeof(courseSpeedBuf), "%03d/%03d", course, speed);
        packet += courseSpeedBuf;
    }

    if (aprsConfig.includeAltitude && gnss->altitudeValid())
    {
        int altitudeFeet = (int)(gnss->altitudeMeters() * 3.28084);
        char altBuf[16];
        snprintf(altBuf, sizeof(altBuf), "/A=%06d", altitudeFeet);
        packet += altBuf;
    }

    if (strlen(aprsConfig.comment) > 0)
    {
        packet += " ";
        packet += aprsConfig.comment;

        if (gnss->satellitesInUse() > 0)
        {
            packet += " ";
            packet += String(gnss->satellitesInUse()) + "sats";
        }
    }

    return packet;
}

String APRSDriver::createStatusMessage(const String &status)
{
    return ">" + status;
}

String APRSDriver::createMessage(const String &destination, const String &message)
{
    String packet = ":";
    packet += padCallsign(destination);
    packet += ":";
    packet += message;
    packet += "{001}";

    return packet;
}

void APRSDriver::updateSmartBeaconing()
{
    if (!gnss || !gnss->locationValid())
    {
        return;
    }

    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    currentSpeed = gnss->speedValid() ? gnss->speedKmph() : 0.0;

    bool wasMoving = moving;
    moving = currentSpeed > aprsConfig.speedThreshold;

    bool movedSignificantly = hasMovedSignificantly();

    if (moving || movedSignificantly)
    {
        currentBeaconInterval = aprsConfig.fastInterval;
        lastMovementTime = millis();
    }
    else
    {
        if (millis() - lastMovementTime > (aprsConfig.slowInterval * 1000UL))
        {
            currentBeaconInterval = aprsConfig.slowInterval;
        }
    }

    if (moving != wasMoving)
    {
        Serial.printf("[APRS] Movement state changed: %s (speed: %.1f km/h)\n",
                      moving ? "Moving" : "Stopped", currentSpeed);
        Serial.printf("[APRS] Beacon interval: %lu seconds\n", currentBeaconInterval);
    }
}

bool APRSDriver::hasMovedSignificantly()
{
    if (!hasValidLastPosition || !gnss || !gnss->locationValid())
    {
        return false;
    }

    extern ConfigManager config;
    const APRSConfig &aprsConfig = config.getAPRSConfig();

    double currentLat = gnss->lat();
    double currentLng = gnss->lng();

    float distance = calculateDistance(lastBeaconLat, lastBeaconLng, currentLat, currentLng);
    return distance > aprsConfig.minDistance;
}

float APRSDriver::calculateDistance(double lat1, double lng1, double lat2, double lng2)
{
    const double R = 6371000.0;

    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLng = (lng2 - lng1) * M_PI / 180.0;

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
                   sin(dLng / 2) * sin(dLng / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

unsigned long APRSDriver::getNextBeaconTime() const
{
    if (lastBeaconTime == 0)
    {
        return millis();
    }
    return lastBeaconTime + (currentBeaconInterval * 1000UL);
}

void APRSDriver::resetStats()
{
    stats = APRSStats{};
    Serial.println("[APRS] Statistics reset");
}

String APRSDriver::encodeLatitude(double lat)
{
    char ns = (lat >= 0) ? 'N' : 'S';
    lat = abs(lat);

    int degrees = (int)lat;
    double minutes = (lat - degrees) * 60.0;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d%05.2f%c", degrees, minutes, ns);
    return String(buffer);
}

String APRSDriver::encodeLongitude(double lng)
{
    char ew = (lng >= 0) ? 'E' : 'W';
    lng = abs(lng);

    int degrees = (int)lng;
    double minutes = (lng - degrees) * 60.0;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%03d%05.2f%c", degrees, minutes, ew);
    return String(buffer);
}

String APRSDriver::createAX25Packet(const String &source, const String &destination,
                                    const String &path, const String &payload)
{

    String packet = source + ">" + destination;

    if (path.length() > 0)
    {
        packet += "," + path;
    }

    packet += ":" + payload;

    return packet;
}

String APRSDriver::padCallsign(const String &call)
{
    String padded = call;
    while (padded.length() < 9)
    {
        padded += " ";
    }
    return padded;
}

bool APRSDriver::isValidCallsign(const String &call)
{
    if (call.length() < 3 || call.length() > 6)
    {
        return false;
    }

    if (!isAlpha(call[0]))
    {
        return false;
    }

    for (int i = 1; i < call.length(); i++)
    {
        if (!isAlphaNumeric(call[i]))
        {
            return false;
        }
    }

    return true;
}
