/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

class IndicatorLeds
{
public:
    enum class LedStatus
    {
        Off,
        HealthOk,
        HealthDegraded,
        HealthFault,
        NetworkConnecting,
        NetworkConnected,
        NetworkFailure,
        ControllerConfigured,
        ControllerConnecting,
        ControllerUnavailable,
        RelayActive,
        RelayIdle,
        ActivityPulse,
        ActivityWarning,
        ActivityError
    };

    void begin();
    void update(unsigned long now, bool relayOn, bool wifiConnected);
    bool setActiveHigh(bool activeHigh);
    bool isActiveHigh() const;
    bool setRelayLed(bool on);
    bool relayLedState() const;
    bool setWifiLed(bool on);
    bool wifiLedState() const;
    bool startRelayLedTest(unsigned long now, unsigned long durationMs);
    bool startWifiLedTest(unsigned long now, unsigned long durationMs);
    bool startAllLedTests(unsigned long now, unsigned long durationMs);
    bool relayLedTestActive() const;
    bool wifiLedTestActive() const;

    // LED Strip control methods
    bool setMasterBrightness(uint8_t brightness);
    uint8_t getMasterBrightness() const;
    bool setPerLedBrightness(uint8_t index, uint8_t brightness);
    uint8_t getPerLedBrightness(uint8_t index) const;
    bool isBootAnimationActive() const;
    bool startBootAnimation(unsigned long now);
    void setStripAllOff();
    void setStripAll(uint8_t r, uint8_t g, uint8_t b);
    void setStripPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    bool setLedStatus(uint8_t index, LedStatus status);
    void setLedPulse(uint8_t index, unsigned long now, unsigned long durationMs, uint8_t r, uint8_t g, uint8_t b);
    void updateLedPulses(unsigned long now);
    void bootAnimationComplete();
    int getStatusIndex(uint8_t index) const;

private:
    void writeRelayLed(bool on);
    void writeWifiLed(bool on);
    void applyStripBrightness();

    struct LedColor
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

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
    bool ledActiveHigh = true;
    bool bootAnimationActive = false;
    unsigned long relayLedTestEnd = 0;
    unsigned long wifiLedTestEnd = 0;
    unsigned long wifiPulseStart = 0;
    unsigned long wifiLastBlink = 0;
    unsigned long bootAnimationStart = 0;
    unsigned long bootAnimationEnd = 0;
    uint8_t masterBrightness = 128;
    uint8_t ledBrightness[5] = {128, 128, 128, 128, 128};
    LedStatus ledStatus[5] = {LedStatus::Off, LedStatus::Off, LedStatus::Off, LedStatus::Off, LedStatus::Off};
    struct
    {
        bool active;
        unsigned long start;
        unsigned long duration;
        LedColor color;
    } ledPulse[5] = {
        {false, 0, 0, {0, 0, 0}},
        {false, 0, 0, {0, 0, 0}},
        {false, 0, 0, {0, 0, 0}},
        {false, 0, 0, {0, 0, 0}},
        {false, 0, 0, {0, 0, 0}}
    };
};
