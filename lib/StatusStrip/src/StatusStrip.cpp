/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "StatusStrip.h"

namespace
{
    constexpr uint32_t COLOR_OFF = 0x000000;
    constexpr uint32_t COLOR_BOOT_START = 0xFF0000;
    constexpr uint32_t COLOR_WIFI = 0x00A5FF;
    constexpr uint32_t COLOR_RELAY_ON = 0x00FF66;
    constexpr uint32_t COLOR_RELAY_OFF = 0x00B7D9;
    constexpr uint32_t COLOR_OTA = 0xFFB000;
    constexpr uint32_t COLOR_FAULT = 0xFF0000;

    uint8_t dim8(uint8_t value, uint8_t scale)
    {
        return (value * scale) >> 8;
    }

    uint32_t dimColor(uint32_t color, uint8_t scale)
    {
        const uint8_t r = dim8(static_cast<uint8_t>((color >> 16) & 0xFF), scale);
        const uint8_t g = dim8(static_cast<uint8_t>((color >> 8) & 0xFF), scale);
        const uint8_t b = dim8(static_cast<uint8_t>(color & 0xFF), scale);
        return static_cast<uint32_t>(r) << 16 | static_cast<uint32_t>(g) << 8 | b;
    }
}

StatusStrip::StatusStrip(uint8_t pin, uint16_t count, uint8_t brightness)
    : pixels(count, pin, NEO_GRB + NEO_KHZ800)
{
    pixels.setBrightness(brightness);
}

void StatusStrip::begin()
{
    pixels.begin();
    pixels.clear();
    pixels.show();
}

StatusStrip::Color StatusStrip::wheel(uint8_t position)
{
    if (position < 85)
    {
        return static_cast<Color>(position * 3) << 16 |
               static_cast<Color>(255 - position * 3) << 8 |
               0;
    }
    if (position < 170)
    {
        position -= 85;
        return static_cast<Color>(255 - position * 3) << 16 |
               0 |
               static_cast<Color>(position * 3) << 0;
    }
    position -= 170;
    return 0 |
           static_cast<Color>(position * 3) << 8 |
           static_cast<Color>(255 - position * 3) << 0;
}

void StatusStrip::setAll(Color color)
{
    for (uint16_t i = 0; i < pixels.numPixels(); ++i)
    {
        pixels.setPixelColor(i, color);
    }
}

void StatusStrip::update(bool relayOnValue, wl_status_t wifiStatusValue, bool otaUpdateAvailableValue, bool faultValue)
{
    relayOn = relayOnValue;
    wifiStatus = wifiStatusValue;
    otaUpdateAvailable = otaUpdateAvailableValue;
    fault = faultValue;

    if (fault)
    {
        mode = Mode::Fault;
    }
    else if (otaUpdateAvailable)
    {
        mode = Mode::OtaUpdate;
    }
    else if (wifiStatus == WL_CONNECTED)
    {
        mode = relayOn ? Mode::RelayOn : Mode::RelayOff;
    }
    else if (wifiStatus == WL_IDLE_STATUS || wifiStatus == WL_DISCONNECTED || wifiStatus == WL_CONNECT_FAILED)
    {
        mode = Mode::WiFiConnecting;
    }
    else if (wifiStatus == WL_NO_SHIELD)
    {
        mode = Mode::Idle;
    }
    else
    {
        mode = Mode::WiFiConnected;
    }

    renderFrame(millis());
}

void StatusStrip::renderFrame(unsigned long nowMs)
{
    switch (mode)
    {
    case Mode::Boot:
    {
        for (uint16_t i = 0; i < pixels.numPixels(); ++i)
        {
            pixels.setPixelColor(i, wheel(static_cast<uint8_t>((i * 255U / pixels.numPixels() + nowMs / 40U) & 0xFF)));
        }
        break;
    }
    case Mode::Idle:
        setAll(dimColor(COLOR_OFF, 0));
        break;
    case Mode::WiFiConnecting:
    {
        const uint8_t step = static_cast<uint8_t>((nowMs / 180U) % pixels.numPixels());
        for (uint16_t i = 0; i < pixels.numPixels(); ++i)
        {
            pixels.setPixelColor(i, (i == step) ? COLOR_WIFI : COLOR_OFF);
        }
        break;
    }
    case Mode::WiFiConnected:
        setAll(dimColor(COLOR_WIFI, 180));
        break;
    case Mode::RelayOn:
        setAll(dimColor(COLOR_RELAY_ON, 200));
        break;
    case Mode::RelayOff:
        setAll(dimColor(COLOR_RELAY_OFF, 180));
        break;
    case Mode::OtaUpdate:
    {
        const uint8_t step = static_cast<uint8_t>((nowMs / 120U) % pixels.numPixels());
        for (uint16_t i = 0; i < pixels.numPixels(); ++i)
        {
            pixels.setPixelColor(i, i == step ? COLOR_OTA : dimColor(COLOR_OTA, 60));
        }
        break;
    }
    case Mode::Fault:
    {
        const bool flash = (nowMs / 180U) % 2U == 0U;
        setAll(flash ? COLOR_FAULT : COLOR_OFF);
        break;
    }
    }

    pixels.show();
}
