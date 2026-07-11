/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

class IndicatorLeds
{
public:
    void begin();
    void update(unsigned long now, bool relayOn, bool wifiConnected);
    bool setRelayLed(bool on);
    bool relayLedState() const;
    bool setWifiLed(bool on);
    bool wifiLedState() const;
    bool startRelayLedTest(unsigned long now, unsigned long durationMs);
    bool startWifiLedTest(unsigned long now, unsigned long durationMs);
    bool startAllLedTests(unsigned long now, unsigned long durationMs);
    bool relayLedTestActive() const;
    bool wifiLedTestActive() const;

private:
    void writeRelayLed(bool on);
    void writeWifiLed(bool on);

    bool lastRelayOn = false;
    bool lastWifiConnected = false;
    bool wifiPulseActive = false;
    bool wifiLedOn = false;
    bool relayLedManual = false;
    bool wifiLedManual = false;
    bool manualRelayLedOn = false;
    bool manualWifiLedOn = false;
    bool relayLedTestMode = false;
    bool wifiLedTestMode = false;
    unsigned long relayLedTestEnd = 0;
    unsigned long wifiLedTestEnd = 0;
    unsigned long wifiPulseStart = 0;
    unsigned long wifiLastBlink = 0;
};