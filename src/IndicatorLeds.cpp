/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "IndicatorLeds.h"

#include <Arduino.h>
#include <Preferences.h>

#include "AppConfig.h"

namespace
{
    constexpr char LED_PREF_NAMESPACE[] = "led_cfg";
    constexpr char LED_PREF_ACTIVE_HIGH[] = "active_high";
}

void IndicatorLeds::begin()
{
    pinMode(RELAY_LED_PIN, OUTPUT);
    pinMode(WIFI_LED_PIN, OUTPUT);

    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, true))
    {
        ledActiveHigh = preferences.getBool(LED_PREF_ACTIVE_HIGH, LED_ACTIVE_HIGH);
        preferences.end();
    }
    else
    {
        ledActiveHigh = LED_ACTIVE_HIGH;
    }

    lastRelayOn = false;
    lastWifiConnected = false;
    wifiPulseActive = false;
    wifiLedOn = false;
    relayLedManual = false;
    wifiLedManual = false;
    manualRelayLedOn = false;
    manualWifiLedOn = false;
    wifiPulseStart = 0;
    wifiLastBlink = 0;

    writeRelayLed(false);
    writeWifiLed(false);
}

bool IndicatorLeds::setActiveHigh(bool activeHigh)
{
    ledActiveHigh = activeHigh;

    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, false))
    {
        preferences.putBool(LED_PREF_ACTIVE_HIGH, ledActiveHigh);
        preferences.end();
    }
    else
    {
        return false;
    }

    const bool relayOutput = relayLedTestMode ? true : (relayLedManual ? manualRelayLedOn : lastRelayOn);
    const bool wifiOutput = wifiLedTestMode ? true : (wifiLedManual ? manualWifiLedOn : wifiLedOn);
    writeRelayLed(relayOutput);
    writeWifiLed(wifiOutput);
    return true;
}

bool IndicatorLeds::isActiveHigh() const
{
    return ledActiveHigh;
}

bool IndicatorLeds::setRelayLed(bool on)
{
    relayLedManual = true;
    manualRelayLedOn = on;
    writeRelayLed(on);
    return true;
}

bool IndicatorLeds::relayLedState() const
{
    return relayLedManual ? manualRelayLedOn : lastRelayOn;
}

bool IndicatorLeds::setWifiLed(bool on)
{
    wifiLedManual = true;
    manualWifiLedOn = on;
    writeWifiLed(on);
    return true;
}

bool IndicatorLeds::wifiLedState() const
{
    if (wifiLedTestMode)
    {
        return true;
    }

    return wifiLedManual ? manualWifiLedOn : wifiLedOn;
}

bool IndicatorLeds::startRelayLedTest(unsigned long now, unsigned long durationMs)
{
    relayLedTestMode = true;
    relayLedTestEnd = now + durationMs;
    writeRelayLed(true);
    return true;
}

bool IndicatorLeds::startWifiLedTest(unsigned long now, unsigned long durationMs)
{
    wifiLedTestMode = true;
    wifiLedTestEnd = now + durationMs;
    writeWifiLed(true);
    return true;
}

bool IndicatorLeds::startAllLedTests(unsigned long now, unsigned long durationMs)
{
    return startRelayLedTest(now, durationMs) && startWifiLedTest(now, durationMs);
}

bool IndicatorLeds::relayLedTestActive() const
{
    return relayLedTestMode;
}

bool IndicatorLeds::wifiLedTestActive() const
{
    return wifiLedTestMode;
}

void IndicatorLeds::writeRelayLed(bool on)
{
    const bool level = ledActiveHigh ? on : !on;
    digitalWrite(RELAY_LED_PIN, level ? HIGH : LOW);
}

void IndicatorLeds::writeWifiLed(bool on)
{
    const bool level = ledActiveHigh ? on : !on;
    digitalWrite(WIFI_LED_PIN, level ? HIGH : LOW);
}

void IndicatorLeds::update(unsigned long now, bool relayOn, bool wifiConnected)
{
    if (relayLedTestMode && static_cast<long>(now - relayLedTestEnd) >= 0)
    {
        relayLedTestMode = false;
    }

    if (wifiLedTestMode && static_cast<long>(now - wifiLedTestEnd) >= 0)
    {
        wifiLedTestMode = false;
    }

    if (relayOn != lastRelayOn)
    {
        lastRelayOn = relayOn;
    }

    if (wifiConnected != lastWifiConnected)
    {
        lastWifiConnected = wifiConnected;

        if (!wifiLedManual)
        {
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
            }
        }
    }

    if (!wifiLedManual && !wifiConnected)
    {
        if (!wifiLedOn)
        {
            wifiLedOn = true;
        }
    }
    else if (!wifiLedManual && wifiPulseActive)
    {
        if (now - wifiPulseStart >= WIFI_LED_PULSE_MS)
        {
            wifiPulseActive = false;
            wifiLedOn = false;
        }
    }
    else if (!wifiLedManual && wifiConnected && now - wifiLastBlink >= WIFI_LED_BLINK_INTERVAL_MS)
    {
        wifiPulseActive = true;
        wifiPulseStart = now;
        wifiLastBlink = now;
        wifiLedOn = true;
    }

    const bool relayOutput = relayLedTestMode ? true : (relayLedManual ? manualRelayLedOn : relayOn);
    const bool wifiOutput = wifiLedTestMode ? true : (wifiLedManual ? manualWifiLedOn : wifiLedOn);
    writeRelayLed(relayOutput);
    writeWifiLed(wifiOutput);
}
