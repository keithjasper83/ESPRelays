#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

class Preferences
{
public:
    bool begin(const char *, bool) { return true; }
    void end() {}

    size_t getBytesLength(const char *key) const
    {
        const auto found = byteValues.find(key);
        return found == byteValues.end() ? 0 : found->second.size();
    }

    size_t getBytes(const char *key, void *value, size_t length) const
    {
        const auto found = byteValues.find(key);
        if (found == byteValues.end()) return 0;
        const size_t copied = length < found->second.size() ? length : found->second.size();
        memcpy(value, found->second.data(), copied);
        return copied;
    }

    size_t putBytes(const char *key, const void *value, size_t length)
    {
        const auto *bytes = static_cast<const uint8_t *>(value);
        byteValues[key] = std::vector<uint8_t>(bytes, bytes + length);
        if (corruptByteWrites && length > 0) byteValues[key][0] ^= 0x01U;
        return length;
    }

    bool getBool(const char *key, bool fallback) const
    {
        const auto found = boolValues.find(key);
        return found == boolValues.end() ? fallback : found->second;
    }

    int getInt(const char *key, int fallback) const
    {
        const auto found = intValues.find(key);
        return found == intValues.end() ? fallback : found->second;
    }

    float getFloat(const char *key, float fallback) const
    {
        const auto found = floatValues.find(key);
        return found == floatValues.end() ? fallback : found->second;
    }

    size_t putBool(const char *key, bool value) { boolValues[key] = value; return sizeof(value); }
    size_t putInt(const char *key, int value) { intValues[key] = value; return sizeof(value); }
    size_t putFloat(const char *key, float value) { floatValues[key] = value; return sizeof(value); }

    static void reset()
    {
        byteValues.clear();
        boolValues.clear();
        intValues.clear();
        floatValues.clear();
        corruptByteWrites = false;
    }

    static inline bool corruptByteWrites = false;

private:
    static inline std::map<std::string, std::vector<uint8_t>> byteValues;
    static inline std::map<std::string, bool> boolValues;
    static inline std::map<std::string, int> intValues;
    static inline std::map<std::string, float> floatValues;
};
