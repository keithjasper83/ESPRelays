/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "TimeSyncManager.h"

#include <Preferences.h>
#include <time.h>

#include "AppConfig.h"

namespace
{
    constexpr char TIME_PREF_NAMESPACE[] = "time_cfg";
    constexpr char TIME_PREF_SERVER[] = "server";
    constexpr char TIME_PREF_TZ[] = "tz";
    constexpr char TIME_PREF_ENABLED[] = "enabled";
}

void TimeSyncManager::loadSettings()
{
    if (settingsLoaded)
    {
        return;
    }

    Preferences preferences;
    if (!preferences.begin(TIME_PREF_NAMESPACE, false))
    {
        ntpServer = TIME_SERVER_DEFAULT;
        tzRule = TIME_TZ_DEFAULT;
        syncEnabled = true;
        nvsReady = false;
        settingsLoaded = true;
        return;
    }

    ntpServer = preferences.isKey(TIME_PREF_SERVER)
                    ? preferences.getString(TIME_PREF_SERVER, TIME_SERVER_DEFAULT)
                    : String(TIME_SERVER_DEFAULT);
    tzRule = preferences.isKey(TIME_PREF_TZ)
                 ? preferences.getString(TIME_PREF_TZ, TIME_TZ_DEFAULT)
                 : String(TIME_TZ_DEFAULT);
    syncEnabled = preferences.getBool(TIME_PREF_ENABLED, true);
    nvsReady = true;
    preferences.end();

    if (ntpServer.length() == 0)
    {
        ntpServer = TIME_SERVER_DEFAULT;
    }

    if (tzRule.length() == 0)
    {
        tzRule = TIME_TZ_DEFAULT;
    }

    settingsLoaded = true;
}

void TimeSyncManager::persistSettings()
{
    Preferences preferences;
    if (!preferences.begin(TIME_PREF_NAMESPACE, false))
    {
        nvsReady = false;
        return;
    }

    preferences.putString(TIME_PREF_SERVER, ntpServer);
    preferences.putString(TIME_PREF_TZ, tzRule);
    preferences.putBool(TIME_PREF_ENABLED, syncEnabled);
    nvsReady = true;
    preferences.end();
}

void TimeSyncManager::applyTimeConfig()
{
    configTzTime(tzRule.c_str(), ntpServer.c_str(), TIME_SERVER_FALLBACK_1, TIME_SERVER_FALLBACK_2);
}

bool TimeSyncManager::checkTimeValid() const
{
    const time_t now = time(nullptr);
    return now >= 1700000000;
}

void TimeSyncManager::begin()
{
    loadSettings();

    if (!syncEnabled)
    {
        lastStatus = "disabled";
        return;
    }

    applyTimeConfig();
    forceSync();
}

void TimeSyncManager::maintain(bool wifiConnected)
{
    loadSettings();

    if (!syncEnabled)
    {
        lastStatus = "disabled";
        return;
    }

    if (!wifiConnected)
    {
        return;
    }

    const unsigned long nowMs = millis();
    const bool valid = checkTimeValid();
    const unsigned long interval = valid ? TIME_SYNC_INTERVAL_MS : TIME_SYNC_RETRY_MS;

    if (nowMs - lastSyncAttemptMs < interval)
    {
        return;
    }

    forceSync();
}

void TimeSyncManager::forceSync()
{
    loadSettings();

    if (!syncEnabled)
    {
        lastStatus = "disabled";
        return;
    }

    applyTimeConfig();
    lastSyncAttemptMs = millis();

    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 1500))
    {
        lastSyncEpochValue = mktime(&timeInfo);
        lastStatus = "ok";
    }
    else
    {
        lastStatus = "failed";
    }
}

void TimeSyncManager::setEnabled(bool enabled)
{
    loadSettings();
    syncEnabled = enabled;
    persistSettings();

    if (syncEnabled)
    {
        applyTimeConfig();
        forceSync();
    }
    else
    {
        lastStatus = "disabled";
    }
}

void TimeSyncManager::setServer(const String &server)
{
    String value = server;
    value.trim();
    if (value.length() == 0)
    {
        return;
    }

    loadSettings();
    ntpServer = value;
    persistSettings();

    if (syncEnabled)
    {
        forceSync();
    }
}

void TimeSyncManager::setTimezone(const String &timezone)
{
    String value = timezone;
    value.trim();
    if (value.length() == 0)
    {
        return;
    }

    loadSettings();
    tzRule = value;
    persistSettings();

    if (syncEnabled)
    {
        forceSync();
    }
}

bool TimeSyncManager::isEnabled() const
{
    return syncEnabled;
}

bool TimeSyncManager::isTimeValid() const
{
    return syncEnabled && checkTimeValid();
}

String TimeSyncManager::server() const
{
    return ntpServer;
}

String TimeSyncManager::timezone() const
{
    return tzRule;
}

time_t TimeSyncManager::lastSyncEpoch() const
{
    return lastSyncEpochValue;
}

String TimeSyncManager::lastSyncStatus() const
{
    return lastStatus;
}
