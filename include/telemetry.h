/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

using ArduinoJson::JsonDocument;

// Fixed capacity limits for bounded storage
static constexpr size_t MAX_RELAYS = 4;
static constexpr size_t MAX_TEMPERATURE_PROBES = 4;
static constexpr size_t MAX_LED_STRIPS = 2;
static constexpr size_t MAX_WARNINGS = 8;

// Telemetry scheduling intervals (in milliseconds)
static constexpr unsigned long TELEMETRY_SERIAL_INTERVAL_MS = 30000;
static constexpr unsigned long TELEMETRY_HEARTBEAT_INTERVAL_MS = 60000;
static constexpr unsigned long TELEMETRY_STATUS_INTERVAL_MS = 600000;  // 10 minutes

// Topic prefixes
static constexpr const char* TELEMETRY_TOPIC_PREFIX = "esp/";

// Schema versions
static constexpr const char* SCHEMA_DISCOVERY = "kjdev.esp.discovery";
static constexpr const char* SCHEMA_HEARTBEAT = "kjdev.esp.heartbeat";
static constexpr const char* SCHEMA_STATUS = "kjdev.esp.status";
static constexpr const char* SCHEMA_EVENT = "kjdev.esp.event";

// Health states
static constexpr const char* HEALTH_STATE_HEALTHY = "healthy";
static constexpr const char* HEALTH_STATE_WARNING = "warning";
static constexpr const char* HEALTH_STATE_CRITICAL = "critical";

// Control states
static constexpr const char* CONTROL_STATE_UNKNOWN = "unknown";
static constexpr const char* CONTROL_STATE_ON = "on";
static constexpr const char* CONTROL_STATE_OFF = "off";

struct TelemetryDevice {
    const char* id;
    const char* name;
    const char* location;
    const char* function;
    const char* project;
    const char* firmwareVersion;
    const char* type;
    const char* fwVersion;
    const char* hwVersion;
};

struct TelemetryRelay {
    const char* id;
    const char* name;
    uint8_t pin;
};

struct TelemetryTemperatureProbe {
    const char* id;
    const char* name;
    uint8_t pin;
};

struct TelemetryLedStrip {
    const char* id;
    const char* name;
    uint8_t pin;
    uint16_t pixelCount;
};

class Telemetry {
public:
    Telemetry();
    ~Telemetry();

    void begin(const TelemetryDevice& device);

    // MQTT setup
    void setMqttClient(PubSubClient& client);
    void setMqttBrokerName(const char* brokerName);

    // Component registration
    bool addRelay(int index);
    bool addTemperatureProbe(int index);
    bool addLedStrip(int index);

    // State setters
    void setRelayState(
        const char* componentId,
        bool state,
        const char* source
    );

    void setTemperature(
        const char* componentId,
        float temperatureC,
        bool valid,
        const char* source = nullptr
    );

    void setLedStripState(
        const char* componentId,
        bool enabled,
        uint8_t brightness,
        uint8_t red,
        uint8_t green,
        uint8_t blue,
        const char* effect,
        const char* source = nullptr
    );

    void setControlState(
        const char* state,
        const char* desiredState,
        const char* mode,
        const char* source
    );

    // Warning management
    void addWarning(
        const char* code,
        const char* message
    );

    void clearWarning(const char* code);

    // Event recording
    void recordEvent(
        const char* eventType,
        const char* componentId,
        const char* source,
        const char* value
    );

    // Publishing
    void publishDiscovery();
    void publishHeartbeat();
    void publishStatus();
    void printStatus();

    // Manual trigger
    void requestImmediateReport();

    // Main loop
    void loop();

    // Time synchronization callback
    void recordNtpSync();

    // Event deduplication
    bool isDuplicateEvent(const char* eventType, const char* componentId) const;

    // Health state management
    void updateHealthState();

    // Availability publishing
    void publishAvailability(bool online);

    // Temperature getter helper
    float getTemperatureValueC(const char* componentId) const;

    // Getters for debugging
    bool isMqttConnected() const;
    bool isTimeValid() const;
    size_t getRelayCount() const;
    size_t getTemperatureProbeCount() const;
    size_t getLedStripCount() const;
    size_t getWarningCount() const;

    // Device/System getters
    const char* getChipModel() const;
    const char* getChipRevision() const;
    uint32_t getUptimeSeconds() const;
    int getRssiDbm() const;
    int getFreeHeapBytes() const;
    int getMinFreeHeapBytes() const;
    int getMaxFreeBlockBytes() const;
    size_t getTotalHeapBytes() const;
    size_t getPsramSizeBytes() const;
    size_t getFlashSizeBytes() const;
    uint32_t getCpuFrequencyMhz() const;
    uint32_t getChipTemperature() const;
    const char* getResetReason() const;
    const char* getTimeSource() const;
    unsigned long getNtpSyncAgeMs() const;
    bool isNtpSynced() const;

    // WiFi getters
    bool isWifiConnected() const;
    int getWifiRssi() const;
    uint32_t getWifiDisconnectCount() const;
    int getLastDisconnectReason() const;
    unsigned long getWifiConnectionUptimeMs() const;
    uint32_t getWifiChannel() const;
    const char* getWifiInterface() const;
    const char* getMacAddress() const;

    // MQTT getters
    const char* getMqttBrokerName() const;
    uint32_t getMqttMessagesReceived() const;
    uint32_t getMqttPublishFailures() const;

    // Health getters
    const char* getHealthState() const;

    // Control state getters
    const char* getControlState() const;
    const char* getControlDesiredState() const;
    const char* getControlMode() const;
    const char* getControlLastCommandSource() const;
    const char* getControlLastCommandName() const;
    unsigned long getControlLastCommandTimestampMs() const;
    const char* getControlLastCommandResult() const;
    const char* getControlLastChanged() const;
    const char* getControlLastActivated() const;
    const char* getControlLastDeactivated() const;

    // Relay getters
    const char* getRelayState(const char* componentId) const;
    uint32_t getRelaySwitchCount(const char* componentId) const;
    unsigned long getRelayLastChangedMs(const char* componentId) const;
    unsigned long getRelayLastActivatedMs(const char* componentId) const;
    unsigned long getRelayLastDeactivatedMs(const char* componentId) const;

    // Temperature probe getters
    bool getTemperatureProbePresent(const char* componentId) const;
    int getTemperatureProbeRaw(const char* componentId) const;
    float getTemperatureProbeC(const char* componentId) const;
    bool getTemperatureValid(const char* componentId) const;
    unsigned long getTemperatureProbeLastReadMs(const char* componentId) const;
    uint32_t getTemperatureProbeErrorCount(const char* componentId) const;

    // LED strip getters
    bool getLedStripEnabled(const char* componentId) const;
    uint8_t getLedStripBrightness(const char* componentId) const;
    uint8_t getLedStripRed(const char* componentId) const;
    uint8_t getLedStripGreen(const char* componentId) const;
    uint8_t getLedStripBlue(const char* componentId) const;
    const char* getLedStripEffect(const char* componentId) const;
    unsigned long getLedStripLastChangedMs(const char* componentId) const;

    // NTP sync getters
    time_t getLastNtpSyncEpoch() const;
    unsigned long getLastNtpSyncMs() const;
    // File system getters
    const char* getFileSystemType() const;
    size_t getFileSystemTotalBytes() const;
    size_t getFileSystemUsedBytes() const;
    size_t getFileSystemFreeBytes() const;

private:
    // Helper functions
    const char* formatTimestamp() const;
    const char* formatTimestampUTC() const;
    unsigned long uptimeSeconds() const;
    int calculateSignalQuality(int rssi) const;
    bool isValidComponentId(const char* id);
    bool componentIdExists(const char* id, const char** existingIds, size_t count);
    void trimString(char* str, size_t maxLen);
    static void escapeJsonString(char* dest, const char* src, size_t destSize);

    // JSON serialization
    String serializeDiscovery() const;
    String serializeHeartbeat() const;
    String serializeStatus() const;
    
    void serializeDiscovery(JsonDocument& doc) const;
    void serializeHeartbeat(JsonDocument& doc) const;
    void serializeStatus(JsonDocument& doc) const;
    String serializeRelay(const TelemetryRelay& relay, bool state, uint32_t switchCount, uint32_t lastChanged);
    String serializeTemperatureProbe(const TelemetryTemperatureProbe& probe, float value, bool valid, uint32_t lastRead, uint32_t errorCount);
    String serializeLedStrip(const TelemetryLedStrip& strip, bool enabled, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b, const char* effect, uint32_t lastChanged);
    String serializeControlState(const char* state, const char* desiredState, const char* mode, const char* lastSource, const char* lastCommand, uint32_t lastChanged);
    String serializeWarnings();
    String serializeCommunications();
    String serializeHealth();
    String serializeEvent(const char* eventType, const char* componentId, const char* source, const char* value);

    // MQTT publishing
    bool publishMqtt(const char* topic, const String& payload, bool retain);
    void handleMqttReconnect();

    // Scheduling
    void scheduleSerialPrint();
    void scheduleHeartbeat();
    void scheduleStatusPublish();
    void scheduleDiscovery();

    // Device identity
    TelemetryDevice deviceInfo;

    // MQTT
    PubSubClient* mqttClient = nullptr;
    String mqttBrokerName;
    bool mqttReconnectDetected = false;
    bool mqttDiscoveryPublished = false;
    uint32_t mqttReconnectCount = 0;
    uint32_t lastMqttConnectTime = 0;
    uint32_t mqttMessagesSent = 0;
    uint32_t mqttMessagesReceived = 0;
    uint32_t mqttPublishFailures = 0;

    // Health state
    struct HealthState {
        char state[32] = {0};
        char lastWarning[128] = {0};
        uint32_t lastWarningTime = 0;
    };
    HealthState health;

    // Time
    bool timeValid = false;
    time_t lastNtpSyncEpoch = 0;
    unsigned long lastNtpSyncMs = 0;

    // Uptime tracking

    // Scheduling tracking
    unsigned long lastSerialPrintMs = 0;
    unsigned long lastHeartbeatMs = 0;
    unsigned long lastStatusPublishMs = 0;
    unsigned long bootTime = 0;

    // Component storage (bounded arrays)
    struct RelayEntry {
        TelemetryRelay config;
        bool state = false;
        uint32_t switchCount = 0;
        uint32_t lastChanged = 0;
        uint32_t lastActivated = 0;
        uint32_t lastDeactivated = 0;
    };
    RelayEntry relays[MAX_RELAYS];
    size_t relayCount = 0;

    struct TemperatureEntry {
        TelemetryTemperatureProbe config;
        float valueC = NAN;
        bool valid = false;
        uint32_t lastRead = 0;
        uint32_t errorCount = 0;
    };
    TemperatureEntry temperatureProbes[MAX_TEMPERATURE_PROBES];
    size_t temperatureProbeCount = 0;

    struct LedEntry {
        TelemetryLedStrip config;
        bool enabled = false;
        uint8_t brightness = 0;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        char effect[32];
        uint8_t brightnessLimit = 255;
        uint32_t lastChanged = 0;
    };
    LedEntry ledStrips[MAX_LED_STRIPS];
    size_t ledStripCount = 0;

    // Control state
    char controlState[32] = {0};
    char controlDesiredState[32] = {0};
    char controlMode[32] = {0};
    char controlLastSource[32] = {0};
    char controlLastCommand[64] = {0};
    uint32_t controlLastChanged = 0;
    uint32_t controlLastActivated = 0;
    uint32_t controlLastDeactivated = 0;

    // Warnings (bounded array)
    struct WarningEntry {
        char code[32];
        char message[128];
        bool active;
    };
    WarningEntry warnings[MAX_WARNINGS];
    size_t warningCount = 0;

    // Event tracking (to prevent duplicates)
    char lastEventTopic[64] = {0};
    char lastEventValue[64] = {0};
    uint32_t lastEventTime = 0;
    constexpr static uint32_t EVENT_DEDUPE_WINDOW_MS = 100;  // 100ms deduplication window
};
