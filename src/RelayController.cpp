/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "RelayController.h"

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

#include "AppConfig.h"

namespace
{
    constexpr char RELAY_PREF_NAMESPACE[] = "relay_cfg";
    constexpr char RELAY_PREF_STATE[] = "state";
    constexpr char RELAY_PREF_AUTO_OFF_MINUTES[] = "auto_off_min";
    constexpr char RELAY_PREF_AUTO_OFF_ARMED[] = "auto_off_armed";
    constexpr char RELAY_PREF_AUTO_OFF_DEADLINE_EPOCH[] = "auto_off_epoch";
    constexpr char RELAY_PREF_AUTO_OFF_REMAINING_MS[] = "auto_off_rem";
    constexpr uint32_t AUTO_OFF_MAX_MINUTES = 7U * 24U * 60U;
}

void RelayController::loadPersistedState()
{
    bool needsPersist = false;

    Preferences preferences;
    if (!preferences.begin(RELAY_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: relay preferences unavailable; using default OFF state.");
        storageReady = false;
        relayOn = false;
        relayAutoOffMinutesValue = 0;
        autoOffTimerArmedValue = false;
        autoOffDeadlineMs = 0;
        autoOffDeadlineEpoch = 0;
        autoOffFallbackRemainingMs = 0;
        return;
    }

    relayOn = preferences.getBool(RELAY_PREF_STATE, false);
    relayAutoOffMinutesValue = preferences.getUShort(RELAY_PREF_AUTO_OFF_MINUTES, 0);
    autoOffTimerArmedValue = preferences.getBool(RELAY_PREF_AUTO_OFF_ARMED, false);
    autoOffDeadlineEpoch = preferences.getULong(RELAY_PREF_AUTO_OFF_DEADLINE_EPOCH, 0);
    autoOffFallbackRemainingMs = preferences.getULong(RELAY_PREF_AUTO_OFF_REMAINING_MS, 0);
    storageReady = true;
    preferences.end();

    if (!relayOn || relayAutoOffMinutesValue == 0)
    {
        if (autoOffTimerArmedValue || autoOffDeadlineEpoch != 0 || autoOffFallbackRemainingMs != 0)
        {
            autoOffTimerArmedValue = false;
            autoOffDeadlineMs = 0;
            autoOffDeadlineEpoch = 0;
            autoOffFallbackRemainingMs = 0;
            needsPersist = true;
        }
        if (needsPersist)
        {
            persistState();
        }
        return;
    }

    if (!autoOffTimerArmedValue)
    {
        autoOffFallbackRemainingMs = static_cast<uint32_t>(relayAutoOffMinutesValue) * 60000UL;
        autoOffTimerArmedValue = true;
        needsPersist = true;
    }

    const time_t nowEpoch = time(nullptr);
    if (autoOffDeadlineEpoch > 0 && nowEpoch > 0)
    {
        if (static_cast<uint32_t>(nowEpoch) >= autoOffDeadlineEpoch)
        {
            relayOn = false;
            autoOffTimerArmedValue = false;
            autoOffDeadlineMs = 0;
            autoOffDeadlineEpoch = 0;
            autoOffFallbackRemainingMs = 0;
            needsPersist = true;
            if (needsPersist)
            {
                persistState();
            }
            return;
        }

        autoOffFallbackRemainingMs = (autoOffDeadlineEpoch - static_cast<uint32_t>(nowEpoch)) * 1000UL;
    }

    if (autoOffFallbackRemainingMs == 0)
    {
        autoOffFallbackRemainingMs = static_cast<uint32_t>(relayAutoOffMinutesValue) * 60000UL;
        needsPersist = true;
    }

    autoOffDeadlineMs = millis() + autoOffFallbackRemainingMs;

    if (needsPersist)
    {
        persistState();
    }
}

void RelayController::persistState()
{
    Preferences preferences;
    if (!preferences.begin(RELAY_PREF_NAMESPACE, false))
    {
        storageReady = false;
        Serial.println("[NVS] Warning: failed to persist relay state.");
        return;
    }

    preferences.putBool(RELAY_PREF_STATE, relayOn);
    preferences.putUShort(RELAY_PREF_AUTO_OFF_MINUTES, relayAutoOffMinutesValue);
    preferences.putBool(RELAY_PREF_AUTO_OFF_ARMED, autoOffTimerArmedValue);
    preferences.putULong(RELAY_PREF_AUTO_OFF_DEADLINE_EPOCH, autoOffDeadlineEpoch);
    preferences.putULong(RELAY_PREF_AUTO_OFF_REMAINING_MS, autoOffFallbackRemainingMs);
    storageReady = true;
    preferences.end();
}

void RelayController::begin()
{
    pinMode(RELAY_PIN, OUTPUT);
    loadPersistedState();
    applyOutput();
}

bool RelayController::isOn() const
{
    return relayOn;
}

void RelayController::maintain(unsigned long nowMs)
{
    if (!relayOn || relayAutoOffMinutesValue == 0 || !autoOffTimerArmedValue)
    {
        return;
    }

    bool expired = false;
    const time_t nowEpoch = time(nullptr);

    if (autoOffDeadlineEpoch > 0 && nowEpoch > 0)
    {
        const uint32_t epochNow = static_cast<uint32_t>(nowEpoch);
        if (epochNow >= autoOffDeadlineEpoch)
        {
            expired = true;
        }
        else
        {
            autoOffFallbackRemainingMs = (autoOffDeadlineEpoch - epochNow) * 1000UL;
            autoOffDeadlineMs = nowMs + autoOffFallbackRemainingMs;
        }
    }
    else
    {
        if (nowMs >= autoOffDeadlineMs)
        {
            expired = true;
        }
        else
        {
            autoOffFallbackRemainingMs = autoOffDeadlineMs - nowMs;
        }
    }

    if (expired)
    {
        set(false);
    }
}

void RelayController::setStateChangedCallback(StateChangedCallback callback)
{
    stateChangedCallback = callback;
}

int RelayController::autoOffMinutes() const
{
    return static_cast<int>(relayAutoOffMinutesValue);
}

bool RelayController::setAutoOffMinutes(int minutes, String &error)
{
    if (minutes < 0 || static_cast<uint32_t>(minutes) > AUTO_OFF_MAX_MINUTES)
    {
        error = "Auto-off minutes must be between 0 and 10080";
        return false;
    }

    relayAutoOffMinutesValue = static_cast<uint16_t>(minutes);

    if (relayAutoOffMinutesValue == 0)
    {
        clearAutoOffTimer();
    }
    else if (relayOn)
    {
        armAutoOffTimer();
    }

    persistState();
    return true;
}

bool RelayController::autoOffArmed() const
{
    return autoOffTimerArmedValue;
}

long RelayController::autoOffRemainingSeconds() const
{
    if (!relayOn || relayAutoOffMinutesValue == 0 || !autoOffTimerArmedValue)
    {
        return 0;
    }

    const time_t nowEpoch = time(nullptr);
    if (autoOffDeadlineEpoch > 0 && nowEpoch > 0)
    {
        const uint32_t epochNow = static_cast<uint32_t>(nowEpoch);
        if (epochNow >= autoOffDeadlineEpoch)
        {
            return 0;
        }
        return static_cast<long>(autoOffDeadlineEpoch - epochNow);
    }

    const unsigned long nowMs = millis();
    if (nowMs >= autoOffDeadlineMs)
    {
        return 0;
    }

    return static_cast<long>((autoOffDeadlineMs - nowMs) / 1000UL);
}

void RelayController::applyOutput() const
{
    const bool relayLevel = RELAY_ACTIVE_LOW ? !relayOn : relayOn;
    digitalWrite(RELAY_PIN, relayLevel ? HIGH : LOW);
}

void RelayController::set(bool on)
{
    if (relayOn == on)
    {
        if (on && relayAutoOffMinutesValue > 0)
        {
            armAutoOffTimer();
            persistState();
        }
        return;
    }

    relayOn = on;

    if (relayOn)
    {
        armAutoOffTimer();
    }
    else
    {
        clearAutoOffTimer();
    }

    applyOutput();
    persistState();

    if (stateChangedCallback != nullptr)
    {
        stateChangedCallback(relayOn);
    }
}

void RelayController::toggle()
{
    set(!relayOn);
}

void RelayController::armAutoOffTimer()
{
    autoOffTimerArmedValue = relayAutoOffMinutesValue > 0;
    if (!autoOffTimerArmedValue)
    {
        clearAutoOffTimer();
        return;
    }

    autoOffFallbackRemainingMs = static_cast<uint32_t>(relayAutoOffMinutesValue) * 60000UL;
    autoOffDeadlineMs = millis() + autoOffFallbackRemainingMs;

    const time_t nowEpoch = time(nullptr);
    if (nowEpoch > 0)
    {
        autoOffDeadlineEpoch = static_cast<uint32_t>(nowEpoch) + static_cast<uint32_t>(relayAutoOffMinutesValue) * 60UL;
    }
    else
    {
        autoOffDeadlineEpoch = 0;
    }
}

void RelayController::clearAutoOffTimer()
{
    autoOffTimerArmedValue = false;
    autoOffDeadlineMs = 0;
    autoOffDeadlineEpoch = 0;
    autoOffFallbackRemainingMs = 0;
}