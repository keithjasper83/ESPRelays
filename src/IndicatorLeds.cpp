/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "IndicatorLeds.h"

#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#include "AppConfig.h"

namespace
{
    constexpr char LED_PREF_NAMESPACE[] = "led_cfg";
    constexpr char LED_PREF_ACTIVE_HIGH[] = "active_high";
    constexpr char LED_PREF_MASTER_BRIGHTNESS[] = "master_brightness";
    constexpr char LED_PREF_LED_BRIGHTNESS[] = "led_brightness";
    constexpr char LED_PREF_BOOT_ANIMATION[] = "boot_animation";
    constexpr uint8_t MAX_BRIGHTNESS = 255;
    constexpr uint8_t MIN_BRIGHTNESS = 0;
    constexpr uint8_t DEFAULT_MASTER_BRIGHTNESS = LED_STRIP_MASTER_BRIGHTNESS_DEFAULT;
}

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

// Hard brightness safety limit: 80% of 255 = 204, non-bypassable
constexpr uint8_t HARD_BRIGHTNESS_LIMIT = LED_STRIP_HARD_LIMIT_BRIGHTNESS;

// NeoPixel strip definition (optional, GPIO 8 safe)
#ifdef LED_STRIP_PIN
Adafruit_NeoPixel strip(LED_STRIP_COUNT, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);
#endif

void IndicatorLeds::begin()
{
    // Initialize discrete LEDs
    if (DISCRETE_STATUS_LEDS_ENABLED)
    {
        pinMode(RELAY_LED_PIN, OUTPUT);
        pinMode(WIFI_LED_PIN, OUTPUT);
    }

    // Initialize addressable strip if configured
#ifdef LED_STRIP_PIN
    strip.begin();
    strip.show(); // Clear strip on startup
#endif

    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, true))
    {
        ledActiveHigh = preferences.getBool(LED_PREF_ACTIVE_HIGH, LED_ACTIVE_HIGH);
        masterBrightness = preferences.getUChar(LED_PREF_MASTER_BRIGHTNESS, DEFAULT_MASTER_BRIGHTNESS);
        preferences.end();
    }
    else
    {
        ledActiveHigh = LED_ACTIVE_HIGH;
        masterBrightness = DEFAULT_MASTER_BRIGHTNESS;
    }

    lastRelayOn = false;
    lastWifiConnected = false;
    wifiPulseActive = false;
    wifiLedOn = false;
    relayLedManual = false;
    wifiLedManual = false;
    manualRelayLedOn = false;
    manualWifiLedOn = false;
    relayLedTestMode = false;
    wifiLedTestMode = false;
    relayLedTestEnd = 0;
    wifiLedTestEnd = 0;
    wifiPulseStart = 0;
    wifiLastBlink = 0;
    bootAnimationActive = false;
    bootAnimationEnd = 0;

    // Start boot animation after initialization
    unsigned long now = millis();
    startBootAnimation(now);

    writeRelayLed(false);
    writeWifiLed(false);
#ifdef LED_STRIP_PIN
    setStripAllOff();
#endif
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
    if (!DISCRETE_STATUS_LEDS_ENABLED)
    {
        return;
    }

    const bool level = ledActiveHigh ? on : !on;
    digitalWrite(RELAY_LED_PIN, level ? HIGH : LOW);
}

void IndicatorLeds::writeWifiLed(bool on)
{
    if (!DISCRETE_STATUS_LEDS_ENABLED)
    {
        return;
    }

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

    if (bootAnimationActive && static_cast<long>(now - bootAnimationEnd) >= 0)
    {
        bootAnimationActive = false;
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

bool IndicatorLeds::setMasterBrightness(uint8_t brightness)
{
    if (brightness < MIN_BRIGHTNESS || brightness > HARD_BRIGHTNESS_LIMIT)
    {
        return false;
    }

    masterBrightness = brightness;
    
    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, false))
    {
        preferences.putUChar(LED_PREF_MASTER_BRIGHTNESS, masterBrightness);
        preferences.end();
    }
    else
    {
        return false;
    }

    // Apply new master brightness to all LEDs
#ifdef LED_STRIP_PIN
    for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Off
    }
    strip.show();
#endif

    return true;
}

uint8_t IndicatorLeds::getMasterBrightness() const
{
    return masterBrightness;
}

bool IndicatorLeds::setPerLedBrightness(uint8_t index, uint8_t brightness)
{
    if (index >= LED_STRIP_COUNT)
    {
        return false;
    }

    if (brightness < MIN_BRIGHTNESS || brightness > HARD_BRIGHTNESS_LIMIT)
    {
        return false;
    }

    ledBrightness[index] = brightness;
    
    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, false))
    {
        char key[32];
        snprintf(key, sizeof(key), "%s_%u", LED_PREF_LED_BRIGHTNESS, index);
        preferences.putUChar(key, brightness);
        preferences.end();
    }
    else
    {
        return false;
    }

    // Apply brightness to strip
#ifdef LED_STRIP_PIN
    applyStripBrightness();
#endif

    return true;
}

uint8_t IndicatorLeds::getPerLedBrightness(uint8_t index) const
{
    if (index >= LED_STRIP_COUNT)
    {
        return 0;
    }

    uint8_t stored = DEFAULT_MASTER_BRIGHTNESS;
    Preferences preferences;
    if (preferences.begin(LED_PREF_NAMESPACE, false))
    {
        char key[32];
        snprintf(key, sizeof(key), "%s_%u", LED_PREF_LED_BRIGHTNESS, index);
        stored = preferences.getUChar(key, DEFAULT_MASTER_BRIGHTNESS);
        preferences.end();
    }

    return stored;
}

bool IndicatorLeds::isBootAnimationActive() const
{
    return bootAnimationActive;
}

bool IndicatorLeds::startBootAnimation(unsigned long now)
{
    if (bootAnimationActive)
    {
        return false;
    }

#ifdef LED_STRIP_PIN
    bootAnimationActive = true;
    bootAnimationEnd = now + LED_STRIP_BOOT_ANIMATION_DURATION_MS;
    bootAnimationStart = now;

    // Progressive fill animation
    const uint8_t steps = LED_STRIP_COUNT;
    const uint8_t stepDuration = LED_STRIP_BOOT_ANIMATION_DURATION_MS / steps;
    const uint8_t step = (now - bootAnimationStart) / stepDuration;
    
    if (step < steps)
    {
        const uint8_t activePixels = step + 1;
        for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
        {
            uint8_t r, g, b;
            if (i < activePixels)
            {
                // Gradient from cyan to green
                const uint8_t t = (i * 255) / activePixels;
                r = t;
                g = t + 50;
                b = 255 - t;
            }
            else
            {
                r = 0;
                g = 0;
                b = 0;
            }
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();
    }
    else
    {
        // Animation complete - show status LEDs
        bootAnimationActive = false;
        bootAnimationEnd = 0;
        bootAnimationStart = 0;
    }
#endif

    return bootAnimationActive;
}

void IndicatorLeds::setStripAllOff()
{
#ifdef LED_STRIP_PIN
    for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
#endif
}

void IndicatorLeds::setStripAll(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef LED_STRIP_PIN
    for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
#endif
}

void IndicatorLeds::setStripPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= LED_STRIP_COUNT)
    {
        return;
    }

#ifdef LED_STRIP_PIN
    strip.setPixelColor(index, strip.Color(r, g, b));
    strip.show();
#endif
}

void IndicatorLeds::applyStripBrightness()
{
#ifdef LED_STRIP_PIN
    for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
    {
        uint8_t brightness = ledBrightness[i];
        if (brightness == 0)
        {
            // Off
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
        else
        {
            // Apply per-LED brightness with master scaling and hard limit
            uint8_t effectiveBrightness = masterBrightness;
            if (ledBrightness[i] > 0)
            {
                effectiveBrightness = (effectiveBrightness * ledBrightness[i]) / 255;
            }
            
            // Hard limit safety
            if (effectiveBrightness > HARD_BRIGHTNESS_LIMIT)
            {
                effectiveBrightness = HARD_BRIGHTNESS_LIMIT;
            }

            Adafruit_NeoPixel::led_t currentColor = strip.getPixelColor(i);
            if (currentColor.r > 0 || currentColor.g > 0 || currentColor.b > 0)
            {
                Adafruit_NeoPixel::led_t scaled;
                scaled.r = (currentColor.r * effectiveBrightness) / 255;
                scaled.g = (currentColor.g * effectiveBrightness) / 255;
                scaled.b = (currentColor.b * effectiveBrightness) / 255;
                strip.setPixelColor(i, strip.Color(scaled.r, scaled.g, scaled.b));
            }
        }
    }
    strip.show();
#endif
}

bool IndicatorLeds::setLedStatus(uint8_t index, LedStatus status)
{
    if (index >= LED_STRIP_COUNT)
    {
        return false;
    }

#ifdef LED_STRIP_PIN
    uint8_t r, g, b;
    switch (status)
    {
        case LedStatus::Off:
            r = 0; g = 0; b = 0;
            break;
        case LedStatus::HealthOk:
            r = 52; g = 211; b = 153;
            break;
        case LedStatus::HealthDegraded:
            r = 245; g = 158; b = 11;
            break;
        case LedStatus::HealthFault:
            r = 248; g = 113; b = 113;
            break;
        case LedStatus::NetworkConnecting:
            r = 59; g = 130; b = 246;
            break;
        case LedStatus::NetworkConnected:
            r = 34; g = 211; b = 153;
            break;
        case LedStatus::NetworkFailure:
            r = 239; g = 68; b = 68;
            break;
        case LedStatus::ControllerConfigured:
            r = 16; g = 185; b = 129;
            break;
        case LedStatus::ControllerConnecting:
            r = 59; g = 130; b = 246;
            break;
        case LedStatus::ControllerUnavailable:
            r = 239; g = 68; b = 68;
            break;
        case LedStatus::RelayActive:
            r = 34; g = 211; b = 238;
            break;
        case LedStatus::RelayIdle:
            r = 52; g = 211; b = 153;
            break;
        case LedStatus::ActivityPulse:
            r = 245; g = 158; b = 11;
            break;
        case LedStatus::ActivityWarning:
            r = 251; g = 146; b = 60;
            break;
        case LedStatus::ActivityError:
            r = 239; g = 68; b = 68;
            break;
        default:
            r = 0; g = 0; b = 0;
    }

    strip.setPixelColor(index, strip.Color(r, g, b));
    strip.show();

    // Store status for potential animation
    ledStatus[index] = status;
#endif

    return true;
}

void IndicatorLeds::setLedPulse(uint8_t index, unsigned long now, unsigned long durationMs, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= LED_STRIP_COUNT)
    {
        return;
    }

#ifdef LED_STRIP_PIN
    ledPulse[index].color.r = r;
    ledPulse[index].color.g = g;
    ledPulse[index].color.b = b;
    ledPulse[index].start = now;
    ledPulse[index].duration = durationMs;
    ledPulse[index].active = true;
#endif
}

void IndicatorLeds::updateLedPulses(unsigned long now)
{
#ifdef LED_STRIP_PIN
    for (uint8_t i = 0; i < LED_STRIP_COUNT; i++)
    {
        if (ledPulse[i].active && static_cast<long>(now - ledPulse[i].start) >= 0)
        {
            ledPulse[i].active = false;
        }
    }
#endif
}

void IndicatorLeds::bootAnimationComplete()
{
    bootAnimationActive = false;
    bootAnimationEnd = 0;
    bootAnimationStart = 0;
}

int IndicatorLeds::getStatusIndex(uint8_t index) const
{
    if (index >= LED_STRIP_COUNT)
    {
        return 0;
    }
    return static_cast<int>(ledStatus[index]);
}
