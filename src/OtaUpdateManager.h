/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

struct OtaCheckResult
{
    bool ok = false;
    bool configured = false;
    bool updateAvailable = false;
    String currentVersion;
    String latestVersion;
    String assetUrl;
    String message;
};

class OtaUpdateManager
{
public:
    void begin();
    OtaCheckResult checkForUpdate();
    bool performUpdate(String &message);

    bool isConfigured() const;
    bool lastCheckOk() const;
    bool updateAvailable() const;
    String currentVersion() const;
    String latestVersion() const;
    String lastMessage() const;
    String selectedAssetUrl() const;

private:
    bool isReleaseApiConfigured() const;
    bool fetchLatestReleaseJson(String &response, String &error) const;
    bool parseLatestInfo(const String &json, String &version, String &assetUrl, String &error) const;
    int compareSemver(const String &left, const String &right) const;
    bool parseSemver(const String &value, int &major, int &minor, int &patch) const;

    bool lastCheckOkFlag = false;
    bool updateAvailableFlag = false;
    String latestVersionValue;
    String selectedAssetUrlValue;
    String lastMessageValue;
};
