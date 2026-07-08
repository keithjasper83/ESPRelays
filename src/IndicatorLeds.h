#pragma once

class IndicatorLeds
{
public:
    void begin();
    void update(unsigned long now, bool relayOn, bool wifiConnected);

private:
    void writeRelayLed(bool on);
    void writeWifiLed(bool on);

    bool lastRelayOn = false;
    bool lastWifiConnected = false;
    bool wifiPulseActive = false;
    bool wifiLedOn = false;
    unsigned long wifiPulseStart = 0;
    unsigned long wifiLastBlink = 0;
};