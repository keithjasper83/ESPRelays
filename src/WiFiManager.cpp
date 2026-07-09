/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "WiFiManager.h"

#include <esp_wifi.h>
#include <Preferences.h>

#include "AppConfig.h"

extern bool debugLogging;

namespace
{
    constexpr char WIFI_PREF_NAMESPACE[] = "wifi_cfg";
    constexpr char WIFI_PREF_SSID[] = "ssid";
    constexpr char WIFI_PREF_PASSWORD[] = "pass";
}

namespace
{
    String jsonEscape(const String &input)
    {
        String escaped;
        escaped.reserve(input.length() + 8);

        for (size_t i = 0; i < input.length(); i++)
        {
            const char c = input[i];
            if (c == '\\')
            {
                escaped += "\\\\";
            }
            else if (c == '"')
            {
                escaped += "\\\"";
            }
            else if (c == '\n')
            {
                escaped += "\\n";
            }
            else if (c == '\r')
            {
                escaped += "\\r";
            }
            else
            {
                escaped += c;
            }
        }

        return escaped;
    }
}

const char *WiFiManager::statusName(wl_status_t status)
{
    switch (status)
    {
    case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
        return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}

bool WiFiManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

wl_status_t WiFiManager::status() const
{
    return WiFi.status();
}

void WiFiManager::begin()
{
    loadCredentials();

    if (debugLogging)
    {
        Serial.println();
        Serial.println("========== WIFI START ==========");
        Serial.print("Connecting to SSID: ");
        Serial.println(wifiSsid);
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

    if (debugLogging)
    {
        Serial.println("WiFi.begin() called.");
        Serial.println("Use serial command: wifi");
        Serial.println("Use serial command: scan");
        Serial.println("================================");
    }
}

void WiFiManager::printFullDetails() const
{
    if (!debugLogging)
    {
        Serial.println("Debug is OFF. Fit jumper GPIO4 -> GND and reboot for full WiFi details.");
        return;
    }

    Serial.println();
    Serial.println("========== WIFI DETAILS ==========");
    Serial.print("Target SSID: ");
    Serial.println(wifiSsid);
    Serial.print("Status code: ");
    Serial.println(WiFi.status());
    Serial.print("Status text: ");
    Serial.println(statusName(WiFi.status()));
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
    Serial.print("STA MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Connected SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("BSSID: ");
    Serial.println(WiFi.BSSIDstr());
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(WiFi.dnsIP(1));

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    Serial.print("ESP WiFi mode: ");
    Serial.println((int)mode);
    Serial.println("==================================");
}

void WiFiManager::scanWifi() const
{
    if (!debugLogging)
    {
        Serial.println("Debug is OFF. Fit jumper GPIO4 -> GND and reboot for WiFi scan.");
        return;
    }

    Serial.println();
    Serial.println("==================================================");
    Serial.println("FULL WIFI NETWORK SCAN");
    Serial.println("Listing every SSID visible to this ESP32-C3");
    Serial.println("==================================================");

    WiFi.mode(WIFI_STA);
    delay(200);

    const int count = WiFi.scanNetworks(false, true);

    Serial.print("Total networks found: ");
    Serial.println(count);

    if (count <= 0)
    {
        Serial.println("No WiFi networks found.");
        Serial.println("ESP32-C3 only sees 2.4GHz WiFi.");
        Serial.println("5GHz-only networks will not appear.");
        Serial.println("Hidden SSIDs may show blank.");
        Serial.println("==================================================");
        return;
    }

    Serial.println();

    for (int i = 0; i < count; i++)
    {
        const String ssid = WiFi.SSID(i);

        Serial.print("#");
        Serial.println(i + 1);
        Serial.print("SSID: ");
        Serial.println(ssid.length() == 0 ? "<hidden / blank>" : ssid);
        Serial.print("BSSID: ");
        Serial.println(WiFi.BSSIDstr(i));
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.println(" dBm");
        Serial.print("Channel: ");
        Serial.println(WiFi.channel(i));
        Serial.print("Encryption code: ");
        Serial.println((int)WiFi.encryptionType(i));
        Serial.println("--------------------------------");
    }

    WiFi.scanDelete();

    Serial.println("END WIFI SCAN");
    Serial.println("==================================================");
}

String WiFiManager::scanWifiJson() const
{
    WiFi.mode(WIFI_STA);
    delay(200);

    const int count = WiFi.scanNetworks(false, true);

    String json = "{\"ok\":true,\"count\":";
    json += count;
    json += ",\"networks\":";
    json += "[";

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            json += ",";
        }

        const String ssid = WiFi.SSID(i);
        json += "{";
        json += "\"ssid\":\"";
        json += jsonEscape(ssid);
        json += "\",";
        json += "\"bssid\":\"";
        json += jsonEscape(WiFi.BSSIDstr(i));
        json += "\",";
        json += "\"rssi\":";
        json += WiFi.RSSI(i);
        json += ",\"channel\":";
        json += WiFi.channel(i);
        json += ",\"encryption\":";
        json += (int)WiFi.encryptionType(i);
        json += "}";
    }

    json += "]}";

    WiFi.scanDelete();
    return json;
}

void WiFiManager::forceReconnect()
{
    Serial.println("Forcing WiFi reconnect...");
    WiFi.disconnect(true);
    delay(500);
    loadCredentials();
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
}

void WiFiManager::maintain()
{
    const wl_status_t currentStatus = WiFi.status();
    const unsigned long now = millis();

    if (debugLogging && (int)currentStatus != lastStatus)
    {
        lastStatus = (int)currentStatus;

        Serial.println();
        Serial.println("***** WIFI STATUS CHANGED *****");
        Serial.print("New status code: ");
        Serial.println((int)currentStatus);
        Serial.print("New status text: ");
        Serial.println(statusName(currentStatus));

        if (currentStatus == WL_CONNECTED)
        {
            Serial.println("WiFi connected.");
            printFullDetails();
        }

        Serial.println("*******************************");
    }

    if (debugLogging && now - lastWifiStatusPrint >= 3000)
    {
        lastWifiStatusPrint = now;

        Serial.print("WiFi trying... status=");
        Serial.print((int)currentStatus);
        Serial.print(" ");
        Serial.print(statusName(currentStatus));
        Serial.print(" | target SSID=");
        Serial.print(WIFI_SSID);
        Serial.print(" | connected SSID=");
        Serial.print(WiFi.SSID());
        Serial.print(" | IP=");
        Serial.print(WiFi.localIP());
        Serial.print(" | RSSI=");
        Serial.print(WiFi.RSSI());
        Serial.print(" | CH=");
        Serial.print(WiFi.channel());
        Serial.print(" | MAC=");
        Serial.println(WiFi.macAddress());
    }

    if (currentStatus == WL_CONNECTED)
    {
        return;
    }

    if (now - lastWifiRetry >= 15000)
    {
        lastWifiRetry = now;

        if (debugLogging)
        {
            Serial.println();
            Serial.println("Retrying WiFi connection...");
            Serial.print("Current status before retry: ");
            Serial.print((int)WiFi.status());
            Serial.print(" ");
            Serial.println(statusName(WiFi.status()));
        }

        WiFi.disconnect(false);
        delay(200);
        WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

        if (debugLogging)
        {
            Serial.println("WiFi.begin() called again.");
        }
    }
}

void WiFiManager::loadCredentials()
{
    if (credentialsLoaded)
    {
        return;
    }

    Preferences preferences;
    if (!preferences.begin(WIFI_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: Wi-Fi preferences unavailable; using compiled defaults.");
        wifiSsid = WIFI_SSID;
        wifiPassword = WIFI_PASS;
        nvsReadyFlag = false;
        credentialsLoaded = true;
        return;
    }

    wifiSsid = preferences.getString(WIFI_PREF_SSID, WIFI_SSID);
    wifiPassword = preferences.getString(WIFI_PREF_PASSWORD, WIFI_PASS);
    nvsReadyFlag = true;
    preferences.end();

    if (wifiSsid.length() == 0)
    {
        wifiSsid = WIFI_SSID;
    }

    credentialsLoaded = true;
}

void WiFiManager::setCredentials(const String &ssid, const String &password)
{
    String cleanSsid = ssid;
    cleanSsid.trim();

    if (cleanSsid.length() == 0)
    {
        return;
    }

    loadCredentials();
    wifiSsid = cleanSsid;

    if (password.length() > 0)
    {
        wifiPassword = password;
    }

    Preferences preferences;
    if (!preferences.begin(WIFI_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save Wi-Fi credentials.");
        nvsReadyFlag = false;
        return;
    }

    preferences.putString(WIFI_PREF_SSID, wifiSsid);
    preferences.putString(WIFI_PREF_PASSWORD, wifiPassword);
    nvsReadyFlag = true;
    preferences.end();
}

String WiFiManager::ssid() const
{
    return wifiSsid;
}

bool WiFiManager::nvsReady() const
{
    return nvsReadyFlag;
}