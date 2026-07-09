/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "RelayController.h"

#include <Arduino.h>
#include <Preferences.h>

#include "AppConfig.h"

namespace
{
    constexpr char RELAY_PREF_NAMESPACE[] = "relay_cfg";
    constexpr char RELAY_PREF_STATE[] = "state";
}

void RelayController::loadPersistedState()
{
    Preferences preferences;
    if (!preferences.begin(RELAY_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: relay preferences unavailable; using default OFF state.");
        storageReady = false;
        relayOn = false;
        return;
    }

    relayOn = preferences.getBool(RELAY_PREF_STATE, false);
    storageReady = true;
    preferences.end();
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

void RelayController::setStateChangedCallback(StateChangedCallback callback)
{
    stateChangedCallback = callback;
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
        return;
    }

    relayOn = on;
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