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

    // Explicit = a deliberate user action (web UI, serial console, button) or an
    // internal timer that must always win. Reflected = state pushed back by an
    // external integration (MQTT bridge, Unified Server) which may be a stale echo.
    enum class CommandSource : uint8_t
    {
        Explicit,
        Reflected
    };

    void begin(bool forceOff);
    void maintain(unsigned long nowMs);
    bool isOn() const;
    void set(bool on, CommandSource source = CommandSource::Explicit);
    void toggle();
    void setStateChangedCallback(StateChangedCallback callback);
    int autoOffMinutes() const;
    bool setAutoOffMinutes(int minutes, String &error);
    bool autoOffArmed() const;
    long autoOffRemainingSeconds() const;
    bool bootFailsafeApplied() const;

private:
    void applyOutput() const;
    void loadPersistedState();
    void persistState();
    void armAutoOffTimer();
    void clearAutoOffTimer();

    bool relayOn = false;
    unsigned long lastExplicitChangeMs = 0;
    bool hasExplicitChange = false;
    StateChangedCallback stateChangedCallback = nullptr;
    bool storageReady = false;
    uint16_t relayAutoOffMinutesValue = 0;
    bool autoOffTimerArmedValue = false;
    unsigned long autoOffDeadlineMs = 0;
    uint32_t autoOffDeadlineEpoch = 0;
    uint32_t autoOffFallbackRemainingMs = 0;
    bool bootFailsafeAppliedValue = false;
};
