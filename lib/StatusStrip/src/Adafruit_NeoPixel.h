/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

#ifndef NEO_GRB
#define NEO_GRB 0x06
#endif

#ifndef NEO_KHZ800
#define NEO_KHZ800 0x02
#endif

class Adafruit_NeoPixel
{
public:
    Adafruit_NeoPixel(uint16_t n, uint8_t p, uint8_t t = NEO_GRB + NEO_KHZ800)
        : pixels_(nullptr), count_(n), pin_(p), type_(t)
    {
        if (count_ > 0)
        {
            pixels_ = new uint8_t[count_ * 3];
            memset(pixels_, 0, count_ * 3);
        }
    }

    ~Adafruit_NeoPixel()
    {
        delete[] pixels_;
    }

    void begin()
    {
        if (pixels_ == nullptr && count_ > 0)
        {
            pixels_ = new uint8_t[count_ * 3];
            memset(pixels_, 0, count_ * 3);
        }
    }

    void setBrightness(uint8_t brightness)
    {
        brightness_ = brightness;
    }

    void clear()
    {
        if (pixels_ != nullptr)
        {
            memset(pixels_, 0, count_ * 3);
        }
    }

    void show()
    {
        // This project keeps the status-strip logic local; the board-level
        // pixel driver is intentionally minimal so the code remains buildable
        // in CI without depending on the retired external NeoPixel library.
        (void)pin_;
        (void)type_;
    }

    void setPixelColor(uint16_t index, uint32_t color)
    {
        if (pixels_ == nullptr || index >= count_)
        {
            return;
        }

        const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
        const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
        const uint8_t b = static_cast<uint8_t>(color & 0xFF);

        const uint16_t offset = index * 3;
        if ((type_ & 0x0F) == 0x06)
        {
            pixels_[offset] = g;
            pixels_[offset + 1] = r;
            pixels_[offset + 2] = b;
        }
        else
        {
            pixels_[offset] = r;
            pixels_[offset + 1] = g;
            pixels_[offset + 2] = b;
        }
    }

    uint32_t getPixelColor(uint16_t index) const
    {
        if (pixels_ == nullptr || index >= count_)
        {
            return 0;
        }

        const uint16_t offset = index * 3;
        return (static_cast<uint32_t>(pixels_[offset]) << 16) |
               (static_cast<uint32_t>(pixels_[offset + 1]) << 8) |
               pixels_[offset + 2];
    }

    uint16_t numPixels() const
    {
        return count_;
    }

private:
    uint8_t *pixels_;
    uint16_t count_;
    uint8_t pin_;
    uint8_t type_;
    uint8_t brightness_ = 64;
};
