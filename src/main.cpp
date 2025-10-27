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

  Serial.printf("[RADIO] Attempting TX: %d bytes\n", len);

  Serial.print("[RADIO] Data: ");
  for (size_t i = 0; i < min(len, (size_t)16); i++)
  {
    Serial.printf("%02X ", frame[i]);
  }
  if (len > 16)
    Serial.print("...");
  Serial.println();

  bool txResult = radio.send(frame, len);
  monitor.lastRadioDebug = "TX " + String(len) + " bytes: " + (txResult ? "SUCCESS" : "FAILED");
  monitor.lastRadioTime = millis();
  if (txResult)
  {
    monitor.radioTxCount++;
  }

  if (!txResult)
  {
    Serial.printf("[RADIO] TX FAILED for frame of %d bytes\n", len);
    Serial.println("[RADIO] Possible causes: Radio not initialized, invalid config, or hardware issue");
  }
  else
  {
    Serial.printf("[RADIO] TX SUCCESS: %d bytes transmitted\n", len);
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
      Serial.printf("[KISS] TX_DELAY set to %d (x10ms)\n", data[0]);
    }
    break;

  case KISS::PERSISTENCE:
    if (len >= 1)
    {
      radio.setPersist(data[0]);
      config.getRadioConfig().persist = data[0];
      Serial.printf("[KISS] PERSISTENCE set to %d\n", data[0]);
    }
    break;

  case KISS::SLOT_TIME:
    if (len >= 1)
    {
      radio.setSlotTime(data[0]);
      config.getRadioConfig().slotTime = data[0];
      Serial.printf("[KISS] SLOT_TIME set to %d (x10ms)\n", data[0]);
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
            Serial.printf("[KISS] Frequency set to %.3f MHz\n", freq);
          }
          else
          {
            Serial.printf("[KISS] Failed to set frequency %.3f MHz\n", freq);
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
            Serial.printf("[KISS] TX power set to %d dBm\n", power);
          }
          else
          {
            Serial.printf("[KISS] Failed to set TX power %d dBm\n", power);
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
            Serial.printf("[KISS] Bandwidth set to %.1f kHz\n", bw);
          }
          else
          {
            Serial.printf("[KISS] Failed to set bandwidth %.1f kHz\n", bw);
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
            Serial.printf("[KISS] Spreading factor set to %d\n", sf);
          }
          else
          {
            Serial.printf("[KISS] Failed to set spreading factor %d\n", sf);
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
            Serial.printf("[KISS] Coding rate set to 4/%d\n", cr);
          }
          else
          {
            Serial.printf("[KISS] Failed to set coding rate 4/%d\n", cr);
          }
        }
        break;

      case 0x10:
      {
        Serial.printf("[KISS] Current radio config:\n");
        Serial.printf("  Frequency: %.3f MHz\n", radio.getFrequency());
        Serial.printf("  TX Power: %d dBm\n", radio.getTxPower());
        Serial.printf("  Bandwidth: %.1f kHz\n", radio.getBandwidth());
        Serial.printf("  Spreading Factor: %d\n", radio.getSpreadingFactor());
        Serial.printf("  Coding Rate: 4/%d\n", radio.getCodingRate());
        Serial.printf("  TX Delay: %d x 10ms\n", radio.getTxDelay());
        Serial.printf("  Persistence: %d\n", radio.getPersist());
        Serial.printf("  Slot Time: %d x 10ms\n", radio.getSlotTime());
      }
      break;

      default:
        Serial.printf("[KISS] Unknown hardware command: 0x%02X\n", hw_cmd);
        break;
      }
    }
    break;

  default:
    Serial.printf("[KISS] Unknown command: 0x%02X\n", cmd);
    break;
  }
}

void onRadioFrameRx(const uint8_t *frame, size_t len, int16_t rssi, float snr)
{
  monitor.lastRadioDebug = "RX " + String(len) + " bytes, RSSI: " + String(rssi) + " dBm, SNR: " + String(snr, 1) + " dB";
  monitor.lastRadioTime = millis();
  monitor.radioRxCount++;
  Serial.printf("[RADIO] RX: %d bytes, RSSI: %d dBm, SNR: %.1f dB\n", len, rssi, snr);

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
  Serial.printf("[BTN] User button initialized on GPIO %d\n", USER_BTN);
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
        Serial.println("[BTN] Button pressed");
      }
      else if (currentState == HIGH && buttonState.lastState == LOW)
      {
        buttonState.lastReleaseTime = now;
        unsigned long pressDuration = now - buttonState.lastPressTime;

        Serial.printf("[BTN] Button released after %lums\n", pressDuration);

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
  Serial.printf("[BTN] Short press - changing screen from %d", currentScreen);

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

  Serial.printf(" to %d\n", currentScreen);

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
  Serial.println("[BTN] Long press detected");

  bool serialConnected = Serial && (Serial.availableForWrite() > 0);

  if (serialConnected)
  {
    Serial.println("[PWR] Power off disabled - serial connection detected");

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
    Serial.println("[PWR] Initiating power off sequence");

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

    Serial.println("[PWR] Entering deep sleep...");
    Serial.flush();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

    esp_deep_sleep_start();
  }
}

void setupDisplay()
{
  Serial.println("[OLED] Initializing I2C and display...");

  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  Serial.printf("[OLED] Vext power enabled (pin %d = LOW)\r\n", VEXT_PIN);
  delay(300);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(100);
  digitalWrite(OLED_RST, HIGH);
  delay(200);
  Serial.printf("[OLED] Reset sequence completed - V4 (pin %d)\r\n", OLED_RST);

  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.printf("[OLED] Using Heltec V4 OLED pins: SDA=%d, SCL=%d\r\n", OLED_SDA, OLED_SCL);

  Wire.setClock(100000);
  delay(100);

  Serial.println("[OLED] Checking for OLED at common addresses...");
  bool deviceFound = false;
  for (byte address : {0x3C, 0x3D})
  {
    Wire.beginTransmission(address);
    byte result = Wire.endTransmission();
    if (result == 0)
    {
      Serial.printf("[OLED] I2C device found at address 0x%02X\r\n", address);
      deviceFound = true;
    }
    delay(10);
  }

  if (!deviceFound)
  {
    Serial.println("[OLED] WARNING: No OLED found at 0x3C or 0x3D!");
    displayAvailable = false;
    return;
  }

  Serial.println("[OLED] Initializing display...");
  bool initResult = display.init();
  Serial.printf("[OLED] Display init result: %s\r\n", initResult ? "SUCCESS" : "FAILED");

  if (initResult)
  {
    Wire.setClock(400000);

    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);

    display.clear();
    display.drawString(0, 0, FPSTR(DISPLAY_LORAX));
    display.drawString(0, 12, FPSTR(DISPLAY_INITIALIZING));
    display.display();

    displayAvailable = true;
    
    // Initialize display utilities and manager
    displayUtils.setup(&display);
    displayManager.init();
    
    RESOLVE_ERROR(ErrorHandler::ERROR_DISPLAY_INIT);
    Serial.println("[OLED] Display initialized successfully");

    delay(1000);
  }
  else
  {
    displayAvailable = false;
    HANDLE_ERROR(ErrorHandler::ERROR_DISPLAY_INIT, "Init result false");
    Serial.println("[OLED] Display initialization failed - continuing without display");
  }
}

void setupWiFi()
{
  WiFiConfig &wifiCfg = config.getWiFiConfig();

  if (!wifiCfg.useAP && strlen(wifiCfg.sta_ssid) > 0)
  {
    Serial.printf("[WIFI] Attempting STA connection to '%s'...\r\n", wifiCfg.sta_ssid);
    
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
      Serial.print(".");

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
      Serial.printf("\r\n[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
      Serial.printf("[WIFI] Signal: %d dBm\r\n", WiFi.RSSI());
    }
    else
    {
      Serial.println("\n[WIFI] STA connection timeout, falling back to AP mode");
      WiFi.disconnect();
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("[WIFI] Starting AP mode - SSID: '%s'\r\n", wifiCfg.ssid);
    WiFi.mode(WIFI_AP);

    if (displayAvailable)
    {
      display.clear();
      display.drawString(0, 0, "Starting AP...");
      display.drawString(0, 12, String("SSID: ") + wifiCfg.ssid);
      display.display();
    }

    WiFi.softAP(wifiCfg.ssid, wifiCfg.password);
    Serial.printf("[WIFI] AP started - IP: %s\r\n", WiFi.softAPIP().toString().c_str());
  }

  kissServer.begin();
  nmeaServer.begin();
  
  Serial.println("[NET] TCP servers started");
  Serial.printf("[NET] KISS server on port %d\n", TCP_KISS_PORT);
  Serial.printf("[NET] NMEA server on port %d\n", TCP_NMEA_PORT);

  // Initialize WebSocket server for web interface
  WebSocketServer::begin(webServer);
  WebSocketServer::setRadio(&radio);
  WebSocketServer::setGNSS(&gnss);
  WebSocketServer::setKISS(&kiss);
  WebSocketServer::setConfig(&config);
  WebSocketServer::setBattery(&battery);
  
  // Start web server
  webServer.begin();
  Serial.printf("[NET] Web server started on port %d\n", WEB_SERVER_PORT);
  Serial.println("[NET] Web interface available at http://[device-ip]/");

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

  Serial.println();
  Serial.println("===== SERIAL TEST START =====");
  Serial.println("If you can see this, serial communication is working!");
  Serial.flush();
  delay(200);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("[BOOT] Waking from deep sleep (button press)");
  }

  Serial.println("[BOOT] Heltec KISS TNC starting...");
  Serial.flush();

  Serial.println("[BOOT] Target: Heltec WiFi LoRa 32 V4 (ESP32-S3)");
  Serial.println("[BOOT] USB CDC Serial Interface Active");
#if GNSS_ENABLE
  Serial.printf("[BOOT] GNSS support ENABLED (pins RX=%d, TX=%d)\n", GNSS_RX, GNSS_TX);
#else
  Serial.println("[BOOT] GNSS support DISABLED");
#endif

  Serial.printf("[BOOT] Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[BOOT] CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.flush();

  // Initialize error handling system
  Serial.println("[BOOT] Initializing error handling system...");
  errorHandler.init();
  errorHandler.enableWatchdog(60000);  // 60 second watchdog - longer for initial setup
  
  Serial.println("[BOOT] Initializing configuration system...");
  Serial.flush();

  bool configOk = false;
  try
  {
    configOk = config.begin();
  }
  catch (...)
  {
    Serial.println("[BOOT] Exception during config initialization!");
    configOk = false;
  }

  if (!configOk)
  {
    HANDLE_ERROR(ErrorHandler::ERROR_CONFIG_LOAD, "Config initialization failed");
    Serial.println("[BOOT] Failed to initialize configuration, using defaults");
    Serial.println("[BOOT] This is not fatal - continuing with defaults...");
  }
  else
  {
    RESOLVE_ERROR(ErrorHandler::ERROR_CONFIG_LOAD);
    Serial.println("[BOOT] Configuration system initialized successfully");
  }
  Serial.flush();

  Serial.println("[RADIO] Loading configuration...");
  Serial.flush();
  radio.loadConfig();
  Serial.println("[RADIO] Configuration loaded successfully");
  Serial.flush();

  Serial.println("[RADIO] Initializing radio hardware...");
  Serial.flush();

  bool radioOk = false;
  try
  {
    radioOk = radio.begin(onRadioFrameRx);
  }
  catch (...)
  {
    Serial.println("[RADIO] Exception during radio initialization!");
    radioOk = false;
  }
  Serial.flush();

  if (radioOk)
  {
    RESOLVE_ERROR(ErrorHandler::ERROR_RADIO_INIT);
    Serial.println("[RADIO] Radio initialized successfully");
    const auto &cfg = config.getRadioConfig();
    Serial.printf("[RADIO] Config: %.3f MHz, %d dBm, SF%d, BW%.1f kHz\n",
                  cfg.frequency, cfg.txPower, cfg.spreadingFactor, cfg.bandwidth);
  }
  else
  {
    HANDLE_ERROR(ErrorHandler::ERROR_RADIO_INIT, "Hardware init failed");
    Serial.println("[RADIO] Radio initialization FAILED or SKIPPED!");
    Serial.println("[RADIO] Check hardware connections and SPI configuration");
    Serial.println("[RADIO] Continuing with radio disabled...");
  }
  Serial.flush();

  Serial.println("[KISS] Setting up KISS protocol handlers...");
  Serial.flush();
  kiss.setOnFrame(onKissFrameRx);
  kiss.setOnCommand(onKissCommandRx);
  Serial.println("[KISS] KISS protocol handlers configured");
  Serial.flush();

#if GNSS_ENABLE
  Serial.println("[GNSS] Loading configuration...");
  gnss.loadConfig();
  GNSSConfig &gnssCfg = config.getGNSSConfig();

  Serial.printf("[GNSS] Config: %s, Baud: %lu\n", gnssCfg.enabled ? "Enabled" : "Disabled", gnssCfg.baudRate);
  if (gnssCfg.verboseLogging)
  {
    Serial.printf("[GNSS] Routing: TCP:%s USB:%s\n",
                  gnssCfg.routeToTcp ? "Y" : "N",
                  gnssCfg.routeToUsb ? "Y" : "N");
  }

  if (gnssCfg.enabled)
  {
    Serial.println("[GNSS] Initializing V4 GNSS module...");

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
      Serial.printf("[GNSS] Starting UART at %lu baud (RX:%d, TX:%d)\n", gnssCfg.baudRate, GNSS_RX, GNSS_TX);
    }
    gnss.begin(gnssCfg.baudRate, GNSS_RX, GNSS_TX);
    Serial.println("[GNSS] V4 GNSS module initialized successfully");
  }
  else
  {
    Serial.println("[GNSS] GNSS disabled - use config menu to enable");
  }
#endif

  Serial.println("[BOOT] Setting up user button...");
  Serial.flush();
  setupButton();

  Serial.println("[BOOT] Initializing battery monitor...");
  Serial.flush();
  if (!battery.begin(&config))
  {
    Serial.println("[BOOT] Battery monitor initialization failed");
  }
  else
  {
    Serial.println("[BOOT] Battery monitor initialized successfully");
  }
  Serial.flush();

  Serial.println("[APRS] Initializing APRS driver...");
  Serial.flush();
  if (!aprs.begin(&radio, &gnss))
  {
    Serial.println("[APRS] APRS driver initialization failed - continuing without APRS");
  }
  else
  {
    Serial.println("[APRS] APRS driver initialized successfully");
  }
  Serial.flush();

  Serial.println("[BOOT] Setting up OLED display...");
  Serial.flush();
  setupDisplay();
  Serial.println("[BOOT] OLED display setup complete");
  Serial.flush();

  Serial.println("[BOOT] Setting up WiFi...");
  Serial.flush();
  setupWiFi();

  // Initialize OTA after WiFi is set up
  Serial.println("[BOOT] Initializing OTA Manager...");
  OTA.begin("LoRaTNCX", OTA_PORT);
  
  // Set OTA callbacks for status updates
  OTA.setStatusCallback([](OTAManager::UpdateStatus status, const String& message) {
    Serial.printf("[OTA] Status: %s\n", message.c_str());
  });
  
  OTA.setProgressCallback([](const OTAManager::ProgressInfo& progress) {
    if (progress.percentage % 10 == 0) {  // Log every 10%
      Serial.printf("[OTA] Progress: %d%% (%zu/%zu bytes)\n", 
                   progress.percentage, progress.bytesReceived, progress.totalBytes);
    }
  });

  Serial.println("[BOOT] ========================================");

  const auto &aprsConfig = config.getAPRSConfig();
  if (aprsConfig.mode == OperatingMode::APRS_TRACKER)
  {
    Serial.println("[BOOT] APRS Tracker Ready!");
    Serial.printf("[BOOT] Callsign: %s-%d\n", aprsConfig.callsign, aprsConfig.ssid);
    Serial.printf("[BOOT] Beacon Interval: %lu seconds\n", aprsConfig.beaconInterval);
    if (aprsConfig.smartBeaconing)
    {
      Serial.println("[BOOT] Smart Beaconing: Enabled");
    }
  }
  else
  {
    Serial.println("[BOOT] LoRaTNCX Ready!");
  }

  Serial.println("[BOOT] ========================================");
  Serial.println("[BOOT] Interfaces:");
  Serial.println("[BOOT]   USB CDC: KISS protocol");
  Serial.println("[BOOT]   TCP 8001: KISS protocol");
  Serial.println("[BOOT]   TCP 10110: NMEA sentences");
  Serial.println("[BOOT]");
  Serial.println("[BOOT] Controls:");
  Serial.println("[BOOT]   Send '+++' to enter configuration menu");
  Serial.println("[BOOT]   Press user button to cycle OLED screens");
  Serial.println("[BOOT]   Hold user button 2s to power off (when serial disconnected)");
  Serial.println("[BOOT] ========================================");
  
  // Final watchdog feed before entering main loop
  Serial.println("[BOOT] Setup complete, feeding watchdog before main loop");
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
      Serial.println("\n[CONFIG] Entering configuration menu...");
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
    Serial.println("[LOOP] First loop iteration started");
    firstLoop = false;
  }

  // Periodic health checks
  if (millis() - lastHealthCheck > 10000) {  // Every 10 seconds
    errorHandler.checkMemoryHealth();
    lastHealthCheck = millis();
  }

  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS)
  {
    Serial.printf("[LOOP] System running - uptime: %lu seconds, free heap: %d\n",
                  millis() / 1000, ESP.getFreeHeap());
    
    // Report error status if any errors exist
    if (errorHandler.getTotalErrorCount() > 0) {
      Serial.printf("[LOOP] %s\n", errorHandler.getErrorSummary().c_str());
    }
    
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
          Serial.printf("[GNSS] [INFO] PPS active: %lu toggles in 5s\n", ppsToggleCount);
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
