/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "AppConfig.h"

class WiFiManager
{
public:
    void begin();
    void maintain();
    void forceReconnect();
    void setCredentials(const String &ssid, const String &password);
    void resetCredentialsToDefaults();
    String ssid() const;
    bool nvsReady() const;
    void printFullDetails() const;
    void scanWifi() const;
    String scanWifiJson() const;
    bool isConnected() const;
    wl_status_t status() const;

    static const char *statusName(wl_status_t status);

private:
    void loadCredentials();

    String wifiSsid = WIFI_SSID;
    String wifiPassword = WIFI_PASS;
    bool credentialsLoaded = false;
    bool nvsReadyFlag = false;
    unsigned long lastWifiStatusPrint = 0;
    unsigned long lastWifiRetry = 0;
    int lastStatus = -999;
};