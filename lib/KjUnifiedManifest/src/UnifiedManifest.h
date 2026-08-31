// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdint>

namespace kjunified {
#ifdef ARDUINO
using Text = String;
#else
using Text = std::string;
#endif
constexpr const char *Protocol = "kj-esp-unified";
constexpr const char *ManifestPath = "/unified/manifest";

// ESP32 station MAC is the factory base MAC; never use mutable WiFi.macAddress().
inline Text hardwareId(const uint8_t mac[6]) {
    char id[19];
    snprintf(id, sizeof(id), "esp32-%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return id;
}

class Manifest {
public:
    JsonDocument document;

    JsonObject add(JsonArray nodes, const char *key, const char *type,
                   const char *label, const char *access, bool available) {
        auto node = nodes.add<JsonObject>();
        node["key"] = key;
        node["type"] = type;
        node["label"] = label;
        node["access"] = access;
        node["available"] = available;
        return node;
    }

    JsonObject add(JsonArray nodes, const Text &key, const char *type,
                   const char *label, const char *access, bool available) {
        auto node = add(nodes, key.c_str(), type, label, access, available);
        node["key"] = key; // Own dynamic keys; literal keys can remain in flash.
        return node;
    }

    // Serialize only a complete valid snapshot. Failure never exposes partial JSON.
    bool serialize(Text &body, Text &error) const {
        body = "";
        error = "";
        const char *keys[128] = {};
        size_t keyCount = 0;
        const auto root = document.as<JsonObjectConst>();
        if (document.overflowed() || root.isNull() || root["protocol"] != Protocol ||
            !root["protocol_version"].is<double>() || root["protocol_version"].as<double>() != 1 ||
            !root["schema_version"].is<double>() || root["schema_version"].as<double>() != 1 ||
            !text(root["manifest_revision"], 256) || !text(root["name"], 256) ||
            !text(root["legacy_device_id"], 256) || !validHardwareId(root["hardware"]["id"]) ||
            !text(root["hardware"]["model"], 256) || !text(root["firmware"]["name"], 256) ||
            !text(root["firmware"]["version"], 256) ||
            !tree(root["capabilities"], 1, keys, keyCount) || !finite(root, 1)) {
            error = "Invalid or oversized capability manifest";
            return false;
        }
        const size_t written = serializeJson(document, body);
        if (written > 65536 || written != body.length()) {
            body = "";
            error = "Oversized or incomplete capability manifest";
            return false;
        }
        return true;
    }
private:
    static bool validHardwareId(JsonVariantConst value) {
        if (!value.is<const char *>()) return false;
        const char *id = value.as<const char *>();
        if (strlen(id) != 18 || strncmp(id, "esp32-", 6) != 0) return false;
        for (size_t i = 6; i < 18; ++i)
            if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
        return true;
    }
    static bool text(JsonVariantConst v, size_t max) {
        if (!v.is<const char *>()) return false;
        const char *s = v.as<const char *>();
        size_t count = 0;
        for (; *s; ++s) if ((static_cast<unsigned char>(*s) & 0xc0) != 0x80) ++count;
        return count > 0 && count <= max;
    }
    static bool strings(JsonVariantConst v) {
        if (!v.is<JsonArrayConst>()) return false;
        for (JsonVariantConst item : v.as<JsonArrayConst>())
            if (!item.is<const char *>()) return false;
        return true;
    }
    static bool finite(JsonVariantConst v, size_t depth) {
        if ((v.is<JsonObjectConst>() || v.is<JsonArrayConst>()) && depth > 32) return false;
        if (v.is<JsonObjectConst>()) {
            for (JsonPairConst item : v.as<JsonObjectConst>())
                if (!finite(item.value(), depth + 1)) return false;
        } else if (v.is<JsonArrayConst>()) {
            for (JsonVariantConst item : v.as<JsonArrayConst>())
                if (!finite(item, depth + 1)) return false;
        } else if (v.is<double>() && !std::isfinite(v.as<double>())) return false;
        return true;
    }
    static bool tree(JsonVariantConst nodes, size_t depth, const char **keys, size_t &keyCount) {
        if (!nodes.is<JsonArrayConst>()) return false;
        for (JsonVariantConst value : nodes.as<JsonArrayConst>()) {
            if (depth > 8 || keyCount >= 128 || !value.is<JsonObjectConst>()) return false;
            auto n = value.as<JsonObjectConst>();
            if (!text(n["key"], 128) || !text(n["type"], 128) || !text(n["label"], 256) ||
                !n["available"].is<bool>() ||
                (n["access"] != "read" && n["access"] != "read_write" && n["access"] != "action")) return false;
            const char *key = n["key"].as<const char *>();
            for (size_t i = 0; i < keyCount; ++i) if (strcmp(keys[i], key) == 0) return false;
            keys[keyCount++] = key;
            if (!n["metadata"].isUnbound() && !n["metadata"].is<JsonObjectConst>()) return false;
            for (const char *field : {"min", "max", "step"}) {
                if (!n[field].isUnbound() && (!n[field].is<double>() || !std::isfinite(n[field].as<double>()))) return false;
            }
            if (!n["min"].isUnbound() && !n["max"].isUnbound() && n["min"].as<double>() > n["max"].as<double>()) return false;
            if (!n["step"].isUnbound() && n["step"].as<double>() <= 0) return false;
            for (const char *field : {"values", "commands"})
                if (!n[field].isUnbound() && !strings(n[field])) return false;
            if (!n["children"].isUnbound() && !tree(n["children"], depth + 1, keys, keyCount)) return false;
        }
        return true;
    }
};
}
