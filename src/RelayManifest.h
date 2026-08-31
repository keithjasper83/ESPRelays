// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <UnifiedManifest.h>

// Values only: no manager setters, callbacks, GPIO access or persistent storage.
struct RelayManifestSnapshot {
    kjunified::Text hardwareId, legacyDeviceId, name, firmwareVersion, firmwareReleaseDate;
    bool relayOn = false, probePresent = false, monitoringEnabled = false, calibrationReady = false;
    double temperatureC = NAN;
    int relayPin = 5, temperaturePin = 1;
    int rawTemperature = -1, autoOffMinutes = 0;
    bool lowValid = false, highValid = false;
    int lowRaw = -1, highRaw = -1;
    double lowC = NAN, highC = NAN, trimC = 0;
    bool wifiConnected = false, timeValid = false;
    unsigned long uptimeSeconds = 0;
};

inline kjunified::Manifest buildRelayManifest(const RelayManifestSnapshot &s) {
    kjunified::Manifest m;
    auto &d = m.document;
    d["protocol"] = kjunified::Protocol;
    d["protocol_version"] = 1;
    d["schema_version"] = 1;
    // Bump whenever keys, structure or supported semantics change, never on state/name changes.
    d["manifest_revision"] = "relay-3";
    d["hardware"]["id"] = s.hardwareId;
    d["hardware"]["model"] = "esp32-c3";
    d["firmware"]["name"] = "esp-relay-controller";
    d["firmware"]["version"] = s.firmwareVersion;
    d["firmware"]["release_date"] = s.firmwareReleaseDate;
    d["legacy_device_id"] = s.legacyDeviceId;
    d["name"] = s.name;
    enum Node { Outputs, Relay, Sensors, Temperature, Calibration, Settings, AutoOff, Monitoring, Trim,
                Status, Wifi, Clock, Uptime, Restart, NodeCount };
    struct Descriptor { int parent; const char *key; const char *type; const char *label; const char *access; bool available; };
    // Register structure as data to avoid repeating ArduinoJson setup code in flash.
    static const Descriptor descriptors[] = {
        {-1, "outputs", "group", "Outputs", "read", true},
        {Outputs, "relay", "relay", "Relay", "read_write", true},
        {-1, "sensors", "group", "Sensors", "read", true},
        {Sensors, "temperature", "temperature_sensor", "Temperature", "read", false},
        {Temperature, "temperature-calibration", "calibration", "Calibration", "read_write", true},
        {-1, "settings", "group", "Settings", "read", true},
        {Settings, "relay-auto-off", "number", "Relay auto-off", "read_write", true},
        {Settings, "temperature-monitoring", "boolean", "Temperature monitoring", "read_write", true},
        {Settings, "temperature-trim", "number", "Temperature trim", "read_write", true},
        {-1, "status", "group", "Status", "read", true},
        {Status, "wifi-connected", "boolean", "Wi-Fi connected", "read", true},
        {Status, "time-valid", "boolean", "Clock synchronized", "read", true},
        {Status, "uptime", "number", "Uptime", "read", true},
        {Status, "restart", "action", "Restart", "action", true},
    };
    static_assert(sizeof(descriptors) / sizeof(descriptors[0]) == NodeCount, "Each relay capability needs a descriptor");
    JsonObject nodes[NodeCount];
    auto roots = d["capabilities"].to<JsonArray>();
    for (unsigned i = 0; i < NodeCount; ++i) {
        const auto &entry = descriptors[i];
        JsonArray siblings = roots;
        if (entry.parent >= 0) {
            auto parent = nodes[entry.parent];
            siblings = parent["children"].isNull() ? parent["children"].to<JsonArray>() : parent["children"].as<JsonArray>();
        }
        nodes[i] = m.add(siblings, entry.key, entry.type, entry.label, entry.access, entry.available);
    }
    auto relay = nodes[Relay];
    relay["state"] = s.relayOn;
    relay["metadata"]["gpio"] = s.relayPin;
    auto commands = relay["commands"].to<JsonArray>();
    for (const char *command : {"on", "off", "toggle"}) commands.add(command);

    const bool valid = s.probePresent && s.monitoringEnabled && s.calibrationReady && std::isfinite(s.temperatureC);
    auto temp = nodes[Temperature];
    temp["available"] = valid;
    temp["unit"] = "°C";
    temp["state"] = nullptr;
    if (valid) temp["state"] = s.temperatureC;
    auto meta = temp["metadata"].to<JsonObject>();
    meta["gpio"] = s.temperaturePin;
    meta["probe_present"] = s.probePresent;
    meta["monitoring_enabled"] = s.monitoringEnabled;
    meta["calibration_ready"] = s.calibrationReady;
    meta["raw"] = s.rawTemperature;
    auto calibration = nodes[Calibration];
    auto calState = calibration["state"].to<JsonObject>();
    calState["ready"] = s.calibrationReady;
    calState["low_valid"] = s.lowValid;
    calState["high_valid"] = s.highValid;
    calState["low_raw"] = s.lowRaw;
    calState["high_raw"] = s.highRaw;
    calState["low_c"] = nullptr;
    calState["high_c"] = nullptr;
    if (s.lowValid && std::isfinite(s.lowC)) calState["low_c"] = s.lowC;
    if (s.highValid && std::isfinite(s.highC)) calState["high_c"] = s.highC;
    auto calCommands = calibration["commands"].to<JsonArray>();
    calCommands.add("capture_low");
    calCommands.add("capture_high");
    calibration["metadata"]["capture_available"] = s.probePresent && s.monitoringEnabled;
    calCommands.add("reset_calibration");

    auto autoOff = nodes[AutoOff];
    autoOff["state"] = s.autoOffMinutes;
    autoOff["unit"] = "min";
    autoOff["min"] = 0;
    autoOff["max"] = 10080;
    autoOff["step"] = 1;
    autoOff["commands"].to<JsonArray>().add("set");
    auto monitoring = nodes[Monitoring];
    monitoring["state"] = s.monitoringEnabled;
    monitoring["commands"].to<JsonArray>().add("set");
    auto trim = nodes[Trim];
    trim["state"] = nullptr;
    if (std::isfinite(s.trimC)) trim["state"] = s.trimC;
    trim["unit"] = "°C";
    trim["min"] = -20;
    trim["max"] = 20;
    trim["step"] = 0.1;
    trim["commands"].to<JsonArray>().add("set");
    nodes[Wifi]["state"] = s.wifiConnected;
    nodes[Clock]["state"] = s.timeValid;
    auto uptime = nodes[Uptime];
    uptime["state"] = s.uptimeSeconds;
    uptime["unit"] = "s";
    auto restart = nodes[Restart];
    restart["commands"].to<JsonArray>().add("restart");
    restart["metadata"]["transport"] = "existing_unified_websocket";
    restart["metadata"]["scheduled_in_ms"] = 1000;
    return m;
}
