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

    bool relayButtonLastReading = HIGH;
    bool relayButtonStableState = HIGH;
    unsigned long relayButtonLastDebounceTime = 0;

    bool resetButtonLastReading = HIGH;
    bool resetButtonStableState = HIGH;
    unsigned long resetButtonLastDebounceTime = 0;
};