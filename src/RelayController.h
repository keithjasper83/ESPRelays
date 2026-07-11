/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

class RelayController
{
public:
    using StateChangedCallback = void (*)(bool state);

    void begin();
    void maintain(unsigned long nowMs);
    bool isOn() const;
    void set(bool on);
    void toggle();
    void setStateChangedCallback(StateChangedCallback callback);
    int autoOffMinutes() const;
    bool setAutoOffMinutes(int minutes, String &error);
    bool autoOffArmed() const;
    long autoOffRemainingSeconds() const;

private:
    void applyOutput() const;
    void loadPersistedState();
    void persistState();
    void armAutoOffTimer();
    void clearAutoOffTimer();

    bool relayOn = false;
    StateChangedCallback stateChangedCallback = nullptr;
    bool storageReady = false;
    uint16_t relayAutoOffMinutesValue = 0;
    bool autoOffTimerArmedValue = false;
    unsigned long autoOffDeadlineMs = 0;
    uint32_t autoOffDeadlineEpoch = 0;
    uint32_t autoOffFallbackRemainingMs = 0;
};