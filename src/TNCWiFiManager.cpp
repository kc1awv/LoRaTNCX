#include "TNCWiFiManager.h"

#include <esp_system.h>

TNCWiFiManager::TNCWiFiManager()
    : networkCount(0), connectedNetworkIndex(-1), apModeActive(false), stationAttemptActive(false),
      stationAttemptIndex(0), stationAttemptsTried(0), stationAttemptStart(0), lastReconnectCheck(0)
{
}

bool TNCWiFiManager::begin()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.disconnect(true, true);

    configureDefaultAPCredentials();
    loadNetworksFromPreferences();

    if (networkCount > 0)
    {
        startStationAttempt(0);
    }
    else
    {
        startAccessPoint();
    }

    return true;
}

void TNCWiFiManager::update()
{
    unsigned long now = millis();

    if (stationAttemptActive)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            stationAttemptActive = false;
            apModeActive = false;
            connectedNetworkIndex = static_cast<int8_t>(stationAttemptIndex);
            stationAttemptsTried = 0;
            lastReconnectCheck = now;
            WiFi.softAPdisconnect(true);
        }
        else if (now - stationAttemptStart >= STA_CONNECT_TIMEOUT_MS)
        {
            advanceStationAttempt();
        }
    }
    else
    {
        if ((WiFi.getMode() & WIFI_MODE_STA) && WiFi.status() == WL_CONNECTED)
        {
            connectedNetworkIndex = connectedNetworkIndex >= 0 ? connectedNetworkIndex : static_cast<int8_t>(stationAttemptIndex);
        }
        else if (networkCount > 0 && (now - lastReconnectCheck) >= RECONNECT_INTERVAL_MS)
        {
            startStationAttempt(connectedNetworkIndex >= 0 ? static_cast<uint8_t>(connectedNetworkIndex) : 0);
        }
        else if (networkCount == 0)
        {
            ensureFallbackIfIdle();
        }
    }

    if (!stationAttemptActive && networkCount == 0)
    {
        ensureFallbackIfIdle();
    }
}

bool TNCWiFiManager::addNetwork(const String &ssid, const String &password, String &message)
{
    String trimmedSSID = ssid;
    trimmedSSID.trim();

    if (trimmedSSID.length() == 0)
    {
        message = "SSID cannot be empty";
        return false;
    }

    if (password.length() < 8)
    {
        message = "Password must be at least 8 characters";
        return false;
    }

    for (uint8_t i = 0; i < networkCount; ++i)
    {
        if (networks[i].ssid == trimmedSSID)
        {
            networks[i].password = password;
            saveNetworksToPreferences();
            message = "Updated credentials for " + trimmedSSID;
            startStationAttempt(i);
            return true;
        }
    }

    if (networkCount >= MAX_NETWORKS)
    {
        message = "Maximum of " + String(MAX_NETWORKS) + " networks supported";
        return false;
    }

    networks[networkCount].ssid = trimmedSSID;
    networks[networkCount].password = password;
    networkCount++;
    saveNetworksToPreferences();

    message = "Added network " + trimmedSSID;
    startStationAttempt(networkCount - 1);
    return true;
}

bool TNCWiFiManager::removeNetwork(const String &ssid, String &message)
{
    String trimmedSSID = ssid;
    trimmedSSID.trim();

    for (uint8_t i = 0; i < networkCount; ++i)
    {
        if (networks[i].ssid == trimmedSSID)
        {
            for (uint8_t j = i; j + 1 < networkCount; ++j)
            {
                networks[j] = networks[j + 1];
            }
            networkCount--;
            saveNetworksToPreferences();

            if (connectedNetworkIndex == static_cast<int8_t>(i))
            {
                WiFi.disconnect(true, true);
                connectedNetworkIndex = -1;
                stationAttemptActive = false;
                if (networkCount > 0)
                {
                    startStationAttempt(0);
                }
                else
                {
                    startAccessPoint();
                }
            }
            else if (connectedNetworkIndex > static_cast<int8_t>(i))
            {
                connectedNetworkIndex--;
            }

            message = "Removed network " + trimmedSSID;
            return true;
        }
    }

    message = "Network " + trimmedSSID + " not found";
    return false;
}

String TNCWiFiManager::getNetworksSummary() const
{
    if (networkCount == 0)
    {
        return String("No STA networks configured");
    }

    String summary = "Configured STA networks:";
    for (uint8_t i = 0; i < networkCount; ++i)
    {
        summary += "\n  ";
        summary += networks[i].ssid;
        if (connectedNetworkIndex == static_cast<int8_t>(i) && WiFi.status() == WL_CONNECTED)
        {
            summary += " (active)";
        }
    }
    return summary;
}

String TNCWiFiManager::getStatusSummary() const
{
    String status;
    wl_status_t wifiStatus = WiFi.status();
    wifi_mode_t mode = WiFi.getMode();

    if ((mode & WIFI_MODE_STA) && wifiStatus == WL_CONNECTED)
    {
        status = "STA connected to " + WiFi.SSID();
        IPAddress ip = WiFi.localIP();
        status += " (" + ip.toString() + ")";
    }
    else if (apModeActive)
    {
        status = "AP " + apSSID + " (" + WiFi.softAPIP().toString() + ")";
    }
    else if (stationAttemptActive && stationAttemptIndex < networkCount)
    {
        status = "Connecting to " + networks[stationAttemptIndex].ssid + "...";
    }
    else
    {
        status = "WiFi idle";
    }

    status += " | Configured STA networks: " + String(networkCount);
    return status;
}

void TNCWiFiManager::loadNetworksFromPreferences()
{
    Preferences preferences;
    if (!preferences.begin(PREF_NAMESPACE, true))
    {
        networkCount = 0;
        return;
    }

    networkCount = preferences.getUChar("count", 0);
    if (networkCount > MAX_NETWORKS)
    {
        networkCount = MAX_NETWORKS;
    }

    for (uint8_t i = 0; i < networkCount; ++i)
    {
        String keySsid = "ssid" + String(i);
        String keyPwd = "pwd" + String(i);
        networks[i].ssid = preferences.getString(keySsid.c_str(), "");
        networks[i].password = preferences.getString(keyPwd.c_str(), "");
    }

    preferences.end();
}

void TNCWiFiManager::saveNetworksToPreferences()
{
    Preferences preferences;
    if (!preferences.begin(PREF_NAMESPACE, false))
    {
        return;
    }

    preferences.putUChar("count", networkCount);
    for (uint8_t i = 0; i < networkCount; ++i)
    {
        String keySsid = "ssid" + String(i);
        String keyPwd = "pwd" + String(i);
        preferences.putString(keySsid.c_str(), networks[i].ssid);
        preferences.putString(keyPwd.c_str(), networks[i].password);
    }

    for (uint8_t i = networkCount; i < MAX_NETWORKS; ++i)
    {
        String keySsid = "ssid" + String(i);
        String keyPwd = "pwd" + String(i);
        preferences.remove(keySsid.c_str());
        preferences.remove(keyPwd.c_str());
    }

    preferences.end();
}

void TNCWiFiManager::configureDefaultAPCredentials()
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);

    apSSID = "LoRaTNCX-" + String(suffix);
    apPassword = DEFAULT_AP_PASSWORD;
}

void TNCWiFiManager::startAccessPoint()
{
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    apModeActive = true;
    stationAttemptActive = false;
    connectedNetworkIndex = -1;
}

void TNCWiFiManager::startStationAttempt(uint8_t index)
{
    if (networkCount == 0 || index >= networkCount)
    {
        startAccessPoint();
        return;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true, true);
    WiFi.begin(networks[index].ssid.c_str(), networks[index].password.c_str());
    WiFi.setAutoReconnect(true);

    stationAttemptActive = true;
    stationAttemptIndex = index;
    stationAttemptsTried = 1;
    stationAttemptStart = millis();
    lastReconnectCheck = stationAttemptStart;
    apModeActive = false;
}

void TNCWiFiManager::advanceStationAttempt()
{
    if (networkCount == 0)
    {
        startAccessPoint();
        return;
    }

    if (stationAttemptsTried >= networkCount)
    {
        startAccessPoint();
        return;
    }

    stationAttemptIndex = (stationAttemptIndex + 1) % networkCount;
    stationAttemptsTried++;
    WiFi.disconnect(true, true);
    WiFi.begin(networks[stationAttemptIndex].ssid.c_str(), networks[stationAttemptIndex].password.c_str());
    stationAttemptStart = millis();
    apModeActive = false;
}

void TNCWiFiManager::ensureFallbackIfIdle()
{
    if (!apModeActive)
    {
        startAccessPoint();
    }
}

