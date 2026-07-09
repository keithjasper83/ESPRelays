/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class TimeSyncManager
{
public:
    void begin();
    void maintain(bool wifiConnected);
    void forceSync();

    void setEnabled(bool enabled);
    void setServer(const String &server);
    void setTimezone(const String &timezone);

    bool isEnabled() const;
    bool isTimeValid() const;
    String server() const;
    String timezone() const;
    time_t lastSyncEpoch() const;
    String lastSyncStatus() const;

private:
    void loadSettings();
    void persistSettings();
    void applyTimeConfig();
    bool checkTimeValid() const;

    String ntpServer = TIME_SERVER_DEFAULT;
    String tzRule = TIME_TZ_DEFAULT;
    bool syncEnabled = true;
    bool settingsLoaded = false;
    bool nvsReady = false;

    unsigned long lastSyncAttemptMs = 0;
    time_t lastSyncEpochValue = 0;
    String lastStatus = "not_synced";
};
