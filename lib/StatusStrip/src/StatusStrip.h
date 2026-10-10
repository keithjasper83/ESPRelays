/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

class StatusStrip
{
public:
    enum class Mode
    {
        Boot,
        Idle,
        WiFiConnecting,
        WiFiConnected,
        RelayOn,
        RelayOff,
        OtaUpdate,
        Fault
    };

    StatusStrip(uint8_t pin, uint16_t count, uint8_t brightness = 64);
    void begin();
    void update(bool relayOn, wl_status_t wifiStatus, bool otaUpdateAvailable, bool fault = false);

private:
    typedef uint32_t Color;

    static Color wheel(uint8_t position);
    void setAll(Color color);
    void renderFrame(unsigned long nowMs);

    Adafruit_NeoPixel pixels;
    Mode mode = Mode::Boot;
    bool relayOn = false;
    bool otaUpdateAvailable = false;
    bool fault = false;
    wl_status_t wifiStatus = WL_IDLE_STATUS;
    unsigned long lastFrameMs = 0;
};
