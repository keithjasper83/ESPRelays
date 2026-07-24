/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "ButtonManager.h"

#include "AppConfig.h"

void ButtonManager::begin()
{
    pinMode(RELAY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    pinMode(FACTORY_RESET_BUTTON_PIN, INPUT_PULLUP);

    relayButtonLastReading = digitalRead(RELAY_BUTTON_PIN);
    relayButtonStableState = relayButtonLastReading;
    relayButtonLastDebounceTime = millis();

    resetButtonLastReading = digitalRead(RESET_BUTTON_PIN);
    resetButtonStableState = resetButtonLastReading;
    resetButtonLastDebounceTime = millis();

    factoryResetButtonLastReading = digitalRead(FACTORY_RESET_BUTTON_PIN);
    factoryResetButtonStableState = factoryResetButtonLastReading;
    factoryResetButtonLastDebounceTime = millis();
    factoryResetButtonPressedAt = factoryResetButtonStableState == LOW ? millis() : 0;
    factoryResetTriggered = false;
}

void ButtonManager::setRelayButtonCallback(ButtonCallback callback)
{
    relayButtonCallback = callback;
}

void ButtonManager::setResetButtonCallback(ButtonCallback callback)
{
    resetButtonCallback = callback;
}

void ButtonManager::setFactoryResetCallback(ButtonCallback callback)
{
    factoryResetCallback = callback;
}

void ButtonManager::pollButton(
    uint8_t pin,
    bool &lastReading,
    bool &stableState,
    unsigned long &lastDebounceTime,
    ButtonCallback callback,
    unsigned long now)
{
    const bool reading = digitalRead(pin);

    if (reading != lastReading)
    {
        lastDebounceTime = now;
        lastReading = reading;
    }

    if (now - lastDebounceTime < BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    if (reading == stableState)
    {
        return;
    }

    stableState = reading;

    if (stableState == LOW && callback != nullptr)
    {
        callback();
    }
}

void ButtonManager::update(unsigned long now)
{
    pollButton(
        RELAY_BUTTON_PIN,
        relayButtonLastReading,
        relayButtonStableState,
        relayButtonLastDebounceTime,
        relayButtonCallback,
        now);

    pollButton(
        RESET_BUTTON_PIN,
        resetButtonLastReading,
        resetButtonStableState,
        resetButtonLastDebounceTime,
        resetButtonCallback,
        now);

    const bool factoryResetReading = digitalRead(FACTORY_RESET_BUTTON_PIN);

    if (factoryResetReading != factoryResetButtonLastReading)
    {
        factoryResetButtonLastDebounceTime = now;
        factoryResetButtonLastReading = factoryResetReading;
    }

    if (now - factoryResetButtonLastDebounceTime >= BUTTON_DEBOUNCE_MS &&
        factoryResetReading != factoryResetButtonStableState)
    {
        factoryResetButtonStableState = factoryResetReading;

        if (factoryResetButtonStableState == LOW)
        {
            factoryResetButtonPressedAt = now;
            factoryResetTriggered = false;
            Serial.println("BOOT button held: keep holding for 5 seconds to reset user settings.");
        }
        else
        {
            factoryResetButtonPressedAt = 0;
            factoryResetTriggered = false;
        }
    }

    if (factoryResetButtonStableState == LOW && !factoryResetTriggered &&
        now - factoryResetButtonPressedAt >= FACTORY_RESET_HOLD_MS)
    {
        factoryResetTriggered = true;
        if (factoryResetCallback != nullptr)
        {
            factoryResetCallback();
        }
    }
}
