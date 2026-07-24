/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

class ButtonManager
{
public:
    using ButtonCallback = void (*)();

    void begin();
    void update(unsigned long now);
    void setRelayButtonCallback(ButtonCallback callback);
    void setResetButtonCallback(ButtonCallback callback);
    void setFactoryResetCallback(ButtonCallback callback);

private:
    void pollButton(
        uint8_t pin,
        bool &lastReading,
        bool &stableState,
        unsigned long &lastDebounceTime,
        ButtonCallback callback,
        unsigned long now);

    ButtonCallback relayButtonCallback = nullptr;
    ButtonCallback resetButtonCallback = nullptr;
    ButtonCallback factoryResetCallback = nullptr;

    bool relayButtonLastReading = HIGH;
    bool relayButtonStableState = HIGH;
    unsigned long relayButtonLastDebounceTime = 0;

    bool resetButtonLastReading = HIGH;
    bool resetButtonStableState = HIGH;
    unsigned long resetButtonLastDebounceTime = 0;

    bool factoryResetButtonLastReading = HIGH;
    bool factoryResetButtonStableState = HIGH;
    unsigned long factoryResetButtonLastDebounceTime = 0;
    unsigned long factoryResetButtonPressedAt = 0;
    bool factoryResetTriggered = false;
};
