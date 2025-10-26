#pragma once
#include <Arduino.h>

// Display timing constants
constexpr unsigned long SCREEN_TIMEOUT_MS = 30000;
constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 2000;
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 30000;

// Button constants
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
constexpr unsigned long BUTTON_LONG_PRESS_MS = 2000;
constexpr unsigned long BUTTON_DOUBLE_CLICK_MS = 400;
constexpr unsigned long BUTTON_PROGRESS_UPDATE_MS = 100;

// Menu system constants
constexpr unsigned long MENU_TIMEOUT_MS = 2000;
constexpr size_t MENU_BUFFER_SIZE = 4;

// Buffer sizes
constexpr size_t KISS_RX_BUFFER_SIZE = 4096;
constexpr size_t KISS_TX_BUFFER_SIZE = 4096;
constexpr size_t MAX_DISPLAY_STRING_LEN = 32;

// Network constants
constexpr uint16_t TCP_KISS_PORT = 8001;
constexpr uint16_t TCP_NMEA_PORT = 10110;
constexpr uint16_t WEB_SERVER_PORT = 80;
constexpr uint16_t DNS_SERVER_PORT = 53;
constexpr uint16_t OTA_PORT = 3232;

// Timing intervals
constexpr unsigned long PPS_CHECK_INTERVAL_MS = 5000;
constexpr unsigned long RX_TIMEOUT_MS = 1000;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr unsigned long SERIAL_TIMEOUT_MS = 3000;
constexpr unsigned long OTA_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000; // 24 hours

// Display text constants (declarations only)
extern const char DISPLAY_LORAX[];
extern const char DISPLAY_APRS_TRACKER[];
extern const char DISPLAY_INITIALIZING[];
extern const char DISPLAY_CONNECTING[];
extern const char DISPLAY_AP_MODE[];
extern const char DISPLAY_CONNECTED[];
extern const char DISPLAY_BATTERY[];
extern const char DISPLAY_NO_BATTERY[];
extern const char DISPLAY_CHARGING[];
extern const char DISPLAY_VOLTAGE[];
extern const char DISPLAY_IP[];
extern const char DISPLAY_BEACONS[];
extern const char DISPLAY_KISS_STATS[];
extern const char DISPLAY_RADIO[];
extern const char DISPLAY_FREQUENCY[];
extern const char DISPLAY_MHZ[];
extern const char DISPLAY_POWER[];
extern const char DISPLAY_DBM[];
extern const char DISPLAY_SF[];
extern const char DISPLAY_BW[];
extern const char DISPLAY_KHZ[];
extern const char DISPLAY_LAST[];
extern const char DISPLAY_SECONDS_AGO[];
extern const char DISPLAY_IDLE[];
extern const char DISPLAY_GNSS[];
extern const char DISPLAY_STATUS_ENABLED[];
extern const char DISPLAY_STATUS_DISABLED[];
extern const char DISPLAY_BAUD[];
extern const char DISPLAY_TCP[];
extern const char DISPLAY_USB[];
extern const char DISPLAY_YES[];
extern const char DISPLAY_NO[];
extern const char DISPLAY_NMEA[];
extern const char DISPLAY_NO_RECENT_NMEA[];
extern const char DISPLAY_NOT_COMPILED[];
extern const char DISPLAY_DISABLED[];
extern const char DISPLAY_BUILD_CONFIG[];
extern const char DISPLAY_APRS[];
extern const char DISPLAY_CALL[];
extern const char DISPLAY_NO_GPS_FIX[];
extern const char DISPLAY_WAITING_SATELLITES[];
extern const char DISPLAY_NEXT[];
extern const char DISPLAY_BEACON_DUE[];
extern const char DISPLAY_MODE_KISS[];
extern const char DISPLAY_USE_CONFIG[];
extern const char DISPLAY_ENABLE_APRS[];
extern const char DISPLAY_TRACKER_MODE[];
extern const char DISPLAY_NETWORK[];
extern const char DISPLAY_KISS_PORT[];
extern const char DISPLAY_NMEA_PORT[];
extern const char DISPLAY_CONNECTED_CLIENT[];
extern const char DISPLAY_NO_CLIENT[];
extern const char DISPLAY_AP_CLIENTS[];
extern const char DISPLAY_RSSI[];
extern const char DISPLAY_WEB_STATUS[];
extern const char DISPLAY_STATISTICS[];
extern const char DISPLAY_KISS_RX[];
extern const char DISPLAY_KISS_TX[];
extern const char DISPLAY_RADIO_RX[];
extern const char DISPLAY_RADIO_TX[];
extern const char DISPLAY_BATTERY_VOLTAGE[];
extern const char DISPLAY_SOC[];
extern const char DISPLAY_PERCENT[];
extern const char DISPLAY_STATUS[];
extern const char DISPLAY_ADC[];
extern const char DISPLAY_NOT_DETECTED[];
extern const char DISPLAY_UPTIME[];
extern const char DISPLAY_MINUTES[];
extern const char DISPLAY_HEAP[];
extern const char DISPLAY_KB[];
extern const char DISPLAY_POWER_OFF_MSG[];

// Screen navigation strings
extern const char DISPLAY_NEXT_ARROW[];
extern const char DISPLAY_POWER_OFF[];
extern const char DISPLAY_DISABLED_MSG[];
extern const char DISPLAY_SERIAL_CONNECTED[];
extern const char DISPLAY_POWERING_OFF[];
extern const char DISPLAY_GOODBYE[];
extern const char DISPLAY_RELEASE_CANCEL[];

// Debug and system info strings
extern const char DISPLAY_KMPH[];
extern const char DISPLAY_SATS[];

// OTA update strings
extern const char DISPLAY_OTA_STATUS[];
extern const char DISPLAY_OTA_READY[];
extern const char DISPLAY_OTA_CHECKING[];
extern const char DISPLAY_OTA_DOWNLOADING[];
extern const char DISPLAY_OTA_INSTALLING[];
extern const char DISPLAY_OTA_SUCCESS[];
extern const char DISPLAY_OTA_FAILED[];
extern const char DISPLAY_OTA_AVAILABLE[];
extern const char DISPLAY_OTA_PROGRESS[];
extern const char DISPLAY_UPDATE_VER[];

// Screen numbers for display indicator
constexpr uint8_t SCREEN_COUNT = 8; // Increased for OTA screen

// Note: FPSTR macro is already defined by Arduino framework