#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <functional>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <vector>

#include "HardwareConfig.h"

/**
 * @brief High-level manager for the Heltec V4 GNSS accessory module.
 *
 * The GNSS manager provides a unified interface for powering the receiver,
 * parsing NMEA telemetry, synchronising the system clock, and exposing the
 * PPS (pulse-per-second) timing reference for downstream applications.
 */
class GNSSManager
{
public:
    struct FixData
    {
        bool valid = false;          ///< True when the receiver currently reports a fix
        bool active = false;         ///< RMC status flag (A = active fix)
        bool is3DFix = false;        ///< True when the fix quality is 3D (quality >= 2)
        double latitude = NAN;       ///< Decimal degrees latitude (positive = North)
        double longitude = NAN;      ///< Decimal degrees longitude (positive = East)
        double altitudeMeters = NAN; ///< Altitude above mean sea level in metres
        float speedKnots = 0.0f;     ///< Ground speed in knots
        float courseDegrees = 0.0f;  ///< Course over ground in degrees
        float hdop = 0.0f;           ///< Horizontal dilution of precision
        uint8_t satellites = 0;      ///< Satellites used in solution
        time_t timestamp = 0;        ///< UTC timestamp for the fix (if available)
        unsigned long lastUpdateMillis = 0; ///< millis() when the fix was last refreshed
    };

    struct TimeStatus
    {
        bool valid = false;                 ///< True if a recent sentence provided date+time
        bool synced = false;                ///< True if the system clock has been synchronised
        time_t epoch = 0;                   ///< UTC epoch seconds from the last fix
        unsigned long lastUpdateMillis = 0; ///< millis() when time was last refreshed
        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day = 0;
        uint8_t hour = 0;
        uint8_t minute = 0;
        uint8_t second = 0;
    };

    struct PPSStatus
    {
        bool enabled = false;              ///< True when the PPS interrupt is armed
        bool available = false;            ///< True when pulses are being received
        uint32_t pulseCount = 0;           ///< Total pulses observed since start
        unsigned long lastPulseMillis = 0; ///< millis() timestamp of the last pulse
        unsigned long lastPulseMicros = 0; ///< micros() timestamp of the last pulse
    };

    using PPSCallback = std::function<void(uint32_t pulseCount)>;
    using NMEACallback = std::function<void(const String &line)>;

    explicit GNSSManager(HardwareSerial &serialPort = Serial1);

    /**
     * @brief Assign a different serial port after construction.
     */
    void setSerialPort(HardwareSerial &serialPort);

    /**
     * @brief Initialise the GNSS receiver interface.
     * @return true if the receiver was powered and the serial link configured.
     */
    bool begin(bool enablePower = true);

    /**
     * @brief Shut down the GNSS receiver and release hardware resources.
     */
    void end();

    /**
     * @brief Periodic processing for the GNSS manager.
     */
    void update();

    /**
     * @brief Enable or disable power to the GNSS module.
     */
    void setPowerEnabled(bool enable);

    /**
     * @brief Query whether the GNSS hardware is currently powered.
     */
    bool isPowerEnabled() const { return powerEnabled; }

    /**
     * @brief Determine if the GNSS manager has been initialised.
     */
    bool isInitialised() const { return initialised; }

    /**
     * @brief Register a callback that fires whenever a PPS pulse is observed.
     */
    void setPPSCallback(PPSCallback callback);

    /**
     * @brief Register a callback that receives validated NMEA sentences.
     */
    void setNMEACallback(NMEACallback callback);

    /**
     * @brief Access the latest fix information.
     */
    const FixData &getFixData() const { return fixData; }

    /**
     * @brief Access the latest time status information.
     */
    const TimeStatus &getTimeStatus() const { return timeStatus; }

    /**
     * @brief Access the current PPS status snapshot.
     */
    PPSStatus getPPSStatus() const;

    /**
     * @brief Attempt to synchronise the system clock with the last valid fix.
     * @return true on success.
     */
    bool syncSystemTime();

private:
    HardwareSerial *serial;
    bool initialised;
    bool powerEnabled;

    FixData fixData;
    TimeStatus timeStatus;
    PPSStatus ppsStatus;

    String nmeaBuffer;

    PPSCallback ppsCallback;
    NMEACallback nmeaCallback;

    volatile uint32_t ppsPulseCount;
    volatile unsigned long ppsLastMicros;
    volatile bool ppsInterruptFlag;
    uint32_t ppsHandledCount;

    static GNSSManager *activeInstance;

    static void IRAM_ATTR handlePPSInterrupt();

    void processPPS();
    void handleNMEALine(const String &line);
    void parseRMC(const std::vector<String> &tokens);
    void parseGGA(const std::vector<String> &tokens);
    bool validateChecksum(const String &line) const;

    static double parseCoordinate(const String &value, const String &direction);
    static bool parseTimeToken(const String &token, uint8_t &hour, uint8_t &minute, uint8_t &second);
    static bool parseDateToken(const String &token, uint8_t &day, uint8_t &month, uint16_t &year);
    static time_t buildEpoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
};
