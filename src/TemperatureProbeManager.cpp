/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "TemperatureProbeManager.h"

#include <math.h>
#include <Preferences.h>

namespace
{
    constexpr char TEMP_PREF_NAMESPACE[] = "temp_probe";
    constexpr char TEMP_PREF_LOW_VALID[] = "low_valid";
    constexpr char TEMP_PREF_LOW_RAW[] = "low_raw";
    constexpr char TEMP_PREF_LOW_TEMP[] = "low_temp";
    constexpr char TEMP_PREF_HIGH_VALID[] = "high_valid";
    constexpr char TEMP_PREF_HIGH_RAW[] = "high_raw";
    constexpr char TEMP_PREF_HIGH_TEMP[] = "high_temp";

    bool isProbeInValidRange(int rawValue)
    {
        return rawValue > TEMP_PROBE_PRESENT_MIN_RAW && rawValue < TEMP_PROBE_PRESENT_MAX_RAW;
    }
}

void TemperatureProbeManager::loadCalibration()
{
    if (calibrationLoaded)
    {
        return;
    }

    Preferences preferences;
    if (!preferences.begin(TEMP_PREF_NAMESPACE, false))
    {
        calibrationLoaded = true;
        return;
    }

    lowPoint.valid = preferences.getBool(TEMP_PREF_LOW_VALID, false);
    lowPoint.raw = preferences.getInt(TEMP_PREF_LOW_RAW, -1);
    lowPoint.tempC = preferences.getFloat(TEMP_PREF_LOW_TEMP, NAN);

    highPoint.valid = preferences.getBool(TEMP_PREF_HIGH_VALID, false);
    highPoint.raw = preferences.getInt(TEMP_PREF_HIGH_RAW, -1);
    highPoint.tempC = preferences.getFloat(TEMP_PREF_HIGH_TEMP, NAN);

    preferences.end();
    calibrationLoaded = true;
}

bool TemperatureProbeManager::persistCalibration()
{
    Preferences preferences;
    if (!preferences.begin(TEMP_PREF_NAMESPACE, false))
    {
        return false;
    }

    preferences.putBool(TEMP_PREF_LOW_VALID, lowPoint.valid);
    preferences.putInt(TEMP_PREF_LOW_RAW, lowPoint.raw);
    preferences.putFloat(TEMP_PREF_LOW_TEMP, lowPoint.tempC);

    preferences.putBool(TEMP_PREF_HIGH_VALID, highPoint.valid);
    preferences.putInt(TEMP_PREF_HIGH_RAW, highPoint.raw);
    preferences.putFloat(TEMP_PREF_HIGH_TEMP, highPoint.tempC);
    preferences.end();
    return true;
}

bool TemperatureProbeManager::hasValidReadingForCapture(String &error) const
{
    if (!probePresent)
    {
        error = "Temperature probe not detected";
        return false;
    }

    if (savedCurrentTemperatureRaw < 0)
    {
        error = "No valid probe sample available";
        return false;
    }

    return true;
}

float TemperatureProbeManager::calculateTemperatureC(int raw) const
{
    if (!calibrationReady())
    {
        return NAN;
    }

    const float lowRaw = static_cast<float>(lowPoint.raw);
    const float highRaw = static_cast<float>(highPoint.raw);
    if (highRaw == lowRaw)
    {
        return NAN;
    }

    const float ratio = (static_cast<float>(raw) - lowRaw) / (highRaw - lowRaw);
    return lowPoint.tempC + ratio * (highPoint.tempC - lowPoint.tempC);
}

void TemperatureProbeManager::begin()
{
    loadCalibration();
    pinMode(TEMP_PROBE_ADC_PIN, INPUT);
    analogReadResolution(12);
    maintain(millis());
}

void TemperatureProbeManager::maintain(unsigned long nowMs)
{
    if ((nowMs - lastSampleAtMs) < TEMP_PROBE_SAMPLE_INTERVAL_MS && lastSampleAtMs != 0)
    {
        return;
    }

    lastSampleAtMs = nowMs;

    const int raw = analogRead(TEMP_PROBE_ADC_PIN);
    lastRawReading = raw;

    probePresent = isProbeInValidRange(raw);
    if (probePresent)
    {
        savedCurrentTemperatureRaw = raw;
        return;
    }

    savedCurrentTemperatureRaw = -1;
}

bool TemperatureProbeManager::isPresent() const
{
    return probePresent;
}

int TemperatureProbeManager::rawReading() const
{
    return lastRawReading;
}

int TemperatureProbeManager::currentTemperatureRaw() const
{
    return savedCurrentTemperatureRaw;
}

float TemperatureProbeManager::currentTemperatureC() const
{
    if (savedCurrentTemperatureRaw < 0)
    {
        return NAN;
    }

    return calculateTemperatureC(savedCurrentTemperatureRaw);
}

bool TemperatureProbeManager::calibrationReady() const
{
    return lowPoint.valid && highPoint.valid && lowPoint.raw != highPoint.raw;
}

bool TemperatureProbeManager::lowPointValid() const
{
    return lowPoint.valid;
}

bool TemperatureProbeManager::highPointValid() const
{
    return highPoint.valid;
}

int TemperatureProbeManager::lowPointRaw() const
{
    return lowPoint.raw;
}

int TemperatureProbeManager::highPointRaw() const
{
    return highPoint.raw;
}

float TemperatureProbeManager::lowPointTempC() const
{
    return lowPoint.tempC;
}

float TemperatureProbeManager::highPointTempC() const
{
    return highPoint.tempC;
}

bool TemperatureProbeManager::captureLow(float knownTempC, String &error)
{
    loadCalibration();
    if (!hasValidReadingForCapture(error))
    {
        return false;
    }

    lowPoint.valid = true;
    lowPoint.raw = savedCurrentTemperatureRaw;
    lowPoint.tempC = knownTempC;
    if (!persistCalibration())
    {
        error = "Failed to persist low calibration point";
        return false;
    }

    return true;
}

bool TemperatureProbeManager::captureHigh(float knownTempC, String &error)
{
    loadCalibration();
    if (!hasValidReadingForCapture(error))
    {
        return false;
    }

    highPoint.valid = true;
    highPoint.raw = savedCurrentTemperatureRaw;
    highPoint.tempC = knownTempC;
    if (!persistCalibration())
    {
        error = "Failed to persist high calibration point";
        return false;
    }

    return true;
}

bool TemperatureProbeManager::captureLowUsingSavedTemp(String &error)
{
    loadCalibration();
    if (!lowPoint.valid || isnan(lowPoint.tempC))
    {
        error = "Low calibration temperature is not set";
        return false;
    }

    return captureLow(lowPoint.tempC, error);
}

bool TemperatureProbeManager::captureHighUsingSavedTemp(String &error)
{
    loadCalibration();
    if (!highPoint.valid || isnan(highPoint.tempC))
    {
        error = "High calibration temperature is not set";
        return false;
    }

    return captureHigh(highPoint.tempC, error);
}

bool TemperatureProbeManager::resetCalibration(String &error)
{
    (void)error;
    loadCalibration();
    lowPoint = {};
    highPoint = {};
    return persistCalibration();
}

bool TemperatureProbeManager::shouldRunTemperatureDependentFunctions() const
{
    return probePresent;
}
