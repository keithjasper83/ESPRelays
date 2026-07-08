#include "IndicatorLeds.h"

#include <Arduino.h>

#include "AppConfig.h"

void IndicatorLeds::begin()
{
    pinMode(RELAY_LED_PIN, OUTPUT);
    pinMode(WIFI_LED_PIN, OUTPUT);

    lastRelayOn = false;
    lastWifiConnected = false;
    wifiPulseActive = false;
    wifiLedOn = false;
    wifiPulseStart = 0;
    wifiLastBlink = 0;

    writeRelayLed(false);
    writeWifiLed(false);
}

void IndicatorLeds::writeRelayLed(bool on)
{
    const bool level = LED_ACTIVE_HIGH ? on : !on;
    digitalWrite(RELAY_LED_PIN, level ? HIGH : LOW);
}

void IndicatorLeds::writeWifiLed(bool on)
{
    const bool level = LED_ACTIVE_HIGH ? on : !on;
    digitalWrite(WIFI_LED_PIN, level ? HIGH : LOW);
}

void IndicatorLeds::update(unsigned long now, bool relayOn, bool wifiConnected)
{
    if (relayOn != lastRelayOn)
    {
        lastRelayOn = relayOn;
        writeRelayLed(relayOn);
    }

    if (wifiConnected != lastWifiConnected)
    {
        lastWifiConnected = wifiConnected;

        if (wifiConnected)
        {
            wifiPulseActive = false;
            wifiLedOn = false;
            wifiLastBlink = now;
            writeWifiLed(false);
        }
        else
        {
            wifiPulseActive = false;
            wifiLedOn = true;
            writeWifiLed(true);
        }
    }

    if (!wifiConnected)
    {
        if (!wifiLedOn)
        {
            wifiLedOn = true;
            writeWifiLed(true);
        }

        return;
    }

    if (wifiPulseActive)
    {
        if (now - wifiPulseStart >= WIFI_LED_PULSE_MS)
        {
            wifiPulseActive = false;
            wifiLedOn = false;
            writeWifiLed(false);
        }

        return;
    }

    if (now - wifiLastBlink >= WIFI_LED_BLINK_INTERVAL_MS)
    {
        wifiPulseActive = true;
        wifiPulseStart = now;
        wifiLastBlink = now;
        wifiLedOn = true;
        writeWifiLed(true);
    }
}