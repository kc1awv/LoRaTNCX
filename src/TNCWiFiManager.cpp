#include "TNCWiFiManager.h"

#include <esp_system.h>
#include <esp_wifi.h>

TNCWiFiManager::TNCWiFiManager()
    : networkCount(0), connectedNetworkIndex(-1), apModeActive(false), stationAttemptActive(false),
      stationAttemptIndex(0), stationAttemptsTried(0), stationAttemptStart(0), lastReconnectCheck(0),
      lastWiFiReady(false)
{
}

bool TNCWiFiManager::begin()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.disconnect(true, true);

    configureDefaultAPCredentials();
    loadNetworksFromPreferences();

    // Start by attempting to connect to saved networks
    if (networkCount > 0)
    {
        startStationAttempt(0);
    }
    else
    {
        // No saved networks, start AP mode immediately
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
            // Successfully connected to station mode
            stationAttemptActive = false;
            connectedNetworkIndex = static_cast<int8_t>(stationAttemptIndex);
            stationAttemptsTried = 0;
            lastReconnectCheck = now;
            Serial.print("✓ Connected to WiFi network: ");
            Serial.println(networks[stationAttemptIndex].ssid);
        }
        else if (now - stationAttemptStart >= STA_CONNECT_TIMEOUT_MS)
        {
            // Connection attempt timed out, try next network or fall back to AP
            advanceStationAttempt();
        }
    }
    else if (!apModeActive)
    {
        // Neither STA nor AP active - this shouldn't happen, fall back to AP
        Serial.println("⚠ WiFi inactive, starting AP mode");
        startAccessPoint();
    }
    else if (connectedNetworkIndex >= 0 && WiFi.status() != WL_CONNECTED)
    {
        // We were connected but lost connection
        connectedNetworkIndex = -1;
        Serial.println("⚠ Lost WiFi connection, will retry");
        
        // Schedule reconnection attempt
        if ((now - lastReconnectCheck) >= RECONNECT_INTERVAL_MS)
        {
            lastReconnectCheck = now;
            if (networkCount > 0)
            {
                startStationAttempt(0);
            }
        }
    }
    else if (networkCount > 0 && apModeActive && connectedNetworkIndex < 0 && 
             (now - lastReconnectCheck) >= RECONNECT_INTERVAL_MS)
    {
        // We're in AP mode but have networks to try connecting to
        lastReconnectCheck = now;
        startStationAttempt(0);
    }

    // Determine current WiFi ready state
    bool currentWiFiReady = apModeActive || (WiFi.status() == WL_CONNECTED);
    
    if (currentWiFiReady != lastWiFiReady)
    {
        lastWiFiReady = currentWiFiReady;
        if (stateChangeCallback)
        {
            stateChangeCallback(currentWiFiReady);
        }
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

    // In dual mode, show both AP and STA status
    if (apModeActive)
    {
        status = "AP: " + apSSID + " (" + WiFi.softAPIP().toString() + ")";
    }

    if ((mode & WIFI_MODE_STA) && wifiStatus == WL_CONNECTED)
    {
        if (!status.isEmpty()) status += " | ";
        status += "STA: " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
    }
    else if (stationAttemptActive && stationAttemptIndex < networkCount)
    {
        if (!status.isEmpty()) status += " | ";
        status += "STA: Connecting to " + networks[stationAttemptIndex].ssid + "...";
    }
    else if (networkCount > 0)
    {
        if (!status.isEmpty()) status += " | ";
        status += "STA: Disconnected";
    }

    if (!status.isEmpty()) status += " | ";
    status += "Networks: " + String(networkCount);
    return status;
}

TNCWiFiManager::StatusInfo TNCWiFiManager::getStatusInfo() const
{
    StatusInfo info;

    wifi_mode_t mode = WiFi.getMode();
    
    // In dual mode, AP should always be active
    info.apActive = apModeActive && (mode & WIFI_MODE_AP);
    info.stationActive = (mode & WIFI_MODE_STA) != 0;
    info.stationAttemptActive = stationAttemptActive;

    wl_status_t status = WiFi.status();
    info.stationConnected = info.stationActive && status == WL_CONNECTED;

    // AP info (should always be available in dual mode)
    if (info.apActive)
    {
        info.apSSID = apSSID;
        info.apIP = WiFi.softAPIP();
    }

    // STA info
    if (info.stationConnected)
    {
        info.stationSSID = WiFi.SSID();
        info.stationIP = WiFi.localIP();
    }
    else if (info.stationAttemptActive && stationAttemptIndex < networkCount)
    {
        info.stationSSID = networks[stationAttemptIndex].ssid;
    }
    else if (connectedNetworkIndex >= 0 && connectedNetworkIndex < networkCount)
    {
        info.stationSSID = networks[connectedNetworkIndex].ssid;
    }

    return info;
}

String TNCWiFiManager::getAPPassword() const
{
    if (apModeActive)
    {
        return apPassword;
    }
    return String();
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
    WiFi.macAddress(mac);

    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);

    apSSID = "LoRaTNCX-" + String(suffix);
    // Password will be generated when AP mode starts
}

void TNCWiFiManager::generateRandomAPPassword()
{
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetSize = sizeof(charset) - 1; // Exclude null terminator
    
    apPassword = "";
    for (int i = 0; i < 8; i++)
    {
        apPassword += charset[random(charsetSize)];
    }
}



void TNCWiFiManager::startAccessPoint()
{
    generateRandomAPPassword();

    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    apModeActive = true;
    stationAttemptActive = false;
    connectedNetworkIndex = -1;

    Serial.println("✓ Access Point started");
    Serial.print("✓ SSID: ");
    Serial.print(apSSID);
    Serial.print(" (Password: ");
    Serial.print(apPassword);
    Serial.println(")");
}

void TNCWiFiManager::startStationAttempt(uint8_t index)
{
    if (networkCount == 0 || index >= networkCount)
    {
        // No networks to try, fall back to AP mode
        if (!apModeActive)
        {
            startAccessPoint();
        }
        return;
    }

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true, true);
    WiFi.begin(networks[index].ssid.c_str(), networks[index].password.c_str());
    WiFi.setAutoReconnect(true);

    stationAttemptActive = true;
    stationAttemptIndex = index;
    stationAttemptsTried = 1;
    stationAttemptStart = millis();
    lastReconnectCheck = stationAttemptStart;
    
    Serial.print("Attempting to connect to: ");
    Serial.println(networks[index].ssid);
}

void TNCWiFiManager::advanceStationAttempt()
{
    if (networkCount == 0)
    {
        // No networks to try, fall back to AP mode
        stationAttemptActive = false;
        if (!apModeActive)
        {
            startAccessPoint();
        }
        return;
    }

    if (stationAttemptsTried >= networkCount)
    {
        // Tried all networks, fall back to AP mode
        stationAttemptActive = false;
        Serial.println("Failed to connect to any saved network, starting AP mode");
        startAccessPoint();
        return;
    }

    // Try next network
    stationAttemptIndex = (stationAttemptIndex + 1) % networkCount;
    stationAttemptsTried++;
    WiFi.disconnect(true, true);
    WiFi.begin(networks[stationAttemptIndex].ssid.c_str(), networks[stationAttemptIndex].password.c_str());
    stationAttemptStart = millis();
    
    Serial.print("Trying next network: ");
    Serial.println(networks[stationAttemptIndex].ssid);
}

void TNCWiFiManager::ensureFallbackIfIdle()
{
    // If neither STA nor AP mode is active, start AP mode
    if (!apModeActive && !stationAttemptActive && WiFi.status() != WL_CONNECTED)
    {
        startAccessPoint();
    }
}

void TNCWiFiManager::setStateChangeCallback(std::function<void(bool)> callback)
{
    stateChangeCallback = callback;
}

