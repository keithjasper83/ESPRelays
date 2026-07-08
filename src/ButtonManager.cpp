#include "ButtonManager.h"

#include "AppConfig.h"

void ButtonManager::begin()
{
    pinMode(RELAY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

    relayButtonLastReading = digitalRead(RELAY_BUTTON_PIN);
    relayButtonStableState = relayButtonLastReading;
    relayButtonLastDebounceTime = millis();

    resetButtonLastReading = digitalRead(RESET_BUTTON_PIN);
    resetButtonStableState = resetButtonLastReading;
    resetButtonLastDebounceTime = millis();
}

void ButtonManager::setRelayButtonCallback(ButtonCallback callback)
{
    relayButtonCallback = callback;
}

void ButtonManager::setResetButtonCallback(ButtonCallback callback)
{
    resetButtonCallback = callback;
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
}