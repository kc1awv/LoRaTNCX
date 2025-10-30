#include "GNSSManager.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdint.h>

namespace
{
constexpr unsigned long FIX_TIMEOUT_MS = 4000UL;
constexpr unsigned long TIME_TIMEOUT_MS = 6000UL;
constexpr unsigned long PPS_TIMEOUT_MS = 2500UL;
}

GNSSManager *GNSSManager::activeInstance = nullptr;

GNSSManager::GNSSManager(HardwareSerial &serialPort)
    : serial(&serialPort),
      initialised(false),
      powerEnabled(false),
      fixData(),
      timeStatus(),
      ppsStatus(),
      nmeaBuffer(),
      ppsCallback(nullptr),
      ppsPulseCount(0),
      ppsLastMicros(0),
      ppsInterruptFlag(false),
      ppsHandledCount(0)
{
}

void GNSSManager::setSerialPort(HardwareSerial &serialPort)
{
    serial = &serialPort;
}

bool GNSSManager::begin(bool enablePower)
{
    if (initialised)
    {
        return true;
    }

    if (serial == nullptr)
    {
#if defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_ESP32)
        serial = &Serial1;
#else
        return false;
#endif
    }

    pinMode(GNSS_VCTL_PIN, OUTPUT);
    pinMode(GNSS_WAKE_PIN, OUTPUT);
    pinMode(GNSS_RST_PIN, OUTPUT);
    pinMode(GNSS_PPS_PIN, INPUT);

    digitalWrite(GNSS_WAKE_PIN, HIGH);
    digitalWrite(GNSS_RST_PIN, HIGH);

    setPowerEnabled(enablePower);

    serial->begin(GNSS_BAUD_RATE, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
    serial->flush();

    nmeaBuffer.reserve(128);

    activeInstance = this;
    attachInterrupt(digitalPinToInterrupt(GNSS_PPS_PIN), GNSSManager::handlePPSInterrupt, RISING);
    ppsStatus.enabled = true;
    ppsStatus.available = false;
    ppsStatus.pulseCount = 0;
    ppsStatus.lastPulseMillis = 0;
    ppsStatus.lastPulseMicros = 0;

    initialised = true;
    return true;
}

void GNSSManager::end()
{
    if (!initialised)
    {
        return;
    }

    detachInterrupt(digitalPinToInterrupt(GNSS_PPS_PIN));
    ppsStatus.enabled = false;
    ppsStatus.available = false;

    if (serial != nullptr)
    {
        serial->end();
    }

    setPowerEnabled(false);
    initialised = false;
    activeInstance = nullptr;
}

void GNSSManager::setPowerEnabled(bool enable)
{
    if (enable == powerEnabled)
    {
        return;
    }

    digitalWrite(GNSS_VCTL_PIN, enable ? HIGH : LOW);
    if (enable)
    {
        digitalWrite(GNSS_WAKE_PIN, HIGH);
        digitalWrite(GNSS_RST_PIN, LOW);
        delay(5);
        digitalWrite(GNSS_RST_PIN, HIGH);
    }
    else
    {
        digitalWrite(GNSS_WAKE_PIN, LOW);
        digitalWrite(GNSS_RST_PIN, LOW);
    }

    powerEnabled = enable;

    if (!enable)
    {
        fixData.valid = false;
        fixData.active = false;
        timeStatus.valid = false;
        ppsStatus.available = false;
    }
}

void GNSSManager::update()
{
    if (!initialised || serial == nullptr || !powerEnabled)
    {
        return;
    }

    while (serial->available())
    {
        char c = static_cast<char>(serial->read());
        if (c == '\r' || c == '\n')
        {
            if (nmeaBuffer.length() > 0)
            {
                handleNMEALine(nmeaBuffer);
                nmeaBuffer = "";
            }
        }
        else
        {
            if (nmeaBuffer.length() < 120)
            {
                nmeaBuffer += c;
            }
            else
            {
                nmeaBuffer = "";
            }
        }
    }

    processPPS();

    unsigned long now = millis();
    if (fixData.valid && (now - fixData.lastUpdateMillis) > FIX_TIMEOUT_MS)
    {
        fixData.valid = false;
        fixData.active = false;
        fixData.is3DFix = false;
    }

    if (timeStatus.valid && (now - timeStatus.lastUpdateMillis) > TIME_TIMEOUT_MS)
    {
        timeStatus.valid = false;
        timeStatus.synced = false;
    }

    if (ppsStatus.available && (now - ppsStatus.lastPulseMillis) > PPS_TIMEOUT_MS)
    {
        ppsStatus.available = false;
    }
}

void GNSSManager::setPPSCallback(PPSCallback callback)
{
    ppsCallback = callback;
}

GNSSManager::PPSStatus GNSSManager::getPPSStatus() const
{
    return ppsStatus;
}

bool GNSSManager::syncSystemTime()
{
    if (!timeStatus.valid)
    {
        return false;
    }

    struct timeval tv;
    tv.tv_sec = timeStatus.epoch;
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) == 0)
    {
        timeStatus.synced = true;
        return true;
    }

    return false;
}

void IRAM_ATTR GNSSManager::handlePPSInterrupt()
{
    if (activeInstance == nullptr)
    {
        return;
    }

    activeInstance->ppsInterruptFlag = true;
    activeInstance->ppsPulseCount++;
    activeInstance->ppsLastMicros = micros();
}

void GNSSManager::processPPS()
{
    if (!ppsStatus.enabled)
    {
        return;
    }

    uint32_t pulseCount;
    unsigned long lastMicros;
    bool flag;

    noInterrupts();
    pulseCount = ppsPulseCount;
    lastMicros = ppsLastMicros;
    flag = ppsInterruptFlag;
    if (flag)
    {
        ppsInterruptFlag = false;
    }
    interrupts();

    if (pulseCount != ppsHandledCount)
    {
        ppsHandledCount = pulseCount;
        ppsStatus.pulseCount = pulseCount;
        ppsStatus.lastPulseMicros = lastMicros;
        ppsStatus.lastPulseMillis = millis();
        ppsStatus.available = true;

        if (ppsCallback)
        {
            ppsCallback(pulseCount);
        }
    }
}

void GNSSManager::handleNMEALine(const String &line)
{
    String trimmed = line;
    trimmed.trim();

    if (trimmed.length() < 6 || trimmed.charAt(0) != '$')
    {
        return;
    }

    if (!validateChecksum(trimmed))
    {
        return;
    }

    int starIndex = trimmed.indexOf('*');
    if (starIndex < 0)
    {
        return;
    }

    String payload = trimmed.substring(1, starIndex);

    std::vector<String> tokens;
    tokens.reserve(16);
    int start = 0;
    int length = payload.length();
    while (start <= length)
    {
        int comma = payload.indexOf(',', start);
        if (comma < 0)
        {
            tokens.emplace_back(payload.substring(start));
            break;
        }
        tokens.emplace_back(payload.substring(start, comma));
        start = comma + 1;
        if (start == length)
        {
            tokens.emplace_back("");
            break;
        }
    }

    if (tokens.empty())
    {
        return;
    }

    const String &sentence = tokens.front();
    if (sentence.endsWith("RMC"))
    {
        parseRMC(tokens);
    }
    else if (sentence.endsWith("GGA"))
    {
        parseGGA(tokens);
    }
}

void GNSSManager::parseRMC(const std::vector<String> &tokens)
{
    if (tokens.size() < 10)
    {
        return;
    }

    const String statusToken = tokens.size() > 2 ? tokens[2] : "";
    fixData.active = (statusToken == "A");

    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;

    bool timeParsed = parseTimeToken(tokens[1], hour, minute, second);
    bool dateParsed = parseDateToken(tokens[9], day, month, year);

    if (tokens.size() > 3 && tokens.size() > 4)
    {
        fixData.latitude = parseCoordinate(tokens[3], tokens[4]);
    }

    if (tokens.size() > 5 && tokens.size() > 6)
    {
        fixData.longitude = parseCoordinate(tokens[5], tokens[6]);
    }

    if (tokens.size() > 7)
    {
        fixData.speedKnots = tokens[7].toFloat();
    }

    if (tokens.size() > 8)
    {
        fixData.courseDegrees = tokens[8].toFloat();
    }

    if (fixData.active)
    {
        fixData.valid = true;
        fixData.lastUpdateMillis = millis();
        if (timeParsed && dateParsed)
        {
            fixData.timestamp = buildEpoch(year, month, day, hour, minute, second);
        }
    }

    if (timeParsed && dateParsed)
    {
        timeStatus.valid = true;
        timeStatus.lastUpdateMillis = millis();
        timeStatus.hour = hour;
        timeStatus.minute = minute;
        timeStatus.second = second;
        timeStatus.day = day;
        timeStatus.month = month;
        timeStatus.year = year;
        timeStatus.epoch = buildEpoch(year, month, day, hour, minute, second);
    }
}

void GNSSManager::parseGGA(const std::vector<String> &tokens)
{
    if (tokens.size() < 10)
    {
        return;
    }

    if (tokens.size() > 1)
    {
        uint8_t hour = 0;
        uint8_t minute = 0;
        uint8_t second = 0;
        if (parseTimeToken(tokens[1], hour, minute, second))
        {
            timeStatus.hour = hour;
            timeStatus.minute = minute;
            timeStatus.second = second;
        }
    }

    if (tokens.size() > 2 && tokens.size() > 3)
    {
        fixData.latitude = parseCoordinate(tokens[2], tokens[3]);
    }

    if (tokens.size() > 4 && tokens.size() > 5)
    {
        fixData.longitude = parseCoordinate(tokens[4], tokens[5]);
    }

    int fixQuality = tokens.size() > 6 ? tokens[6].toInt() : 0;
    fixData.is3DFix = (fixQuality >= 2);
    fixData.active = (fixQuality > 0);
    fixData.valid = (fixQuality > 0);

    if (tokens.size() > 7)
    {
        fixData.satellites = static_cast<uint8_t>(tokens[7].toInt());
    }

    if (tokens.size() > 8)
    {
        fixData.hdop = tokens[8].toFloat();
    }

    if (tokens.size() > 9)
    {
        fixData.altitudeMeters = tokens[9].toFloat();
    }

    fixData.lastUpdateMillis = millis();
}

bool GNSSManager::validateChecksum(const String &line) const
{
    int starIndex = line.indexOf('*');
    if (starIndex <= 0 || starIndex + 2 >= line.length())
    {
        return false;
    }

    uint8_t checksum = 0;
    for (int i = 1; i < starIndex; ++i)
    {
        checksum ^= static_cast<uint8_t>(line.charAt(i));
    }

    String provided = line.substring(starIndex + 1, starIndex + 3);
    uint8_t expected = static_cast<uint8_t>(strtoul(provided.c_str(), nullptr, 16));
    return checksum == expected;
}

double GNSSManager::parseCoordinate(const String &value, const String &direction)
{
    if (value.length() == 0)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double raw = value.toDouble();
    int degrees = static_cast<int>(raw / 100.0);
    double minutes = raw - (degrees * 100.0);
    double decimal = static_cast<double>(degrees) + (minutes / 60.0);

    if (direction == "S" || direction == "W")
    {
        decimal = -decimal;
    }

    return decimal;
}

bool GNSSManager::parseTimeToken(const String &token, uint8_t &hour, uint8_t &minute, uint8_t &second)
{
    if (token.length() < 6)
    {
        return false;
    }

    hour = static_cast<uint8_t>(token.substring(0, 2).toInt());
    minute = static_cast<uint8_t>(token.substring(2, 4).toInt());
    second = static_cast<uint8_t>(token.substring(4, 6).toInt());
    return true;
}

bool GNSSManager::parseDateToken(const String &token, uint8_t &day, uint8_t &month, uint16_t &year)
{
    if (token.length() != 6)
    {
        return false;
    }

    day = static_cast<uint8_t>(token.substring(0, 2).toInt());
    month = static_cast<uint8_t>(token.substring(2, 4).toInt());
    year = static_cast<uint16_t>(token.substring(4, 6).toInt());

    if (year < 80)
    {
        year += 2000;
    }
    else
    {
        year += 1900;
    }

    return true;
}

time_t GNSSManager::buildEpoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    // Convert the calendar date to a days-since-1970 count using Howard Hinnant's algorithm.
    int64_t y = static_cast<int64_t>(year);
    int64_t m = static_cast<int64_t>(month);
    int64_t d = static_cast<int64_t>(day);

    y -= (m <= 2) ? 1 : 0;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t moy = m + (m > 2 ? -3 : 9);
    const int64_t doy = (153 * moy + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t daysSinceEpoch = era * 146097 + doe - 719468; // 719468 = days from 0000-03-01 to 1970-01-01

    int64_t totalSeconds = daysSinceEpoch * 86400LL;
    totalSeconds += static_cast<int64_t>(hour) * 3600LL;
    totalSeconds += static_cast<int64_t>(minute) * 60LL;
    totalSeconds += static_cast<int64_t>(second);

    return static_cast<time_t>(totalSeconds);
}
