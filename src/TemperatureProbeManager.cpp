/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "TemperatureProbeManager.h"

#include <math.h>
#include <Preferences.h>

#include "CalibrationRecordStorage.h"
#include "TemperatureCalibrationRecord.h"

namespace
{
    constexpr char TEMP_PREF_NAMESPACE[] = "temp_probe";
    constexpr char TEMP_PREF_LOW_VALID[] = "low_valid";
    constexpr char TEMP_PREF_LOW_RAW[] = "low_raw";
    constexpr char TEMP_PREF_LOW_TEMP[] = "low_temp";
    constexpr char TEMP_PREF_HIGH_VALID[] = "high_valid";
    constexpr char TEMP_PREF_HIGH_RAW[] = "high_raw";
    constexpr char TEMP_PREF_HIGH_TEMP[] = "high_temp";
    constexpr char TEMP_PREF_TRIM_OFFSET[] = "trim_ofs";
    constexpr char TEMP_PREF_ENABLED[] = "enabled";
    constexpr char TEMP_PREF_RECORD_V2[] = "cal_v2";
    constexpr char TEMP_PREF_RECORD_A[] = "cal_a";
    constexpr char TEMP_PREF_RECORD_B[] = "cal_b";

    class PreferencesCalibrationBlobStorage final : public CalibrationBlobStorage
    {
    public:
        explicit PreferencesCalibrationBlobStorage(Preferences &preferences) : preferences(preferences) {}

        size_t length(const char *key) override { return preferences.getBytesLength(key); }
        size_t read(const char *key, void *value, size_t valueLength) override
        {
            return preferences.getBytes(key, value, valueLength);
        }
        size_t write(const char *key, const void *value, size_t valueLength) override
        {
            return preferences.putBytes(key, value, valueLength);
        }

    private:
        Preferences &preferences;
    };
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
        storageSlot = "none";
        storageLoadSucceeded = false;
        calibrationLoaded = true;
        return;
    }

    PreferencesCalibrationBlobStorage storage(preferences);
    SelectedCalibrationRecord selected = selectNewestCalibration(
        storage, TEMP_PREF_RECORD_A, TEMP_PREF_RECORD_B);
    if (!selected.found)
    {
        selected = selectNewestCalibration(storage, TEMP_PREF_RECORD_V2, TEMP_PREF_RECORD_V2);
    }
    if (selected.found)
    {
        const TemperatureCalibrationRecord &record = selected.record;
        lowPoint.valid = (record.flags & TEMPERATURE_CALIBRATION_LOW_VALID) != 0;
        lowPoint.raw = record.lowRaw;
        lowPoint.tempC = lowPoint.valid ? record.lowTempC : NAN;
        highPoint.valid = (record.flags & TEMPERATURE_CALIBRATION_HIGH_VALID) != 0;
        highPoint.raw = record.highRaw;
        highPoint.tempC = highPoint.valid ? record.highTempC : NAN;
        trimOffset = record.trimOffsetC;
        enabled = (record.flags & TEMPERATURE_MONITORING_ENABLED) != 0;
        calibrationGeneration = record.reserved;
        storageLoadSucceeded = true;
        storageSlot = selected.key;
        preferences.end();
        calibrationLoaded = true;
        if (storageSlot == TEMP_PREF_RECORD_V2) persistCalibration();
        return;
    }

    lowPoint.valid = preferences.getBool(TEMP_PREF_LOW_VALID, false);
    lowPoint.raw = preferences.getInt(TEMP_PREF_LOW_RAW, -1);
    lowPoint.tempC = preferences.getFloat(TEMP_PREF_LOW_TEMP, NAN);

    highPoint.valid = preferences.getBool(TEMP_PREF_HIGH_VALID, false);
    highPoint.raw = preferences.getInt(TEMP_PREF_HIGH_RAW, -1);
    highPoint.tempC = preferences.getFloat(TEMP_PREF_HIGH_TEMP, NAN);
    trimOffset = preferences.getFloat(TEMP_PREF_TRIM_OFFSET, 0.0f);
    enabled = preferences.getBool(TEMP_PREF_ENABLED, true);

    preferences.end();
    storageLoadSucceeded = false;
    storageSlot = "legacy_keys";
    calibrationLoaded = true;
    // Migrate legacy keys without changing their values. The legacy mirror is
    // retained on future writes so downgrades remain safe.
    persistCalibration();
}

bool TemperatureProbeManager::persistCalibration()
{
    Preferences preferences;
    if (!preferences.begin(TEMP_PREF_NAMESPACE, false))
    {
        lastStorageWriteWasVerified = false;
        return false;
    }

    PreferencesCalibrationBlobStorage storage(preferences);
    const SelectedCalibrationRecord selected = selectNewestCalibration(
        storage, TEMP_PREF_RECORD_A, TEMP_PREF_RECORD_B);
    const uint8_t nextGeneration = static_cast<uint8_t>(
        (selected.found ? selected.record.reserved : calibrationGeneration) + 1U);
    const TemperatureCalibrationRecord record = makeTemperatureCalibrationRecord(
        lowPoint.valid, lowPoint.raw, lowPoint.tempC,
        highPoint.valid, highPoint.raw, highPoint.tempC,
        trimOffset, enabled, nextGeneration);
    const char *target = !selected.found || selected.key == TEMP_PREF_RECORD_B
                             ? TEMP_PREF_RECORD_A : TEMP_PREF_RECORD_B;
    const bool recordSaved = writeVerifiedCalibration(storage, target, record);
    lastStorageWriteWasVerified = recordSaved;
    if (recordSaved)
    {
        calibrationGeneration = nextGeneration;
        preferences.putBool(TEMP_PREF_LOW_VALID, lowPoint.valid);
        preferences.putInt(TEMP_PREF_LOW_RAW, lowPoint.raw);
        preferences.putFloat(TEMP_PREF_LOW_TEMP, lowPoint.tempC);
        preferences.putBool(TEMP_PREF_HIGH_VALID, highPoint.valid);
        preferences.putInt(TEMP_PREF_HIGH_RAW, highPoint.raw);
        preferences.putFloat(TEMP_PREF_HIGH_TEMP, highPoint.tempC);
        preferences.putFloat(TEMP_PREF_TRIM_OFFSET, trimOffset);
        preferences.putBool(TEMP_PREF_ENABLED, enabled);
        // Retain a single-slot v2 mirror for downgrade compatibility. Recovery
        // always prefers the independently checksummed A/B records.
        preferences.putBytes(TEMP_PREF_RECORD_V2, &record, sizeof(record));
    }
    preferences.end();
    return recordSaved;
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
    return lowPoint.tempC + ratio * (highPoint.tempC - lowPoint.tempC) + trimOffset;
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
    if (!enabled)
    {
        probePresenceFilter.reset();
        probePresent = false;
        lastRawReading = -1;
        savedCurrentTemperatureRaw = -1;
        return;
    }

    if ((nowMs - lastSampleAtMs) < TEMP_PROBE_SAMPLE_INTERVAL_MS && lastSampleAtMs != 0)
    {
        return;
    }

    lastSampleAtMs = nowMs;

    const int raw = analogRead(TEMP_PROBE_ADC_PIN);
    lastRawReading = raw;

    probePresent = probePresenceFilter.update(raw, enabled);
    savedCurrentTemperatureRaw = probePresenceFilter.stableRaw();
}

bool TemperatureProbeManager::isPresent() const
{
    return probePresent;
}

bool TemperatureProbeManager::isEnabled() const
{
    return enabled;
}

bool TemperatureProbeManager::setEnabled(bool newEnabled, String &error)
{
    loadCalibration();
    const bool previousEnabled = enabled;
    const uint8_t previousGeneration = calibrationGeneration;
    const bool previousProbePresent = probePresent;
    const int previousLastRawReading = lastRawReading;
    const int previousSavedCurrentTemperatureRaw = savedCurrentTemperatureRaw;
    const unsigned long previousLastSampleAtMs = lastSampleAtMs;
    const ProbePresenceFilter previousProbePresenceFilter = probePresenceFilter;
    enabled = newEnabled;
    probePresent = false;
    lastRawReading = -1;
    savedCurrentTemperatureRaw = -1;
    probePresenceFilter.reset();
    lastSampleAtMs = 0;
    if (!persistCalibration())
    {
        enabled = previousEnabled;
        calibrationGeneration = previousGeneration;
        probePresent = previousProbePresent;
        lastRawReading = previousLastRawReading;
        savedCurrentTemperatureRaw = previousSavedCurrentTemperatureRaw;
        lastSampleAtMs = previousLastSampleAtMs;
        probePresenceFilter = previousProbePresenceFilter;
        error = "Failed to save temperature monitoring setting";
        return false;
    }
    return true;
}

int TemperatureProbeManager::rawReading() const
{
    return lastRawReading;
}

int TemperatureProbeManager::currentTemperatureRaw() const
{
    return probePresenceFilter.stableRaw();
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
    return lowPoint.valid && highPoint.valid && temperatureCalibrationPairValid(
        lowPoint.raw, lowPoint.tempC, highPoint.raw, highPoint.tempC);
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

float TemperatureProbeManager::trimOffsetC() const
{
    return trimOffset;
}

bool TemperatureProbeManager::storageLoadOk() const
{
    return storageLoadSucceeded;
}

bool TemperatureProbeManager::lastStorageWriteVerified() const
{
    return lastStorageWriteWasVerified;
}

const char *TemperatureProbeManager::selectedStorageSlot() const
{
    return storageSlot;
}

uint8_t TemperatureProbeManager::calibrationGenerationValue() const
{
    return calibrationGeneration;
}

bool TemperatureProbeManager::captureLow(float knownTempC, String &error)
{
    loadCalibration();
    if (!isfinite(knownTempC) || knownTempC < -100.0f || knownTempC > 200.0f)
    {
        error = "Low calibration temperature must be between -100 and 200 C";
        return false;
    }
    if (!hasValidReadingForCapture(error))
    {
        return false;
    }

    if (highPoint.valid && !temperatureCalibrationPairValid(
            savedCurrentTemperatureRaw, knownTempC, highPoint.raw, highPoint.tempC))
    {
        error = "Low reference must be colder than, and have a different ADC value from, the high reference";
        return false;
    }

    const CalibrationPoint previousLow = lowPoint;
    const uint8_t previousGeneration = calibrationGeneration;
    lowPoint.valid = true;
    lowPoint.raw = savedCurrentTemperatureRaw;
    lowPoint.tempC = knownTempC;
    if (!persistCalibration())
    {
        lowPoint = previousLow;
        calibrationGeneration = previousGeneration;
        error = "Failed to persist low calibration point";
        return false;
    }

    return true;
}

bool TemperatureProbeManager::captureHigh(float knownTempC, String &error)
{
    loadCalibration();
    if (!isfinite(knownTempC) || knownTempC < -100.0f || knownTempC > 200.0f)
    {
        error = "High calibration temperature must be between -100 and 200 C";
        return false;
    }
    if (!hasValidReadingForCapture(error))
    {
        return false;
    }

    if (lowPoint.valid && !temperatureCalibrationPairValid(
            lowPoint.raw, lowPoint.tempC, savedCurrentTemperatureRaw, knownTempC))
    {
        error = "High reference must be warmer than, and have a different ADC value from, the low reference";
        return false;
    }

    const CalibrationPoint previousHigh = highPoint;
    const uint8_t previousGeneration = calibrationGeneration;
    highPoint.valid = true;
    highPoint.raw = savedCurrentTemperatureRaw;
    highPoint.tempC = knownTempC;
    if (!persistCalibration())
    {
        highPoint = previousHigh;
        calibrationGeneration = previousGeneration;
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
    loadCalibration();
    const CalibrationPoint previousLow = lowPoint;
    const CalibrationPoint previousHigh = highPoint;
    const uint8_t previousGeneration = calibrationGeneration;
    lowPoint = {};
    highPoint = {};
    if (!persistCalibration())
    {
        lowPoint = previousLow;
        highPoint = previousHigh;
        calibrationGeneration = previousGeneration;
        error = "Failed to persist calibration reset";
        return false;
    }
    return true;
}

bool TemperatureProbeManager::setTrimOffsetC(float offsetC, String &error)
{
    loadCalibration();

    if (isnan(offsetC) || offsetC < -20.0f || offsetC > 20.0f)
    {
        error = "Trim offset must be between -20.0 and 20.0 C";
        return false;
    }

    const float previousTrim = trimOffset;
    const uint8_t previousGeneration = calibrationGeneration;
    trimOffset = offsetC;
    if (!persistCalibration())
    {
        trimOffset = previousTrim;
        calibrationGeneration = previousGeneration;
        error = "Failed to persist trim offset";
        return false;
    }

    return true;
}

bool TemperatureProbeManager::restoreCalibration(
    const bool lowValid, const int lowRaw, const float lowTempC,
    const bool highValid, const int highRaw, const float highTempC,
    const float newTrimOffsetC, const bool monitoringEnabled, String &error)
{
    loadCalibration();
    if (!lowValid || !highValid)
    {
        error = "A restore requires both calibration references";
        return false;
    }
    if (!temperatureCalibrationPairValid(lowRaw, lowTempC, highRaw, highTempC))
    {
        error = "Calibration requires distinct ADC values and an ordered low-to-high temperature pair from -100 to 200 C";
        return false;
    }
    if (!isfinite(newTrimOffsetC) || newTrimOffsetC < -20.0f || newTrimOffsetC > 20.0f)
    {
        error = "Trim offset must be between -20 and 20 C";
        return false;
    }

    const CalibrationPoint previousLow = lowPoint;
    const CalibrationPoint previousHigh = highPoint;
    const float previousTrim = trimOffset;
    const bool previousEnabled = enabled;
    const uint8_t previousGeneration = calibrationGeneration;
    lowPoint.valid = true;
    lowPoint.raw = lowRaw;
    lowPoint.tempC = lowTempC;
    highPoint.valid = true;
    highPoint.raw = highRaw;
    highPoint.tempC = highTempC;
    trimOffset = newTrimOffsetC;
    enabled = monitoringEnabled;
    if (!persistCalibration())
    {
        lowPoint = previousLow;
        highPoint = previousHigh;
        trimOffset = previousTrim;
        enabled = previousEnabled;
        calibrationGeneration = previousGeneration;
        error = "Failed to persist restored calibration";
        return false;
    }
    return true;
}

bool TemperatureProbeManager::shouldRunTemperatureDependentFunctions() const
{
    return enabled && probePresent;
}
