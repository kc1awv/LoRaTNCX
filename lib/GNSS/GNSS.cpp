#include "GNSS.h"
#include "Config.h"
#include <WiFi.h>

extern WiFiClient nmeaClient;

void GNSSDriver::begin(uint32_t baud, int rxPin, int txPin)
{
    cfgBaud = baud;
    cfgRx = rxPin;
    cfgTx = txPin;

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[GNSS] Initializing UART: RX=%d, TX=%d, Baud=%lu\n", rxPin, txPin, baud);
    #endif
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);

    if (config.getGNSSConfig().verboseLogging)
    {
        int rxState = digitalRead(rxPin);
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] Initial RX pin %d state: %s\n", rxPin, rxState ? "HIGH" : "LOW");
        #endif
    }

    uart->begin(cfgBaud, SERIAL_8N1, cfgRx, cfgTx);

    delay(100);

    if (config.getGNSSConfig().verboseLogging)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS] Testing for GNSS module response...");
        #endif
    }

    lastValidGnssTime = millis();
}

void GNSSDriver::poll()
{
    static unsigned long lastDebugTime = 0;
    static bool firstCheck = true;

    if (!gnssEnabled)
    {
        if (firstCheck)
        {
            #ifndef KISS_SERIAL_MODE
            Serial.println("[GNSS] GNSS is disabled in configuration");
            #endif
            firstCheck = false;
        }
        return;
    }

    if (firstCheck)
    {
        GNSSConfig &cfg = config.getGNSSConfig();
        if (cfg.verboseLogging)
        {
            #ifndef KISS_SERIAL_MODE
            Serial.println("[GNSS] GNSS enabled, polling for data...");
            #endif
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[GNSS] UART: RX pin %d, TX pin %d, Baud %lu\n", cfgRx, cfgTx, cfgBaud);
            #endif
        }
        firstCheck = false;
    }

    bool hadValidData = false;
    int bytesRead = 0;

    while (uart->available())
    {
        char c = uart->read();
        bytesRead++;

        if (gps.encode(c))
        {
            hadValidData = true;
        }
        if (c == '\n')
        {
            last = line;
            line = "";
            fresh = true;

            if (config.getGNSSConfig().verboseLogging && millis() - lastDebugTime > 10000)
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("[GNSS] Received NMEA: " + last.substring(0, 40) + (last.length() > 40 ? "..." : ""));
                #endif
                lastDebugTime = millis();
            }

            routeToOutputs(last);
        }
        else
        {
            line += c;
        }
    }

    if (bytesRead == 0 && config.getGNSSConfig().verboseLogging && millis() - lastDebugTime > 15000)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] No UART data received (RX:%d, TX:%d, Baud:%lu)\n", cfgRx, cfgTx, cfgBaud);
        #endif
        int rxState = digitalRead(cfgRx);
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] RX pin %d current state: %s\n", cfgRx, rxState ? "HIGH" : "LOW");
        #endif
        static bool testedBauds = false;
        static unsigned long lastTestTime = 0;
        if ((!testedBauds || (millis() - lastTestTime > 60000)) && config.getGNSSConfig().verboseLogging)
        {
            testedBauds = true;
            lastTestTime = millis();
            #ifndef KISS_SERIAL_MODE
            Serial.println("[GNSS] Testing alternative baud rates...");
            #endif
            uint32_t testBauds[] = {9600, 4800, 38400, 115200};
            for (int i = 0; i < 3; i++)
            {
                uart->end();
                delay(50);
                uart->begin(testBauds[i], SERIAL_8N1, cfgRx, cfgTx);
                delay(200);

                int testBytes = 0;
                unsigned long start = millis();
                while (millis() - start < 500 && testBytes < 10)
                {
                    if (uart->available())
                    {
                        char c = uart->read();
                        testBytes++;
                        if (config.getGNSSConfig().verboseLogging)
                        {
                            #ifndef KISS_SERIAL_MODE
                            Serial.printf("[GNSS] Baud %lu: got char 0x%02X ('%c')\n",
                                          testBauds[i], (uint8_t)c, isprint(c) ? c : '.');
                            #endif
                        }
                    }
                }

                if (testBytes > 0)
                {
                    if (config.getGNSSConfig().verboseLogging)
                    {
                        #ifndef KISS_SERIAL_MODE
                        Serial.printf("[GNSS] Found data at baud rate %lu!\n", testBauds[i]);
                        #endif
                    }
                    cfgBaud = testBauds[i];
                    return;
                }
            }

            uart->end();
            delay(50);
            uart->begin(cfgBaud, SERIAL_8N1, cfgRx, cfgTx);
            if (config.getGNSSConfig().verboseLogging)
            {
                #ifndef KISS_SERIAL_MODE
                Serial.println("[GNSS] Restored original baud rate");
                #endif
            }
        }

        lastDebugTime = millis();
    }

    if (hadValidData)
    {
        lastValidGnssTime = millis();
        if (gps.location.isValid())
        {
            lastValidLat = gps.location.lat();
            lastValidLng = gps.location.lng();
            hasValidPosition = true;
        }
    }

    GNSSConfig &cfg = config.getGNSSConfig();
    if (cfg.synthesizeOnSilence && isGnssSilent())
    {
        synthesizeSentences();
    }
}

bool GNSSDriver::hasFreshSentence()
{
    bool f = fresh;
    fresh = false;
    return f;
}

const String &GNSSDriver::lastSentence() const
{
    return last;
}

bool GNSSDriver::locationValid() const { return gps.location.isValid(); }

double GNSSDriver::lat() const { return const_cast<TinyGPSPlus &>(gps).location.lat(); }

double GNSSDriver::lng() const { return const_cast<TinyGPSPlus &>(gps).location.lng(); }

bool GNSSDriver::speedValid() const { return gps.speed.isValid(); }

double GNSSDriver::speedKmph() const
{
    return speedValid() ? const_cast<TinyGPSPlus &>(gps).speed.kmph() : 0.0;
}

bool GNSSDriver::courseValid() const { return gps.course.isValid(); }

double GNSSDriver::courseDeg() const
{
    return courseValid() ? const_cast<TinyGPSPlus &>(gps).course.deg() : 0.0;
}

bool GNSSDriver::altitudeValid() const { return gps.altitude.isValid(); }

double GNSSDriver::altitudeMeters() const
{
    return altitudeValid() ? const_cast<TinyGPSPlus &>(gps).altitude.meters() : 0.0;
}

uint32_t GNSSDriver::satellitesInUse() const
{
    return const_cast<TinyGPSPlus &>(gps).satellites.isValid() ? const_cast<TinyGPSPlus &>(gps).satellites.value() : 0;
}

void GNSSDriver::loadConfig()
{
    GNSSConfig &cfg = config.getGNSSConfig();

    gnssEnabled = cfg.enabled;

    if (cfg.baudRate != cfgBaud)
    {
        cfgBaud = cfg.baudRate;
        if (gnssEnabled)
        {
            uart->end();
            uart->begin(cfgBaud, SERIAL_8N1, cfgRx, cfgTx);
        }
    }
}

void GNSSDriver::setEnabled(bool enabled)
{
    gnssEnabled = enabled;
    if (!enabled)
    {
        uart->end();
    }
    else
    {
        uart->begin(cfgBaud, SERIAL_8N1, cfgRx, cfgTx);
        lastValidGnssTime = millis();
    }
}

void GNSSDriver::routeToOutputs(const String &sentence)
{
    if (sentence.length() == 0)
        return;

    updateNmeaMonitor(sentence);

    GNSSConfig &cfg = config.getGNSSConfig();

    if (cfg.routeToTcp && nmeaClient && nmeaClient.connected())
    {
        nmeaClient.print(sentence);
        if (!sentence.endsWith("\r\n"))
        {
            nmeaClient.print("\r\n");
        }
    }

    if (cfg.routeToUsb)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.print(sentence);
        #endif
        if (!sentence.endsWith("\r\n"))
        {
            #ifndef KISS_SERIAL_MODE
            Serial.print("\r\n");
            #endif
        }
    }
}

bool GNSSDriver::isGnssSilent() const
{
    GNSSConfig &cfg = config.getGNSSConfig();
    return (millis() - lastValidGnssTime) > cfg.silenceTimeoutMs;
}

void GNSSDriver::synthesizeSentences()
{
    if (millis() - lastSynthesisTime < 5000)
    {
        return;
    }

    lastSynthesisTime = millis();

    if (!hasValidPosition)
    {
        return;
    }

    String gprmc = generateGPRMC();
    String gpgga = generateGPGGA();

    if (config.getGNSSConfig().verboseLogging)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS] Synthesizing NMEA sentences (GNSS silent)");
        #endif
    }

    routeToOutputs(gprmc);
    routeToOutputs(gpgga);
}

String GNSSDriver::generateGPRMC()
{
    String sentence = "$GPRMC,";

    unsigned long currentTime = millis() / 1000;
    int hours = (currentTime / 3600) % 24;
    int minutes = (currentTime / 60) % 60;
    int seconds = currentTime % 60;

    sentence += String(hours, DEC);
    if (hours < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(minutes, DEC);
    if (minutes < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(seconds, DEC);
    if (seconds < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += ".00,";

    sentence += "V,";

    double lat = abs(lastValidLat);
    int latDeg = (int)lat;
    double latMin = (lat - latDeg) * 60.0;
    sentence += String(latDeg, DEC);
    if (latDeg < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(latMin, 4) + ",";
    sentence += (lastValidLat >= 0) ? "N," : "S,";

    double lng = abs(lastValidLng);
    int lngDeg = (int)lng;
    double lngMin = (lng - lngDeg) * 60.0;
    sentence += String(lngDeg, DEC);
    if (lngDeg < 100)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    if (lngDeg < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(lngMin, 4) + ",";
    sentence += (lastValidLng >= 0) ? "E," : "W,";

    sentence += "0.0,0.0,240101,0.0,E,S";

    uint8_t checksum = calculateNmeaChecksum(sentence);
    sentence += "*";
    if (checksum < 16)
        sentence += "0";
    sentence += String(checksum, HEX);

    return sentence;
}

String GNSSDriver::generateGPGGA()
{
    String sentence = "$GPGGA,";

    unsigned long currentTime = millis() / 1000;
    int hours = (currentTime / 3600) % 24;
    int minutes = (currentTime / 60) % 60;
    int seconds = currentTime % 60;

    sentence += String(hours, DEC);
    if (hours < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(minutes, DEC);
    if (minutes < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(seconds, DEC);
    if (seconds < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += ".00,";

    double lat = abs(lastValidLat);
    int latDeg = (int)lat;
    double latMin = (lat - latDeg) * 60.0;
    sentence += String(latDeg, DEC);
    if (latDeg < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(latMin, 4) + ",";
    sentence += (lastValidLat >= 0) ? "N," : "S,";

    double lng = abs(lastValidLng);
    int lngDeg = (int)lng;
    double lngMin = (lng - lngDeg) * 60.0;
    sentence += String(lngDeg, DEC);
    if (lngDeg < 100)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    if (lngDeg < 10)
        sentence = sentence.substring(0, sentence.length() - 1) + "0" + sentence.substring(sentence.length() - 1);
    sentence += String(lngMin, 4) + ",";
    sentence += (lastValidLng >= 0) ? "E," : "W,";

    sentence += "0,00,99.9,0.0,M,0.0,M,,";

    uint8_t checksum = calculateNmeaChecksum(sentence);
    sentence += "*";
    if (checksum < 16)
        sentence += "0";
    sentence += String(checksum, HEX);

    return sentence;
}

uint8_t GNSSDriver::calculateNmeaChecksum(const String &sentence)
{
    uint8_t checksum = 0;

    for (int i = 1; i < sentence.length(); i++)
    {
        if (sentence[i] == '*')
            break;
        checksum ^= sentence[i];
    }

    return checksum;
}

void GNSSDriver::testGnssModule()
{
    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] Testing GNSS module communication...");
    #endif
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[GNSS] Test 1: Monitoring RX pin %d for 3 seconds...\n", cfgRx);
    #endif
    int transitions = 0;
    int lastState = digitalRead(cfgRx);
    unsigned long start = millis();

    while (millis() - start < 3000)
    {
        int currentState = digitalRead(cfgRx);
        if (currentState != lastState)
        {
            transitions++;
            lastState = currentState;
        }
        delay(1);
    }

    #ifndef KISS_SERIAL_MODE
    Serial.printf("[GNSS] RX pin transitions in 3s: %d %s\n",
                  transitions, transitions > 0 ? "(ACTIVITY DETECTED)" : "(NO ACTIVITY)");
    #endif

    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] Test 2: Sending wake-up commands...");
    #endif
    uart->println("$PMTK000*32");
    delay(100);
    uart->println("$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
    delay(100);
    uart->println("$PMTK220,1000*1F");
    delay(500);

    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] Test 3: Checking for module response...");
    #endif
    int responseBytes = 0;
    start = millis();

    while (millis() - start < 2000 && responseBytes < 50)
    {
        if (uart->available())
        {
            char c = uart->read();
            responseBytes++;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[GNSS] Response byte %d: 0x%02X ('%c')\n",
                          responseBytes, (uint8_t)c, isprint(c) ? c : '.');
            #endif
        }
    }

    if (responseBytes > 0)
    {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] Module responded with %d bytes!\n", responseBytes);
        #endif
    }
    else
    {

        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS] Test 4: Hardware diagnostics...");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] UART pins - RX:%d TX:%d\n", cfgRx, cfgTx);
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[GNSS] Current RX state: %s\n", digitalRead(cfgRx) ? "HIGH" : "LOW");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS] Troubleshooting suggestions:");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS]   - Verify GNSS module is powered");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS]   - Check antenna connection");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS]   - Try different baud rates (4800, 38400)");
        #endif
        #ifndef KISS_SERIAL_MODE
        Serial.println("[GNSS]   - Verify correct board variant (V3 vs V4)");
        #endif
    }
}
