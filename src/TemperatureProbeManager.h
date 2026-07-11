/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <math.h>

#include "AppConfig.h"

class TemperatureProbeManager
{
public:
    void begin();
    void maintain(unsigned long nowMs);

    bool isPresent() const;
    int rawReading() const;
    int currentTemperatureRaw() const;
    float currentTemperatureC() const;

    bool calibrationReady() const;
    bool lowPointValid() const;
    bool highPointValid() const;
    int lowPointRaw() const;
    int highPointRaw() const;
    float lowPointTempC() const;
    float highPointTempC() const;

    bool captureLow(float knownTempC, String &error);
    bool captureHigh(float knownTempC, String &error);
    bool captureLowUsingSavedTemp(String &error);
    bool captureHighUsingSavedTemp(String &error);
    bool resetCalibration(String &error);

    bool shouldRunTemperatureDependentFunctions() const;

private:
    struct CalibrationPoint
    {
        bool valid = false;
        int raw = -1;
        float tempC = NAN;
    };

    void loadCalibration();
    bool persistCalibration();
    bool hasValidReadingForCapture(String &error) const;
    float calculateTemperatureC(int raw) const;

    int lastRawReading = -1;
    int savedCurrentTemperatureRaw = -1;
    bool probePresent = false;
    unsigned long lastSampleAtMs = 0;
    bool calibrationLoaded = false;
    CalibrationPoint lowPoint;
    CalibrationPoint highPoint;
};
