/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>

class String
{
public:
    String() = default;
    String(const char *value) : value_(value != nullptr ? value : "") {}
    String(const std::string &value) : value_(value) {}
    String(char value) : value_(1, value) {}

    size_t length() const { return value_.length(); }
    const char *c_str() const { return value_.c_str(); }

    bool equalsIgnoreCase(const char *other) const
    {
        if (other == nullptr)
        {
            return false;
        }
        return equalsIgnoreCase(String(other));
    }

    bool equalsIgnoreCase(const String &other) const
    {
        if (value_.length() != other.value_.length())
        {
            return false;
        }

        for (size_t i = 0; i < value_.length(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(value_[i])) !=
                std::tolower(static_cast<unsigned char>(other.value_[i])))
            {
                return false;
            }
        }

        return true;
    }

    bool operator==(const String &other) const { return value_ == other.value_; }
    bool operator!=(const String &other) const { return !(*this == other); }

    String &operator+=(const char *rhs)
    {
        if (rhs != nullptr)
        {
            value_ += rhs;
        }
        return *this;
    }

    String &operator+=(const String &rhs)
    {
        value_ += rhs.value_;
        return *this;
    }

    char operator[](size_t index) const { return value_[index]; }

    std::string std() const { return value_; }

private:
    std::string value_;
};

inline String operator+(const String &lhs, const String &rhs)
{
    return String(lhs.std() + rhs.std());
}

class Stream
{
public:
    virtual ~Stream() = default;

    virtual void print(const char *text)
    {
        if (text != nullptr)
        {
            buffer_ += text;
        }
    }

    virtual void print(const String &text)
    {
        buffer_ += text.std();
    }

    virtual void println(const char *text)
    {
        print(text);
        buffer_ += '\n';
    }

    virtual void println(const String &text)
    {
        print(text);
        buffer_ += '\n';
    }

    std::string buffer() const
    {
        return buffer_;
    }

    void clearBuffer()
    {
        buffer_.clear();
    }

private:
    std::string buffer_;
};

class HardwareSerial : public Stream
{
};

class EspStub
{
public:
    void restart() { restarted = true; }
    bool restarted = false;
};

inline HardwareSerial Serial;
inline EspStub ESP;

inline void delay(unsigned long)
{
}
