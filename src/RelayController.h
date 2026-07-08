#pragma once

class RelayController
{
public:
    using StateChangedCallback = void (*)(bool state);

    void begin();
    bool isOn() const;
    void set(bool on);
    void toggle();
    void setStateChangedCallback(StateChangedCallback callback);

private:
    void applyOutput() const;
    void loadPersistedState();
    void persistState();

    bool relayOn = false;
    StateChangedCallback stateChangedCallback = nullptr;
    bool storageReady = false;
};