#include <Arduino.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "KISS.h"
#include "Radio.h"
#include "GNSS.h"
#include "Config.h"
#include "Battery.h"
#include "APRS.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "Constants.h"
#include "DisplayUtils.h"
#include "DisplayManager.h"
#include "ErrorHandler.h"
#include "OTA.h"
#include "WebSocketServer.h"
#include <ESPAsyncWebServer.h>

#ifndef WIFI_SSID
#define WIFI_SSID "LoRaTNCX"
#define WIFI_PASS "tncpass123"
#endif

#define VEXT_PIN 36
#define OLED_RST 21
#define OLED_SDA 17
#define OLED_SCL 18
#define USER_BTN 0

#define GNSS_RX 39
#define GNSS_TX 38
#define VEXT_CTRL 37
#define VGNSS_CTRL 34
#define GNSS_WAKE 40
#define GNSS_PPS 41
#define GNSS_RST 42

// Use constants from Constants.h instead

WiFiServer kissServer(TCP_KISS_PORT);
WiFiServer nmeaServer(TCP_NMEA_PORT);
WiFiClient kissClient;
WiFiClient nmeaClient;

// Web server for the interface
AsyncWebServer webServer(WEB_SERVER_PORT);

static unsigned long lastPpsCheck = 0;
static int lastPpsState = -1;
static unsigned long ppsToggleCount = 0;

SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL, GEOMETRY_128_64);
bool displayAvailable = false;

enum DisplayScreen
{
  SCREEN_STATUS = 0,
  SCREEN_RADIO,
  SCREEN_GNSS,
  SCREEN_APRS,
  SCREEN_NETWORK,
  SCREEN_STATS,
  SCREEN_OTA,
  SCREEN_POWER
};

struct ButtonState
{
  bool lastState = HIGH;
  unsigned long lastPressTime = 0;
  unsigned long lastReleaseTime = 0;
  bool longPressHandled = false;
  // Use constants from Constants.h
};

DisplayScreen currentScreen = SCREEN_STATUS;
ButtonState buttonState;
unsigned long lastScreenChange = 0;

KISS kiss(Serial, 4096, 4096);
RadioHAL radio;
GNSSDriver gnss;

void runRadioHealthCheckImpl()
{
  radio.testRadioHealth();
}

void runHardwarePinCheckImpl()
{
  radio.checkHardwarePins();
}

void runTransmissionTestImpl()
{
  radio.runTransmissionTest();
}

void runContinuousTransmissionTestImpl()
{
  radio.testContinuousTransmission();
}

void updateDisplay();
void updateStatusScreen();
void updateRadioScreen();
void updateGnssScreen();
void updateAprsScreen();
void updateNetworkScreen();
void updateStatsScreen();
void updateOtaScreen();
void updatePowerScreen();
void setupButton();
void handleButton();
void handleButtonShortPress();
void handleButtonLongPress();
void showLongPressProgress(float progress);
bool reinitializeDisplay();
bool reinitializeGNSS();
void shutdownGNSS();

struct MonitorData
{
  String lastKissFrame = "";
  String lastNmeaSentence = "";
  String lastRadioDebug = "";
  unsigned long lastKissTime = 0;
  unsigned long lastNmeaTime = 0;
  unsigned long lastRadioTime = 0;
  int kissFrameCount = 0;
  int nmeaCount = 0;
  int radioTxCount = 0;
  int radioRxCount = 0;
};
static MonitorData monitor;

void updateNmeaMonitor(const String &sentence)
{
  monitor.lastNmeaSentence = sentence;
  monitor.lastNmeaTime = millis();
  monitor.nmeaCount++;
  
  // Mark GNSS-related screens dirty
  displayManager.markDirty(DisplayManager::GNSS_CHANGED);
  displayManager.recordActivity(SCREEN_GNSS);
  displayManager.recordActivity(SCREEN_APRS);
}

void onKissFrameRx(const uint8_t *frame, size_t len)
{
  monitor.lastKissFrame = "TX Frame: " + String(len) + " bytes";
  for (size_t i = 0; i < min(len, (size_t)16); i++)
  {
    monitor.lastKissFrame += " " + String(frame[i], 16);
  }
  if (len > 16)
    monitor.lastKissFrame += "...";
  monitor.lastKissTime = millis();
  monitor.kissFrameCount++;

#ifndef KISS_SERIAL_MODE
  Serial.printf("[RADIO] Attempting TX: %d bytes\n", len);

  Serial.print("[RADIO] Data: ");
  for (size_t i = 0; i < min(len, (size_t)16); i++)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.printf("%02X ", frame[i]);
    #endif
  }
  if (len > 16)
    #ifndef KISS_SERIAL_MODE
    Serial.print("...");
  Serial.println();
    #endif
#endif

  bool txResult = radio.send(frame, len);
  monitor.lastRadioDebug = "TX " + String(len) + " bytes: " + (txResult ? "SUCCESS" : "FAILED");
  monitor.lastRadioTime = millis();
  if (txResult)
  {
    monitor.radioTxCount++;
  }

  if (!txResult)
  {
#ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] TX FAILED for frame of %d bytes\n", len);
    Serial.println("[RADIO] Possible causes: Radio not initialized, invalid config, or hardware issue");
#endif
  }
  else
  {
#ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] TX SUCCESS: %d bytes transmitted\n", len);
#endif
  }
}

void onKissCommandRx(uint8_t cmd, const uint8_t *data, size_t len)
{
  switch (cmd)
  {
  case KISS::TX_DELAY:
    if (len >= 1)
    {
      radio.setTxDelay(data[0]);
      config.getRadioConfig().txDelay = data[0];
#ifndef KISS_SERIAL_MODE
      Serial.printf("[KISS] TX_DELAY set to %d (x10ms)\n", data[0]);
#endif
    }
    break;

  case KISS::PERSISTENCE:
    if (len >= 1)
    {
      radio.setPersist(data[0]);
      config.getRadioConfig().persist = data[0];
#ifndef KISS_SERIAL_MODE
      Serial.printf("[KISS] PERSISTENCE set to %d\n", data[0]);
#endif
    }
    break;

  case KISS::SLOT_TIME:
    if (len >= 1)
    {
      radio.setSlotTime(data[0]);
      config.getRadioConfig().slotTime = data[0];
      #ifndef KISS_SERIAL_MODE
      Serial.printf("[KISS] SLOT_TIME set to %d (x10ms)\n", data[0]);
      #endif
    }
    break;

  case KISS::SET_HARDWARE:
    if (len >= 1)
    {
      uint8_t hw_cmd = data[0];
      switch (hw_cmd)
      {
      case 0x01:
        if (len >= 5)
        {
          float freq;
          memcpy(&freq, &data[1], 4);
          if (radio.setFrequency(freq))
          {
            config.getRadioConfig().frequency = freq;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Frequency set to %.3f MHz\n", freq);
            #endif
          }
          else
          {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Failed to set frequency %.3f MHz\n", freq);
            #endif
          }
        }
        break;

      case 0x02:
        if (len >= 2)
        {
          int8_t power = (int8_t)data[1];
          if (radio.setTxPower(power))
          {
            config.getRadioConfig().txPower = power;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] TX power set to %d dBm\n", power);
            #endif
          }
          else
          {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Failed to set TX power %d dBm\n", power);
            #endif
          }
        }
        break;

      case 0x03:
        if (len >= 5)
        {
          float bw;
          memcpy(&bw, &data[1], 4);
          if (radio.setBandwidth(bw))
          {
            config.getRadioConfig().bandwidth = bw;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Bandwidth set to %.1f kHz\n", bw);
            #endif
          }
          else
          {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Failed to set bandwidth %.1f kHz\n", bw);
            #endif
          }
        }
        break;

      case 0x04:
        if (len >= 2)
        {
          uint8_t sf = data[1];
          if (radio.setSpreadingFactor(sf))
          {
            config.getRadioConfig().spreadingFactor = sf;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Spreading factor set to %d\n", sf);
            #endif
          }
          else
          {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Failed to set spreading factor %d\n", sf);
            #endif
          }
        }
        break;

      case 0x05:
        if (len >= 2)
        {
          uint8_t cr = data[1];
          if (radio.setCodingRate(cr))
          {
            config.getRadioConfig().codingRate = cr;
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Coding rate set to 4/%d\n", cr);
            #endif
          }
          else
          {
            #ifndef KISS_SERIAL_MODE
            Serial.printf("[KISS] Failed to set coding rate 4/%d\n", cr);
            #endif
          }
        }
        break;

      case 0x10:
      {
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[KISS] Current radio config:\n");
        #endif
        Serial.printf("  Frequency: %.3f MHz\n", radio.getFrequency());
        #ifndef KISS_SERIAL_MODE
        Serial.printf("  TX Power: %d dBm\n", radio.getTxPower());
        Serial.printf("  Bandwidth: %.1f kHz\n", radio.getBandwidth());
        Serial.printf("  Spreading Factor: %d\n", radio.getSpreadingFactor());
        Serial.printf("  Coding Rate: 4/%d\n", radio.getCodingRate());
        Serial.printf("  TX Delay: %d x 10ms\n", radio.getTxDelay());
        Serial.printf("  Persistence: %d\n", radio.getPersist());
        Serial.printf("  Slot Time: %d x 10ms\n", radio.getSlotTime());
        #endif
      }
      break;

      default:
        #ifndef KISS_SERIAL_MODE
        Serial.printf("[KISS] Unknown hardware command: 0x%02X\n", hw_cmd);
        #endif
        break;
      }
    }
    break;

  default:
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[KISS] Unknown command: 0x%02X\n", cmd);
    #endif
    break;
  }
}

void onRadioFrameRx(const uint8_t *frame, size_t len, int16_t rssi, float snr)
{
  monitor.lastRadioDebug = "RX " + String(len) + " bytes, RSSI: " + String(rssi) + " dBm, SNR: " + String(snr, 1) + " dB";
  monitor.lastRadioTime = millis();
  monitor.radioRxCount++;
  
#ifndef KISS_SERIAL_MODE
  Serial.printf("[RADIO] RX: %d bytes, RSSI: %d dBm, SNR: %.1f dB\n", len, rssi, snr);
#endif

  // Mark relevant screens dirty due to radio activity
  displayManager.markDirty(DisplayManager::RADIO_CHANGED | DisplayManager::STATS_CHANGED);
  displayManager.recordActivity(SCREEN_RADIO);
  displayManager.recordActivity(SCREEN_STATS);

  kiss.writeFrame(frame, len);
  if (kissClient.connected())
  {
    kiss.writeFrameTo(kissClient, frame, len);
  }
}

void updateDisplay()
{
  if (!displayAvailable)
    return;

  // Check if screen should timeout to status
  if (currentScreen != SCREEN_STATUS && millis() - lastScreenChange > SCREEN_TIMEOUT_MS)
  {
    currentScreen = SCREEN_STATUS;
    displayManager.markDirty(DisplayManager::SCREEN_CHANGED);
  }

  // Check if current screen needs updating
  unsigned long refreshInterval = displayManager.getRefreshInterval(currentScreen);
  if (!displayManager.needsUpdate(currentScreen, refreshInterval)) {
    return;  // Skip update if not needed
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  displayUtils.drawPageIndicator(currentScreen, SCREEN_COUNT);

  switch (currentScreen)
  {
  case SCREEN_STATUS:
    updateStatusScreen();
    break;
  case SCREEN_RADIO:
    updateRadioScreen();
    break;
  case SCREEN_GNSS:
#if GNSS_ENABLE
    updateGnssScreen();
#else
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "GNSS");
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 18, "Not available");
    display.drawString(0, 28, "GNSS disabled");
    display.drawString(0, 38, "in build config");
#endif
    break;
  case SCREEN_APRS:
#if GNSS_ENABLE
    updateAprsScreen();
#else
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "APRS");
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 18, "Not available");
    display.drawString(0, 28, "Requires GNSS");
    display.drawString(0, 38, "for positioning");
#endif
    break;
  case SCREEN_NETWORK:
    updateNetworkScreen();
    break;
  case SCREEN_STATS:
    updateStatsScreen();
    break;
  case SCREEN_OTA:
    updateOtaScreen();
    break;
  case SCREEN_POWER:
    updatePowerScreen();
    break;
  }

  display.display();
  displayManager.updated(currentScreen);
}

void updateStatusScreen()
{
  display.setFont(ArialMT_Plain_16);
  const auto &aprsConfig = config.getAPRSConfig();
  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    display.drawString(0, 0, FPSTR(DISPLAY_APRS_TRACKER));
  }
  else
  {
    display.drawString(0, 0, FPSTR(DISPLAY_LORAX));
  }

  display.setFont(ArialMT_Plain_10);

  // Battery status - optimized with utility function
  const auto &batteryStatus = battery.getStatus();
  if (batteryStatus.isConnected)
  {
    displayUtils.drawBatteryStatus(18, batteryStatus.voltage, batteryStatus.stateOfCharge, 
                                   batteryStatus.state == BatteryMonitor::CHARGING);
  }
  else
  {
    display.drawString(0, 18, FPSTR(DISPLAY_NO_BATTERY));
  }

  // WiFi status - optimized string operations
  if (WiFi.status() == WL_CONNECTED)
  {
    display.drawString(0, 28, FPSTR(DISPLAY_CONNECTED));
    displayUtils.drawPrefixedString(0, 38, FPSTR(DISPLAY_IP), WiFi.localIP().toString().c_str());
  }
  else if (WiFi.getMode() == WIFI_AP)
  {
    display.drawString(0, 28, FPSTR(DISPLAY_AP_MODE));
    displayUtils.drawPrefixedString(0, 38, FPSTR(DISPLAY_IP), WiFi.softAPIP().toString().c_str());
  }
  else
  {
    display.drawString(0, 28, FPSTR(DISPLAY_CONNECTING));
  }

  // Statistics - optimized display
  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    const auto &aprsStats = aprs.getStats();
    displayUtils.drawNumberWithUnit(0, 48, FPSTR(DISPLAY_BEACONS), aprsStats.beaconsSent, F(""));
  }
  else
  {
    const auto &stats = kiss.getStats();
    displayUtils.drawFormatted(0, 48, F("%S%lu TX:%lu"), FPSTR(DISPLAY_KISS_STATS), stats.framesRx, stats.framesTx);
  }
}

void updateRadioScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, FPSTR(DISPLAY_RADIO));

  display.setFont(ArialMT_Plain_10);
  const auto &cfg = config.getRadioConfig();

  // Use optimized radio config display
  displayUtils.drawRadioConfig(18, cfg.frequency, cfg.txPower, cfg.spreadingFactor, cfg.bandwidth);

  // Activity status
  displayUtils.drawTimeAgo(0, 48, FPSTR(DISPLAY_LAST), monitor.lastRadioTime);
}

void updateGnssScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "GNSS");

  display.setFont(ArialMT_Plain_10);

#if GNSS_ENABLE
  const auto &gnssCfg = config.getGNSSConfig();

  display.drawString(0, 18, gnssCfg.enabled ? "Status: Enabled" : "Status: Disabled");

  if (gnssCfg.enabled)
  {
    display.drawString(0, 28, String("Baud: ") + String(gnssCfg.baudRate));
    display.drawString(0, 38, String("TCP: ") + (gnssCfg.routeToTcp ? "Yes" : "No") + " USB: " + (gnssCfg.routeToUsb ? "Yes" : "No"));

    unsigned long lastNmea = (millis() - monitor.lastNmeaTime) / 1000;
    if (lastNmea < 60)
    {
      display.drawString(0, 48, "NMEA: " + String(lastNmea) + "s ago");
    }
    else
    {
      display.drawString(0, 48, "No recent NMEA");
    }
  }
  else
  {
    display.drawString(0, 28, "Use config menu");
    display.drawString(0, 38, "to enable GNSS");
  }
#else
  display.drawString(0, 18, "Not compiled");
  display.drawString(0, 28, "GNSS disabled");
  display.drawString(0, 38, "in build config");
#endif
}

void updateAprsScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "APRS");

  display.setFont(ArialMT_Plain_10);

  const auto &aprsConfig = config.getAPRSConfig();

  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    display.drawString(0, 18, String("Call: ") + aprsConfig.callsign + "-" + String(aprsConfig.ssid));

    if (gnss.locationValid())
    {
      String coords = String(gnss.lat(), 4) + "," + String(gnss.lng(), 4);
      if (coords.length() > 18)
        coords = coords.substring(0, 18) + "...";
      display.drawString(0, 28, coords);

      String speedSats = "";
      if (gnss.speedValid())
      {
        speedSats += String(gnss.speedKmph(), 0) + "km/h ";
      }
      if (gnss.satellitesInUse() > 0)
      {
        speedSats += String(gnss.satellitesInUse()) + "sats";
      }
      display.drawString(0, 38, speedSats);
    }
    else
    {
      display.drawString(0, 28, "No GPS fix");
      display.drawString(0, 38, "Waiting for satellites");
    }

    const auto &aprsStats = aprs.getStats();
    unsigned long nextBeacon = aprs.getNextBeaconTime();
    if (nextBeacon > millis())
    {
      unsigned long timeToNext = (nextBeacon - millis()) / 1000;
      display.drawString(0, 48, "Next: " + String(timeToNext) + "s (#" + String(aprsStats.beaconsSent) + ")");
    }
    else
    {
      display.drawString(0, 48, "Beacon due (#" + String(aprsStats.beaconsSent) + ")");
    }
  }
  else
  {
    display.drawString(0, 18, "Mode: KISS TNC");
    display.drawString(0, 28, "Use config menu");
    display.drawString(0, 38, "to enable APRS");
    display.drawString(0, 48, "Tracker mode");
  }
}

void updateNetworkScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Network");

  display.setFont(ArialMT_Plain_10);

  String kissConn = kissClient.connected() ? "Connected" : "None";
  String nmeaConn = nmeaClient.connected() ? "Connected" : "None";

  display.drawString(0, 18, "KISS (8001): " + kissConn);
  display.drawString(0, 28, "NMEA (10110): " + nmeaConn);

  if (WiFi.getMode() == WIFI_AP)
  {
    display.drawString(0, 38, String("AP Clients: ") + String(WiFi.softAPgetStationNum()));
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    display.drawString(0, 38, String("RSSI: ") + String(WiFi.RSSI()) + " dBm");
  }

  display.drawString(0, 48, "TCP services active");
}

void updateStatsScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, FPSTR(DISPLAY_STATISTICS));

  display.setFont(ArialMT_Plain_10);
  const auto &stats = kiss.getStats();

  displayUtils.drawNumberWithUnit(0, 18, FPSTR(DISPLAY_KISS_RX), stats.framesRx, F(""));
  displayUtils.drawNumberWithUnit(0, 28, FPSTR(DISPLAY_KISS_TX), stats.framesTx, F(""));
  displayUtils.drawNumberWithUnit(0, 38, FPSTR(DISPLAY_RADIO_RX), monitor.radioRxCount, F(""));
  displayUtils.drawNumberWithUnit(0, 48, FPSTR(DISPLAY_RADIO_TX), monitor.radioTxCount, F(""));
}

void updateOtaScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, FPSTR(DISPLAY_OTA_STATUS));

  display.setFont(ArialMT_Plain_10);
  
  // Current version
  displayUtils.drawPrefixedString(0, 18, FPSTR(DISPLAY_UPDATE_VER), OTA.getCurrentVersion().c_str());
  
  // OTA Status
  auto status = OTA.getStatus();
  const __FlashStringHelper* statusText;
  
  switch (status) {
    case OTAManager::UpdateStatus::IDLE:
      statusText = FPSTR(DISPLAY_OTA_READY);
      break;
    case OTAManager::UpdateStatus::CHECKING:
      statusText = FPSTR(DISPLAY_OTA_CHECKING);
      break;
    case OTAManager::UpdateStatus::DOWNLOADING:
      statusText = FPSTR(DISPLAY_OTA_DOWNLOADING);
      break;
    case OTAManager::UpdateStatus::INSTALLING:
      statusText = FPSTR(DISPLAY_OTA_INSTALLING);
      break;
    case OTAManager::UpdateStatus::SUCCESS:
      statusText = FPSTR(DISPLAY_OTA_SUCCESS);
      break;
    case OTAManager::UpdateStatus::FAILED:
      statusText = FPSTR(DISPLAY_OTA_FAILED);
      break;
    default:
      statusText = FPSTR(DISPLAY_OTA_READY);
  }
  
  displayUtils.drawPrefixedString(0, 28, FPSTR(DISPLAY_STATUS), (const char*)statusText);
  
  // Progress bar for downloads/installs
  if (status == OTAManager::UpdateStatus::DOWNLOADING || 
      status == OTAManager::UpdateStatus::INSTALLING) {
    const auto& progress = OTA.getProgress();
    
    // Progress percentage
    displayUtils.drawNumberWithUnit(0, 38, FPSTR(DISPLAY_OTA_PROGRESS), 
                                    progress.percentage, FPSTR(DISPLAY_PERCENT));
    
    // Progress bar
    int barWidth = 120;
    int barHeight = 4;
    int barX = 4;
    int barY = 50;
    
    display.drawRect(barX, barY, barWidth, barHeight);
    int fillWidth = (barWidth - 2) * progress.percentage / 100;
    if (fillWidth > 0) {
      display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2);
    }
  } else if (OTA.isUpdateAvailable()) {
    // Show available update info
    const auto& updateInfo = OTA.getAvailableUpdate();
    display.drawString(0, 38, FPSTR(DISPLAY_OTA_AVAILABLE));
    displayUtils.drawPrefixedString(0, 48, FPSTR(DISPLAY_UPDATE_VER), updateInfo.version.c_str());
  } else {
    // Show free space
    uint32_t freeSpace = OTA.getFreeSketchSpace() / 1024;
    displayUtils.drawNumberWithUnit(0, 38, F("Free: "), freeSpace, FPSTR(DISPLAY_KB));
    display.drawString(0, 48, F("OTA via network"));
  }
}

void updatePowerScreen()
{
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Power");

  display.setFont(ArialMT_Plain_10);

  const auto &batteryStatus = battery.getStatus();
  if (batteryStatus.isConnected)
  {
    display.drawString(0, 18, String("Battery: ") + String(batteryStatus.voltage, 2) + "V");
    display.drawString(0, 28, String("SoC: ") + String(batteryStatus.stateOfCharge) + "%");

    String chargingStatus = battery.getBatteryStateString();
    display.drawString(0, 38, String("Status: ") + chargingStatus);

    display.drawString(0, 48, String("ADC: ") + String(batteryStatus.rawADC));
  }
  else
  {
    display.drawString(0, 18, "Battery: Not detected");
    display.drawString(0, 28, String("Uptime: ") + String(millis() / 60000) + "m");
    display.drawString(0, 38, String("Heap: ") + String(ESP.getFreeHeap() / 1024) + "KB");

    bool serialConnected = Serial && (Serial.availableForWrite() > 0);
    if (!serialConnected)
    {
      display.drawString(0, 48, "Hold btn 2s: power off");
    }
  }
}

void setupButton()
{
  pinMode(USER_BTN, INPUT_PULLUP);
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[BTN] User button initialized on GPIO %d\n", USER_BTN);
  #endif
}

void handleButton()
{
  bool currentState = digitalRead(USER_BTN);
  unsigned long now = millis();

  if (currentState != buttonState.lastState)
  {
    if (now - buttonState.lastPressTime > BUTTON_DEBOUNCE_MS)
    {

      if (currentState == LOW && buttonState.lastState == HIGH)
      {
        buttonState.lastPressTime = now;
        buttonState.longPressHandled = false;
        #ifndef KISS_SERIAL_MODE
        Serial.println("[BTN] Button pressed");
        #endif
      }
      else if (currentState == HIGH && buttonState.lastState == LOW)
      {
        buttonState.lastReleaseTime = now;
        unsigned long pressDuration = now - buttonState.lastPressTime;

        #ifndef KISS_SERIAL_MODE
        Serial.printf("[BTN] Button released after %lums\n", pressDuration);
        #endif

        if (!buttonState.longPressHandled)
        {
          if (pressDuration < BUTTON_LONG_PRESS_MS)
          {

            handleButtonShortPress();
          }
        }
      }
    }
    buttonState.lastState = currentState;
  }

  if (currentState == LOW && !buttonState.longPressHandled)
  {
    unsigned long pressDuration = now - buttonState.lastPressTime;

    if (pressDuration > 500 && displayAvailable)
    {
      float progress = (float)pressDuration / BUTTON_LONG_PRESS_MS;
      if (progress <= 1.0)
      {
        showLongPressProgress(progress);
      }
    }

    if (pressDuration >= BUTTON_LONG_PRESS_MS)
    {
      handleButtonLongPress();
      buttonState.longPressHandled = true;
    }
  }
}

void handleButtonShortPress()
{
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[BTN] Short press - changing screen from %d", currentScreen);
  #endif

  if (displayAvailable)
  {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(35, 20, FPSTR(DISPLAY_NEXT_ARROW));
    display.display();
    delay(150);
  }

  int nextScreen = (currentScreen + 1) % SCREEN_COUNT;

#if !GNSS_ENABLE
  while (nextScreen == SCREEN_GNSS || nextScreen == SCREEN_APRS)
  {
    nextScreen = (nextScreen + 1) % SCREEN_COUNT;
  }
#else
  const auto &gnssConfig = config.getGNSSConfig();
  const auto &aprsConfig = config.getAPRSConfig();

  if (nextScreen == SCREEN_GNSS && !gnssConfig.enabled)
  {
    nextScreen = (nextScreen + 1) % SCREEN_COUNT;
  }

  // Note: SCREEN_APRS is always shown - updateAprsScreen() handles both modes appropriately
#endif

  currentScreen = static_cast<DisplayScreen>(nextScreen);
  lastScreenChange = millis();

  #ifndef KISS_SERIAL_MODE
  Serial.printf(" to %d\n", currentScreen);
  #endif

  // Force immediate update for screen change
  displayManager.markDirty(DisplayManager::SCREEN_CHANGED);
  updateDisplay();
}

void showLongPressProgress(float progress)
{
  static unsigned long lastProgressUpdate = 0;
  unsigned long now = millis();

  if (now - lastProgressUpdate < 100)
    return;
  lastProgressUpdate = now;

  display.clear();
  display.setFont(ArialMT_Plain_16);

  bool serialConnected = Serial && (Serial.availableForWrite() > 0);

  if (serialConnected)
  {
    display.drawString(10, 10, "Power Off");
    display.drawString(25, 30, "Disabled");
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 45, "Serial connected");
  }
  else
  {
    display.drawString(15, 10, "Power Off");

    int barWidth = 100;
    int barHeight = 8;
    int barX = (128 - barWidth) / 2;
    int barY = 30;

    display.drawRect(barX, barY, barWidth, barHeight);

    int fillWidth = (int)(barWidth * progress);
    if (fillWidth > 0)
    {
      for (int y = barY + 1; y < barY + barHeight - 1; y++)
      {
        display.drawHorizontalLine(barX + 1, y, fillWidth - 1);
      }
    }

    display.setFont(ArialMT_Plain_10);
    display.drawString(30, 45, "Release to cancel");
  }

  display.display();
}

void handleButtonLongPress()
{
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BTN] Long press detected");
  #endif

  bool serialConnected = Serial && (Serial.availableForWrite() > 0);

  if (serialConnected)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[PWR] Power off disabled - serial connection detected");
    #endif

    if (displayAvailable)
    {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Power Off");
      display.drawString(0, 20, "Serial connected");
      display.drawString(0, 30, "Power off disabled");
      display.display();
      delay(2000);
    }
  }
  else
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[PWR] Initiating power off sequence");
    #endif

    if (displayAvailable)
    {
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "Powering Off...");
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 35, "Goodbye!");
      display.display();
      delay(2000);

      display.clear();
      display.display();

      digitalWrite(VEXT_PIN, HIGH);
    }

    #ifndef KISS_SERIAL_MODE
    Serial.println("[PWR] Entering deep sleep...");
    #endif
    Serial.flush();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

    esp_deep_sleep_start();
  }
}

bool reinitializeDisplay()
{
  #ifndef KISS_SERIAL_MODE
  Serial.println("[OLED] Reinitializing I2C and display...");
  #endif

  // Power control
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[OLED] Vext power enabled (pin %d = LOW)\r\n", VEXT_PIN);
  #endif
  delay(300);

  // Reset sequence
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(100);
  digitalWrite(OLED_RST, HIGH);
  delay(200);
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[OLED] Reset sequence completed - V4 (pin %d)\r\n", OLED_RST);
  #endif

  // I2C initialization
  Wire.begin(OLED_SDA, OLED_SCL);
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[OLED] Using Heltec V4 OLED pins: SDA=%d, SCL=%d\r\n", OLED_SDA, OLED_SCL);
  #endif
  Wire.setClock(100000);
  delay(100);

  // Device detection
  #ifndef KISS_SERIAL_MODE
  Serial.println("[OLED] Checking for OLED at common addresses...");
  #endif
  bool deviceFound = false;
  for (byte address : {0x3C, 0x3D})
  {
    Wire.beginTransmission(address);
    byte result = Wire.endTransmission();
    if (result == 0)
    {
      #ifndef KISS_SERIAL_MODE
      Serial.printf("[OLED] I2C device found at address 0x%02X\r\n", address);
      #endif
      deviceFound = true;
    }
    delay(10);
  }

  if (!deviceFound)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[OLED] WARNING: No OLED found at 0x3C or 0x3D!");
    #endif
    return false;
  }

  // Display initialization
  #ifndef KISS_SERIAL_MODE
  Serial.println("[OLED] Initializing display...");
  #endif
  bool initResult = display.init();
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[OLED] Display init result: %s\r\n", initResult ? "SUCCESS" : "FAILED");
  #endif

  if (initResult)
  {
    Wire.setClock(400000);

    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);

    display.clear();
    display.drawString(0, 0, FPSTR(DISPLAY_LORAX));
    display.drawString(0, 12, FPSTR(DISPLAY_INITIALIZING));
    display.display();
    
    // Initialize display utilities and manager
    displayUtils.setup(&display);
    displayManager.init();
    
    RESOLVE_ERROR(ErrorHandler::ERROR_DISPLAY_INIT);
    #ifndef KISS_SERIAL_MODE
    Serial.println("[OLED] Display initialized successfully");
    #endif

    delay(1000);
    return true;
  }
  else
  {
    HANDLE_ERROR(ErrorHandler::ERROR_DISPLAY_INIT, "Init result false");
    #ifndef KISS_SERIAL_MODE
    Serial.println("[OLED] Display initialization failed");
    #endif
    return false;
  }
}

void setupDisplay()
{
  if (reinitializeDisplay())
  {
    displayAvailable = true;
  }
  else
  {
    displayAvailable = false;
    #ifndef KISS_SERIAL_MODE
    Serial.println("[OLED] Display setup failed - continuing without display");
    #endif
  }
}

void shutdownGNSS()
{
#if GNSS_ENABLE
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] Shutting down GNSS module...");
  #endif
  
  // Stop the GNSS driver
  gnss.setEnabled(false);
  
  // Power down the GNSS module
  digitalWrite(VGNSS_CTRL, HIGH);  // Turn off GNSS power
  digitalWrite(VEXT_CTRL, HIGH);   // Turn off external power  
  digitalWrite(GNSS_RST, LOW);     // Hold in reset
  digitalWrite(GNSS_WAKE, LOW);    // Sleep mode
  
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] GNSS module powered down");
  #endif
#endif
}

bool reinitializeGNSS()
{
#if GNSS_ENABLE
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] Reinitializing GNSS module...");
  #endif
  
  const auto& gnssCfg = config.getGNSSConfig();
  
  // Hardware initialization
  pinMode(VGNSS_CTRL, OUTPUT);
  digitalWrite(VGNSS_CTRL, LOW);   // Enable GNSS power
  
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);    // Enable external power
  
  pinMode(GNSS_RST, OUTPUT);
  digitalWrite(GNSS_RST, HIGH);    // Release reset
  
  pinMode(GNSS_WAKE, OUTPUT);
  digitalWrite(GNSS_WAKE, HIGH);   // Wake up
  
  pinMode(GNSS_PPS, INPUT);        // PPS input
  
  delay(500);  // Allow power to stabilize
  
  // Initialize UART communication
  if (gnssCfg.verboseLogging)
  {
#ifndef KISS_SERIAL_MODE
    Serial.printf("[GNSS] Starting UART at %lu baud (RX:%d, TX:%d)\n", 
                  gnssCfg.baudRate, GNSS_RX, GNSS_TX);
#endif
  }
  
  gnss.begin(gnssCfg.baudRate, GNSS_RX, GNSS_TX);
  gnss.setEnabled(true);
  
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] GNSS module reinitialized successfully");
  #endif
  return true;
#else
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] GNSS support not compiled in");
  #endif
  return false;
#endif
}

void setupWiFi()
{
  WiFiConfig &wifiCfg = config.getWiFiConfig();

  if (!wifiCfg.useAP && strlen(wifiCfg.sta_ssid) > 0)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[WIFI] Attempting STA connection to '%s'...\r\n", wifiCfg.sta_ssid);
    #endif
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiCfg.sta_ssid, wifiCfg.sta_password);

    if (displayAvailable)
    {
      display.clear();
      display.drawString(0, 0, "WiFi Connecting...");
      display.drawString(0, 12, String("SSID: ") + wifiCfg.sta_ssid);
      display.display();
    }

    unsigned long startTime = millis();
    int dots = 0;

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000)
    {
      delay(500);
      #ifndef KISS_SERIAL_MODE
      Serial.print(".");
      #endif

      if (displayAvailable && (millis() - startTime) % 2000 < 500)
      {
        display.clear();
        display.drawString(0, 0, "WiFi Connecting...");
        display.drawString(0, 12, String("SSID: ") + wifiCfg.sta_ssid);
        String progress = "Progress: ";
        for (int i = 0; i < (dots % 4); i++)
          progress += ".";
        display.drawString(0, 24, progress);
        display.display();
        dots++;
      }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      #ifndef KISS_SERIAL_MODE
      Serial.printf("\r\n[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
      Serial.printf("[WIFI] Signal: %d dBm\r\n", WiFi.RSSI());
      #endif
    }
    else
    {
      #ifndef KISS_SERIAL_MODE
      Serial.println("\n[WIFI] STA connection timeout, falling back to AP mode");
      #endif
      WiFi.disconnect();
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[WIFI] Starting AP mode - SSID: '%s'\r\n", wifiCfg.ssid);
    #endif
    WiFi.mode(WIFI_AP);

    if (displayAvailable)
    {
      display.clear();
      display.drawString(0, 0, "Starting AP...");
      display.drawString(0, 12, String("SSID: ") + wifiCfg.ssid);
      display.display();
    }

    WiFi.softAP(wifiCfg.ssid, wifiCfg.password);
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[WIFI] AP started - IP: %s\r\n", WiFi.softAPIP().toString().c_str());
    #endif
  }

  kissServer.begin();
  nmeaServer.begin();
  
  #ifndef KISS_SERIAL_MODE
  Serial.println("[NET] TCP servers started");
  Serial.printf("[NET] KISS server on port %d\n", TCP_KISS_PORT);
  Serial.printf("[NET] NMEA server on port %d\n", TCP_NMEA_PORT);
  #endif

  // Initialize WebSocket server for web interface
  WebSocketServer::begin(webServer);
  WebSocketServer::setRadio(&radio);
  WebSocketServer::setGNSS(&gnss);
  WebSocketServer::setKISS(&kiss);
  WebSocketServer::setConfig(&config);
  WebSocketServer::setBattery(&battery);
  
  // Start web server
  webServer.begin();
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[NET] Web server started on port %d\n", WEB_SERVER_PORT);
  Serial.println("[NET] Web interface available at http://[device-ip]/");
  #endif

  if (displayAvailable)
  {
    delay(1000);
    updateDisplay();
  }
}



void setup()
{
  Serial.begin(115200);

  unsigned long serialTimeout = millis();
  while (!Serial && (millis() - serialTimeout < 3000))
  {
    delay(100);
  }

  if (!Serial)
  {
    Serial.begin(115200);
  }

  delay(1000);

  #ifndef KISS_SERIAL_MODE
  Serial.println();
  Serial.println("===== SERIAL TEST START =====");
  Serial.println("If you can see this, serial communication is working!");
  #endif
  Serial.flush();
  delay(200);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Waking from deep sleep (button press)");
    #endif
  }

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Heltec KISS TNC starting...");
  #endif
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Target: Heltec WiFi LoRa 32 V4 (ESP32-S3)");
  Serial.println("[BOOT] USB CDC Serial Interface Active");
  #endif
#if GNSS_ENABLE
  #ifndef KISS_SERIAL_MODE
  Serial.printf("[BOOT] GNSS support ENABLED (pins RX=%d, TX=%d)\n", GNSS_RX, GNSS_TX);
  #endif
#else
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] GNSS support DISABLED");
  #endif
#endif

  #ifndef KISS_SERIAL_MODE
  Serial.printf("[BOOT] Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[BOOT] CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
  #endif
  Serial.flush();

  // Initialize error handling system
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Initializing error handling system...");
  #endif
  errorHandler.init();
  errorHandler.enableWatchdog(60000);  // 60 second watchdog - longer for initial setup
  
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Initializing configuration system...");
  #endif
  Serial.flush();

  bool configOk = false;
  try
  {
    configOk = config.begin();
  }
  catch (...)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Exception during config initialization!");
    #endif
    configOk = false;
  }

  if (!configOk)
  {
    HANDLE_ERROR(ErrorHandler::ERROR_CONFIG_LOAD, "Config initialization failed");
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Failed to initialize configuration, using defaults");
    Serial.println("[BOOT] This is not fatal - continuing with defaults...");
    #endif
  }
  else
  {
    RESOLVE_ERROR(ErrorHandler::ERROR_CONFIG_LOAD);
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Configuration system initialized successfully");
    #endif
  }
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[RADIO] Loading configuration...");
  #endif
  Serial.flush();
  radio.loadConfig();
  #ifndef KISS_SERIAL_MODE
  Serial.println("[RADIO] Configuration loaded successfully");
  #endif
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[RADIO] Initializing radio hardware...");
  #endif
  Serial.flush();

  bool radioOk = false;
  try
  {
    radioOk = radio.begin(onRadioFrameRx);
  }
  catch (...)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Exception during radio initialization!");
    #endif
    radioOk = false;
  }
  Serial.flush();

  if (radioOk)
  {
    RESOLVE_ERROR(ErrorHandler::ERROR_RADIO_INIT);
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Radio initialized successfully");
    #endif
    const auto &cfg = config.getRadioConfig();
#ifndef KISS_SERIAL_MODE
    Serial.printf("[RADIO] Config: %.3f MHz, %d dBm, SF%d, BW%.1f kHz\n",
                  cfg.frequency, cfg.txPower, cfg.spreadingFactor, cfg.bandwidth);
#endif
  }
  else
  {
    HANDLE_ERROR(ErrorHandler::ERROR_RADIO_INIT, "Hardware init failed");
    #ifndef KISS_SERIAL_MODE
    Serial.println("[RADIO] Radio initialization FAILED or SKIPPED!");
    Serial.println("[RADIO] Check hardware connections and SPI configuration");
    Serial.println("[RADIO] Continuing with radio disabled...");
    #endif
  }
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[KISS] Setting up KISS protocol handlers...");
  #endif
  Serial.flush();
  kiss.setOnFrame(onKissFrameRx);
  kiss.setOnCommand(onKissCommandRx);
  #ifndef KISS_SERIAL_MODE
  Serial.println("[KISS] KISS protocol handlers configured");
  #endif
  Serial.flush();

#if GNSS_ENABLE
  #ifndef KISS_SERIAL_MODE
  Serial.println("[GNSS] Loading configuration...");
  #endif
  gnss.loadConfig();
  GNSSConfig &gnssCfg = config.getGNSSConfig();

  #ifndef KISS_SERIAL_MODE
  Serial.printf("[GNSS] Config: %s, Baud: %lu\n", gnssCfg.enabled ? "Enabled" : "Disabled", gnssCfg.baudRate);
  #endif
  if (gnssCfg.verboseLogging)
  {
#ifndef KISS_SERIAL_MODE
    Serial.printf("[GNSS] Routing: TCP:%s USB:%s\n",
                  gnssCfg.routeToTcp ? "Y" : "N",
                  gnssCfg.routeToUsb ? "Y" : "N");
#endif
  }

  if (gnssCfg.enabled)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] Initializing V4 GNSS module...");
    #endif

    pinMode(VGNSS_CTRL, OUTPUT);
    digitalWrite(VGNSS_CTRL, LOW);

    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW);

    pinMode(GNSS_RST, OUTPUT);
    digitalWrite(GNSS_RST, HIGH);

    pinMode(GNSS_WAKE, OUTPUT);
    digitalWrite(GNSS_WAKE, HIGH);

    pinMode(GNSS_PPS, INPUT);

    delay(500);

    if (gnssCfg.verboseLogging)
    {
      #ifndef KISS_SERIAL_MODE
      Serial.printf("[GNSS] Starting UART at %lu baud (RX:%d, TX:%d)\n", gnssCfg.baudRate, GNSS_RX, GNSS_TX);
      #endif
    }
    gnss.begin(gnssCfg.baudRate, GNSS_RX, GNSS_TX);
    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] V4 GNSS module initialized successfully");
    #endif
  }
  else
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[GNSS] GNSS disabled - use config menu to enable");
    #endif
  }
#endif

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Setting up user button...");
  #endif
  Serial.flush();
  setupButton();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Initializing battery monitor...");
  #endif
  Serial.flush();
  if (!battery.begin(&config))
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Battery monitor initialization failed");
    #endif
  }
  else
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] Battery monitor initialized successfully");
    #endif
  }
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[APRS] Initializing APRS driver...");
  #endif
  Serial.flush();
  if (!aprs.begin(&radio, &gnss))
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[APRS] APRS driver initialization failed - continuing without APRS");
    #endif
  }
  else
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[APRS] APRS driver initialized successfully");
    #endif
  }
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Setting up OLED display...");
  #endif
  Serial.flush();
  setupDisplay();
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] OLED display setup complete");
  #endif
  Serial.flush();

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Setting up WiFi...");
  #endif
  Serial.flush();
  setupWiFi();

  // Initialize OTA after WiFi is set up
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Initializing OTA Manager...");
  #endif
  OTA.begin("LoRaTNCX", OTA_PORT);
  
  // Set OTA callbacks for status updates
  OTA.setStatusCallback([](OTAManager::UpdateStatus status, const String& message) {
    #ifndef KISS_SERIAL_MODE
    Serial.printf("[OTA] Status: %s\n", message.c_str());
    #endif
  });
  
  OTA.setProgressCallback([](const OTAManager::ProgressInfo& progress) {
    if (progress.percentage % 10 == 0) {  // Log every 10%
#ifndef KISS_SERIAL_MODE
      Serial.printf("[OTA] Progress: %d%% (%zu/%zu bytes)\n", 
                   progress.percentage, progress.bytesReceived, progress.totalBytes);
#endif
    }
  });

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] ========================================");
  #endif

  const auto &aprsConfig = config.getAPRSConfig();
  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] APRS Tracker Ready!");
    Serial.printf("[BOOT] Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    Serial.printf("[BOOT] Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    #endif
    if (aprsConfig.smartBeaconing)
    {
      #ifndef KISS_SERIAL_MODE
      Serial.println("[BOOT] Smart Beaconing: Enabled");
      #endif
    }
  }
  else
  {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[BOOT] LoRaTNCX Ready!");
    #endif
  }

  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] ========================================");
  Serial.println("[BOOT] Interfaces:");
  Serial.println("[BOOT]   USB CDC: KISS protocol");
  Serial.println("[BOOT]   TCP 8001: KISS protocol");
  #endif
#ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT]   TCP 10110: NMEA sentences");
  Serial.println("[BOOT]");
  Serial.println("[BOOT] Controls:");
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT]   Send '+++' to enter configuration menu");
  Serial.println("[BOOT]   Press user button to cycle OLED screens");
  Serial.println("[BOOT]   Hold user button 2s to power off (when serial disconnected)");
  Serial.println("[BOOT] ========================================");
  #endif
  
  // Final watchdog feed before entering main loop
  #ifndef KISS_SERIAL_MODE
  Serial.println("[BOOT] Setup complete, feeding watchdog before main loop");
  #endif
#endif
  FEED_WATCHDOG();
}

void serviceTCP()
{
  if (!kissClient.connected())
  {
    WiFiClient c = kissServer.accept();
    if (c)
      kissClient = c;
  }
  else if (!kissClient.connected())
  {
    kissClient.stop();
  }

  if (!nmeaClient.connected())
  {
    WiFiClient c = nmeaServer.accept();
    if (c)
      nmeaClient = c;
  }
  else if (!nmeaClient.connected())
  {
    nmeaClient.stop();
  }

  if (kissClient && kissClient.available())
  {
    while (kissClient.available())
    {
      kiss.pushSerialByte(kissClient.read());
    }
  }
}

String menuBuffer = "";
unsigned long lastMenuChar = 0;
// MENU_TIMEOUT_MS now defined in Constants.h

void checkMenuActivation()
{
  if (config.inMenu)
  {
    config.handleMenuInput();
    return;
  }

  while (Serial.available())
  {
    char c = Serial.read();

    if (millis() - lastMenuChar > MENU_TIMEOUT_MS)
    {
      for (char ch : menuBuffer)
      {
        kiss.pushSerialByte(ch);
      }
      menuBuffer = "";
    }
    lastMenuChar = millis();

    menuBuffer += c;

    if (menuBuffer.length() > 3)
    {
      kiss.pushSerialByte(menuBuffer[0]);
      menuBuffer = menuBuffer.substring(1);
    }

    if (menuBuffer == "+++")
    {
      #ifndef KISS_SERIAL_MODE
      Serial.println("\n[CONFIG] Entering configuration menu...");
      #endif
      config.showMenu();
      menuBuffer = "";
      return;
    }
  }

  if (menuBuffer.length() > 0 && millis() - lastMenuChar > MENU_TIMEOUT_MS)
  {
    for (char ch : menuBuffer)
    {
      kiss.pushSerialByte(ch);
    }
    menuBuffer = "";
  }
}

void loop()
{
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastHealthCheck = 0;
  static bool firstLoop = true;

  // Feed watchdog to prevent reset
  FEED_WATCHDOG();

  // Handle OTA updates
  OTA.handle();

  // Debug output for first loop iteration
  if (firstLoop) {
    #ifndef KISS_SERIAL_MODE
    Serial.println("[LOOP] First loop iteration started");
    #endif
    firstLoop = false;
  }

  // Periodic health checks
  if (millis() - lastHealthCheck > 10000) {  // Every 10 seconds
    errorHandler.checkMemoryHealth();
    lastHealthCheck = millis();
  }

  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS)
  {
#ifndef KISS_SERIAL_MODE
    Serial.printf("[LOOP] System running - uptime: %lu seconds, free heap: %d\n",
                  millis() / 1000, ESP.getFreeHeap());
    
    // Report error status if any errors exist
    if (errorHandler.getTotalErrorCount() > 0) {
      #ifndef KISS_SERIAL_MODE
      Serial.printf("[LOOP] %s\n", errorHandler.getErrorSummary().c_str());
      #endif
    }
#endif
    
    lastHeartbeat = millis();
  }

  handleButton();

  battery.poll();

  // Feed watchdog after battery polling
  FEED_WATCHDOG();

  const auto &aprsConfig = config.getAPRSConfig();

  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    aprs.poll();
    checkMenuActivation();
  }
  else
  {
    checkMenuActivation();
  }

  // Feed watchdog after menu/APRS processing
  FEED_WATCHDOG();

#if GNSS_ENABLE
  if (config.getGNSSConfig().enabled)
  {
    gnss.poll();

    if (GNSS_PPS >= 0 && millis() - lastPpsCheck > 5000)
    {
      int ppsState = digitalRead(GNSS_PPS);
      if (lastPpsState != -1 && ppsState != lastPpsState)
      {
        ppsToggleCount++;
      }
      if (millis() - lastPpsCheck > 5000)
      {
        if (ppsToggleCount > 0 && config.getGNSSConfig().verboseLogging)
        {
          #ifndef KISS_SERIAL_MODE
          Serial.printf("[GNSS] [INFO] PPS active: %lu toggles in 5s\n", ppsToggleCount);
          #endif
        }
        ppsToggleCount = 0;
        lastPpsCheck = millis();
      }
      lastPpsState = ppsState;
    }
  }
#endif

  // Feed watchdog after GNSS processing
  FEED_WATCHDOG();

  serviceTCP();

  // Handle WebSocket server
  WebSocketServer::handle();

  // Feed watchdog after network processing
  FEED_WATCHDOG();

  if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL_MS)
  {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}
