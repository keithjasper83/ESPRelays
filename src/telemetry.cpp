/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "telemetry.h"

#include <Arduino.h>
#include <time.h>
#include <esp_system.h>
#include <esp_flash.h>
#include <esp_chip_info.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "AppConfig.h"

static const char* TAG = "telemetry";

Telemetry::Telemetry()
    : bootTime(millis())
{
    // Initialize control state to unknown
    controlState[0] = '\0';
    controlDesiredState[0] = '\0';
    controlMode[0] = '\0';
    controlLastSource[0] = '\0';
    controlLastCommand[0] = '\0';
    
    // Initialize component arrays
    for (size_t i = 0; i < MAX_RELAYS; i++) {
        relays[i].config = {};
        relays[i].state = false;
        relays[i].switchCount = 0;
        relays[i].lastChanged = 0;
        relays[i].lastActivated = 0;
        relays[i].lastDeactivated = 0;
    }
    
    for (size_t i = 0; i < MAX_TEMPERATURE_PROBES; i++) {
        temperatureProbes[i].config = {};
        temperatureProbes[i].valueC = NAN;
        temperatureProbes[i].valid = false;
        temperatureProbes[i].lastRead = 0;
        temperatureProbes[i].errorCount = 0;
    }
    
    for (size_t i = 0; i < MAX_LED_STRIPS; i++) {
        ledStrips[i].config = {};
        ledStrips[i].enabled = false;
        ledStrips[i].brightness = 0;
        ledStrips[i].red = 0;
        ledStrips[i].green = 0;
        ledStrips[i].blue = 0;
        ledStrips[i].effect[0] = '\0';
        ledStrips[i].brightnessLimit = 255;
        ledStrips[i].lastChanged = 0;
    }
    
    // Initialize warnings array
    for (size_t i = 0; i < MAX_WARNINGS; i++) {
        warnings[i].code[0] = '\0';
        warnings[i].message[0] = '\0';
        warnings[i].active = false;
    }
}

Telemetry::~Telemetry()
{
}

void Telemetry::begin(const TelemetryDevice& dev)
{
    deviceInfo = dev;
    
    // Initialize control state
    controlState[0] = '\0';
    controlDesiredState[0] = '\0';
    controlMode[0] = '\0';
    controlLastSource[0] = '\0';
    controlLastCommand[0] = '\0';
    controlLastChanged = 0;
    controlLastActivated = 0;
    controlLastDeactivated = 0;
    
    // Clear all components
    relayCount = 0;
    temperatureProbeCount = 0;
    ledStripCount = 0;
    warningCount = 0;
    
    // Clear warnings
    for (size_t i = 0; i < MAX_WARNINGS; i++) {
        warnings[i].active = false;
    }
    
    // Reset MQTT tracking
    mqttReconnectDetected = false;
    mqttDiscoveryPublished = false;
    mqttReconnectCount = 0;
    lastMqttConnectTime = 0;
    mqttMessagesSent = 0;
    mqttMessagesReceived = 0;
    mqttPublishFailures = 0;
    
    // Reset time tracking
    timeValid = false;
    lastNtpSyncEpoch = 0;
    lastNtpSyncMs = 0;
    
    ESP_LOGI(TAG, "Telemetry initialized for device: %s", deviceInfo.id);
}

void Telemetry::setMqttClient(PubSubClient& client)
{
    mqttClient = &client;
}

void Telemetry::setMqttBrokerName(const char* brokerName)
{
    mqttBrokerName = brokerName;
}

bool Telemetry::addRelay(int index)
{
    // Validate index
    if (index < 0 || index >= MAX_RELAYS) {
        ESP_LOGW(TAG, "Invalid relay index: %d", index);
        return false;
    }
    
    // Set default configuration
    relays[index].config.id = String(index).c_str();
    relays[index].config.name = (String("Relay") + String(index + 1)).c_str();
    relays[index].config.pin = index;
    relays[index].state = false;
    relays[index].switchCount = 0;
    relays[index].lastChanged = 0;
    relays[index].lastActivated = 0;
    relays[index].lastDeactivated = 0;
    
    relayCount++;
    ESP_LOGI(TAG, "Relay registered: %s (pin %d)", relays[index].config.name, relays[index].config.pin);
    return true;
}

bool Telemetry::addTemperatureProbe(int index)
{
    // Validate index
    if (index < 0 || index >= MAX_TEMPERATURE_PROBES) {
        ESP_LOGW(TAG, "Invalid temperature probe index: %d", index);
        return false;
    }
    
    // Set default configuration
    temperatureProbes[index].config.id = String(index).c_str();
    temperatureProbes[index].config.name = (String("Temp") + String(index + 1)).c_str();
    temperatureProbes[index].config.pin = index;
    temperatureProbes[index].valueC = NAN;
    temperatureProbes[index].valid = false;
    temperatureProbes[index].lastRead = 0;
    temperatureProbes[index].errorCount = 0;
    
    temperatureProbeCount++;
    ESP_LOGI(TAG, "Temperature probe registered: %s (pin %d)", temperatureProbes[index].config.name, temperatureProbes[index].config.pin);
    return true;
}

bool Telemetry::addLedStrip(int index)
{
    // Validate index
    if (index < 0 || index >= MAX_LED_STRIPS) {
        ESP_LOGW(TAG, "Invalid LED strip index: %d", index);
        return false;
    }
    
    // Set default configuration
    ledStrips[index].config.id = String(index).c_str();
    ledStrips[index].config.name = (String("LED") + String(index + 1)).c_str();
    ledStrips[index].config.pin = index;
    ledStrips[index].config.pixelCount = 1;
    ledStrips[index].enabled = true;
    ledStrips[index].brightness = 255;
    ledStrips[index].red = 0;
    ledStrips[index].green = 0;
    ledStrips[index].blue = 0;
    ledStrips[index].effect[0] = '\0';
    ledStrips[index].brightnessLimit = 255;
    ledStrips[index].lastChanged = 0;
    
    ledStripCount++;
    ESP_LOGI(TAG, "LED strip registered: %s (pin %d, %d pixels)", ledStrips[index].config.name, ledStrips[index].config.pin, ledStrips[index].config.pixelCount);
    return true;
}

void Telemetry::setRelayState(const char* componentId, bool state, const char* source)
{
    // Find the relay
    for (size_t i = 0; i < relayCount; i++) {
        if (strcmp(relays[i].config.id, componentId) == 0) {
            // Only publish event if state actually changed
            if (relays[i].state == state) {
                return;
            }
            
            relays[i].state = state;
            relays[i].lastChanged = millis();
            
            if (state) {
                relays[i].lastActivated = millis();
                relays[i].switchCount++;
            } else {
                relays[i].lastDeactivated = millis();
            }
            
            // Publish event if MQTT is available
            if (mqttClient && mqttClient->connected()) {
                String event = serializeEvent("relay.changed", componentId, source ? source : "", state ? "on" : "off");
                publishMqtt((String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/event").c_str(), event, false);
            }
            
            return;
        }
    }
}

void Telemetry::setTemperature(const char* componentId, float temperatureC, bool valid, const char* source)
{
    // Find the temperature probe
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        if (strcmp(temperatureProbes[i].config.id, componentId) == 0) {
            if (temperatureProbes[i].valid == valid && 
                (!valid || fabsf(temperatureProbes[i].valueC - temperatureC) < 0.1f)) {
                return;
            }
            
            temperatureProbes[i].valueC = valid ? temperatureC : NAN;
            temperatureProbes[i].valid = valid;
            temperatureProbes[i].lastRead = millis();
            
            // Publish event if MQTT is available
            if (mqttClient && mqttClient->connected()) {
                String event = serializeEvent("temperature.changed", componentId, source ? source : "", valid ? 
                    (temperatureProbes[i].valueC >= 0 ? "valid" : "invalid") : "error");
                publishMqtt((String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/event").c_str(), event, false);
            }
            
            return;
        }
    }
}

void Telemetry::setLedStripState(const char* componentId, bool enabled, uint8_t brightness,
                                  uint8_t red, uint8_t green, uint8_t blue, const char* effect, const char* source)
{
    // Find the LED strip
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            bool stateChanged = false;
            
            if (ledStrips[i].enabled != enabled) {
                stateChanged = true;
            }
            if (ledStrips[i].brightness != brightness) {
                stateChanged = true;
            }
            if (ledStrips[i].red != red || ledStrips[i].green != green || ledStrips[i].blue != blue) {
                stateChanged = true;
            }
            if (strcmp(ledStrips[i].effect, effect) != 0) {
                stateChanged = true;
            }
            
            if (!stateChanged) {
                return;
            }
            
            ledStrips[i].enabled = enabled;
            ledStrips[i].brightness = brightness;
            ledStrips[i].red = red;
            ledStrips[i].green = green;
            ledStrips[i].blue = blue;
            strncpy(ledStrips[i].effect, effect, sizeof(ledStrips[i].effect) - 1);
            ledStrips[i].effect[sizeof(ledStrips[i].effect) - 1] = '\0';
            ledStrips[i].lastChanged = millis();
            
            // Publish event if MQTT is available
            if (mqttClient && mqttClient->connected()) {
                String event = serializeEvent("led.changed", componentId, source ? source : "", enabled ? "on" : "off");
                publishMqtt((String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/event").c_str(), event, false);
            }
            
            return;
        }
    }
}

void Telemetry::setControlState(const char* state, const char* desiredState, const char* mode, const char* source)
{
    strncpy(controlState, state, sizeof(controlState) - 1);
    controlState[sizeof(controlState) - 1] = '\0';
    
    if (desiredState) {
        strncpy(controlDesiredState, desiredState, sizeof(controlDesiredState) - 1);
        controlDesiredState[sizeof(controlDesiredState) - 1] = '\0';
    }
    
    if (mode) {
        strncpy(controlMode, mode, sizeof(controlMode) - 1);
        controlMode[sizeof(controlMode) - 1] = '\0';
    }
    
    controlLastChanged = millis();
    
    // Update desired state tracking for activation/deactivation
    if (strcmp(controlDesiredState, CONTROL_STATE_ON) == 0 && strcmp(controlState, CONTROL_STATE_OFF) == 0) {
        controlLastActivated = millis();
    } else if (strcmp(controlDesiredState, CONTROL_STATE_OFF) == 0 && strcmp(controlState, CONTROL_STATE_ON) == 0) {
        controlLastDeactivated = millis();
    }
    
    // Publish event if MQTT is available
    if (mqttClient && mqttClient->connected()) {
        String event = serializeEvent("control.state_changed", nullptr, source ? source : "", state);
        publishMqtt((String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/event").c_str(), event, false);
    }
}

void Telemetry::addWarning(const char* code, const char* message)
{
    // Check if warning already exists
    for (size_t i = 0; i < warningCount; i++) {
        if (strcmp(warnings[i].code, code) == 0) {
            // Update message if different
            if (strcmp(warnings[i].message, message) != 0) {
                strncpy(warnings[i].message, message, sizeof(warnings[i].message) - 1);
                warnings[i].message[sizeof(warnings[i].message) - 1] = '\0';
            }
            return;
        }
    }
    
    // Check capacity
    if (warningCount >= MAX_WARNINGS) {
        ESP_LOGW(TAG, "Warning storage full (max %d)", MAX_WARNINGS);
        return;
    }
    
    strncpy(warnings[warningCount].code, code, sizeof(warnings[warningCount].code) - 1);
    warnings[warningCount].code[sizeof(warnings[warningCount].code) - 1] = '\0';
    
    strncpy(warnings[warningCount].message, message, sizeof(warnings[warningCount].message) - 1);
    warnings[warningCount].message[sizeof(warnings[warningCount].message) - 1] = '\0';
    
    warnings[warningCount].active = true;
    warningCount++;
    
    ESP_LOGW(TAG, "Warning added: %s - %s", code, message);
}

void Telemetry::clearWarning(const char* code)
{
    for (size_t i = 0; i < warningCount; i++) {
        if (strcmp(warnings[i].code, code) == 0) {
            warnings[i].active = false;
            // Shift remaining warnings down
            for (size_t j = i; j < warningCount - 1; j++) {
                warnings[j] = warnings[j + 1];
            }
            warningCount--;
            return;
        }
    }
}

void Telemetry::recordEvent(const char* eventType, const char* componentId, const char* source, const char* value)
{
    // Deduplicate recent events
    if (lastEventTime > 0 && (millis() - lastEventTime) < EVENT_DEDUPE_WINDOW_MS) {
        if (strcmp(lastEventTopic, eventType) == 0 && 
            strcmp(lastEventValue, value) == 0) {
            return;
        }
    }
    
    lastEventTime = millis();
    strncpy(lastEventTopic, eventType, sizeof(lastEventTopic) - 1);
    lastEventTopic[sizeof(lastEventTopic) - 1] = '\0';
    strncpy(lastEventValue, value, sizeof(lastEventValue) - 1);
    lastEventValue[sizeof(lastEventValue) - 1] = '\0';
    
    // Publish event if MQTT is available
    if (mqttClient && mqttClient->connected()) {
        String event = serializeEvent(eventType, componentId, source ? source : "", value);
        publishMqtt((String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/event").c_str(), event, false);
    }
}

void Telemetry::recordNtpSync()
{
    timeValid = true;
    lastNtpSyncEpoch = time(nullptr);
    lastNtpSyncMs = millis();
}

void Telemetry::publishDiscovery()
{
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    String discovery = serializeDiscovery();
    String topic = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/discovery";
    
    if (publishMqtt(topic.c_str(), discovery, true)) {
        mqttDiscoveryPublished = true;
        ESP_LOGI(TAG, "Discovery published to %s", topic.c_str());
    } else {
        ESP_LOGW(TAG, "Failed to publish discovery");
    }
}

void Telemetry::publishHeartbeat()
{
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    String heartbeat = serializeHeartbeat();
    String topic = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/heartbeat";
    
    if (publishMqtt(topic.c_str(), heartbeat, false)) {
        mqttMessagesSent++;
        ESP_LOGD(TAG, "Heartbeat published");
    } else {
        mqttPublishFailures++;
    }
}

void Telemetry::publishStatus()
{
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    String status = serializeStatus();
    String topic = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/status";
    
    if (publishMqtt(topic.c_str(), status, true)) {
        mqttMessagesSent++;
        ESP_LOGD(TAG, "Status published");
    } else {
        mqttPublishFailures++;
    }
}

void Telemetry::printStatus()
{
    Serial.println(serializeStatus());
}

void Telemetry::requestImmediateReport()
{
    // Print to serial immediately
    printStatus();
    
    // Publish to MQTT if connected
    if (mqttClient && mqttClient->connected()) {
        publishStatus();
    }
}

void Telemetry::loop()
{
    unsigned long now = millis();
    
    // Check for MQTT reconnection
    if (mqttClient && mqttClient->connected() && !mqttReconnectDetected && lastMqttConnectTime > 0) {
        mqttReconnectDetected = true;
        handleMqttReconnect();
    } else if (mqttClient && !mqttClient->connected() && mqttReconnectDetected) {
        mqttReconnectDetected = false;
    }
    
    // Update last MQTT connect time
    if (mqttClient && mqttClient->connected()) {
        lastMqttConnectTime = millis();
    }
    
    // Schedule tasks based on intervals
    if (now - lastSerialPrintMs >= TELEMETRY_SERIAL_INTERVAL_MS) {
        scheduleSerialPrint();
        lastSerialPrintMs = now;
    }
    
    if (now - lastHeartbeatMs >= TELEMETRY_HEARTBEAT_INTERVAL_MS) {
        scheduleHeartbeat();
        lastHeartbeatMs = now;
    }
    
    if (now - lastStatusPublishMs >= TELEMETRY_STATUS_INTERVAL_MS) {
        scheduleStatusPublish();
        lastStatusPublishMs = now;
    }
    
    // Check if discovery should be published (only once per MQTT connection)
    if (mqttClient && mqttClient->connected() && !mqttDiscoveryPublished) {
        scheduleDiscovery();
    }
}

void Telemetry::scheduleSerialPrint()
{
    printStatus();
}

void Telemetry::scheduleHeartbeat()
{
    publishHeartbeat();
}

void Telemetry::scheduleStatusPublish()
{
    publishStatus();
}

void Telemetry::scheduleDiscovery()
{
    publishDiscovery();
}

void Telemetry::handleMqttReconnect()
{
    mqttReconnectCount++;
    
    // Publish discovery, availability online, and status snapshot
    publishDiscovery();
    
    // Publish availability online
    String avail = "{\"schema\":\"kjdev.esp.availability\",\"schema_version\":1,\"state\":\"online\"}";
    String topic = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/availability";
    publishMqtt(topic.c_str(), avail, true);
    
    // Publish status snapshot
    publishStatus();
    
    ESP_LOGI(TAG, "MQTT reconnected (count: %d). Published discovery, availability, and status.", mqttReconnectCount);
}

bool Telemetry::publishMqtt(const char* topic, const String& payload, bool retain)
{
    if (!mqttClient || !mqttClient->connected()) {
        return false;
    }
    
    if (mqttClient->publish(topic, payload.c_str(), retain)) {
        mqttMessagesSent++;
        return true;
    }
    
    mqttPublishFailures++;
    return false;
}

const char* Telemetry::formatTimestamp() const
{
    static char buffer[32];
    
    if (!timeValid) {
        buffer[0] = '\0';
        return buffer;
    }
    
    time_t now = time(nullptr);
    struct tm* tmInfo = gmtime(&now);
    
    if (tmInfo && strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tmInfo) > 0) {
        return buffer;
    }
    
    buffer[0] = '\0';
    return buffer;
}

const char* Telemetry::formatTimestampUTC() const
{
    return formatTimestamp();
}

unsigned long Telemetry::uptimeSeconds() const
{
    if (bootTime == 0) {
        return 0;
    }
    
    unsigned long now = millis();
    if (now < bootTime) {
        // Handle millis() rollover
        return (0xFFFFFFFFUL - bootTime) + now + 1;
    }
    
    return (now - bootTime) / 1000;
}

int Telemetry::calculateSignalQuality(int rssi) const
{
    // Bounded conversion: quality = constrain(2 * (rssi + 100), 0, 100)
    int quality = 2 * (rssi + 100);
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;
    return quality;
}

bool Telemetry::isValidComponentId(const char* id)
{
    if (!id || id[0] == '\0') {
        return false;
    }
    
    // Check for invalid characters
    for (const char* p = id; *p; p++) {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) {
            return false;
        }
    }
    
    // Reasonable length check
    size_t len = strlen(id);
    if (len > 64) {
        return false;
    }
    
    return true;
}

bool Telemetry::componentIdExists(const char* id, const char** existingIds, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (existingIds[i] && strcmp(existingIds[i], id) == 0) {
            return true;
        }
    }
    return false;
}

void Telemetry::trimString(char* str, size_t maxLen)
{
    size_t len = strlen(str);
    if (len >= maxLen) {
        str[maxLen - 1] = '\0';
    }
}

void Telemetry::escapeJsonString(char* dest, const char* src, size_t destSize)
{
    size_t j = 0;
    for (size_t i = 0; src[i] && j < destSize - 1; i++) {
        char c = src[i];
        if (c == '"' || c == '\\') {
            if (j < destSize - 2) {
                dest[j++] = '\\';
                dest[j++] = c;
            }
        } else if (c == '\n') {
            if (j < destSize - 2) {
                dest[j++] = '\\';
                dest[j++] = 'n';
            }
        } else if (c == '\r') {
            if (j < destSize - 2) {
                dest[j++] = '\\';
                dest[j++] = 'r';
            }
        } else if (c == '\t') {
            if (j < destSize - 2) {
                dest[j++] = '\\';
                dest[j++] = 't';
            }
        } else {
            dest[j++] = c;
        }
    }
    dest[j] = '\0';
}

bool Telemetry::isDuplicateEvent(const char* eventType, const char* componentId) const
{
    if (!eventType || !componentId || eventType[0] == '\0' || componentId[0] == '\0') {
        return false;
    }
    
    unsigned long now = millis();
    if (now - lastEventTime < EVENT_DEDUPE_WINDOW_MS) {
        return true;
    }
    
    return false;
}

void Telemetry::updateHealthState()
{
    if (warningCount > 0) {
        strncpy(health.state, HEALTH_STATE_WARNING, sizeof(health.state) - 1);
        health.state[sizeof(health.state) - 1] = '\0';
    } else {
        strncpy(health.state, HEALTH_STATE_HEALTHY, sizeof(health.state) - 1);
        health.state[sizeof(health.state) - 1] = '\0';
    }
}

void Telemetry::publishAvailability(bool online)
{
    if (!mqttClient) {
        return;
    }
    
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["online"] = online;
    
    String json;
    serializeJson(doc, json);
    
    String topic = TELEMETRY_TOPIC_PREFIX;
    topic += deviceInfo.id;
    topic += "/availability";
    publishMqtt(topic.c_str(), json, true);
}

const char* Telemetry::getChipModel() const
{
    return "ESP32-C3";
}

const char* Telemetry::getChipRevision() const
{
    return "v0.0";
}

uint32_t Telemetry::getUptimeSeconds() const
{
    return (millis() - bootTime) / 1000;
}

int Telemetry::getRssiDbm() const
{
    return 0;
}

int Telemetry::getFreeHeapBytes() const
{
    return esp_get_free_heap_size();
}

int Telemetry::getMinFreeHeapBytes() const
{
    return ESP.getMinFreeHeap();
}

int Telemetry::getMaxFreeBlockBytes() const
{
    return ESP.getFreeHeap();
}

const char* Telemetry::getFileSystemType() const
{
    return "none";
}

size_t Telemetry::getFileSystemTotalBytes() const
{
    return 0;
}

size_t Telemetry::getFileSystemUsedBytes() const
{
    return 0;
}

size_t Telemetry::getFileSystemFreeBytes() const
{
    return 0;
}

uint32_t Telemetry::getChipTemperature() const
{
    return 0;
}

const char* Telemetry::getTimeSource() const
{
    return "ntp";
}

time_t Telemetry::getLastNtpSyncEpoch() const
{
    return lastNtpSyncEpoch;
}

unsigned long Telemetry::getNtpSyncAgeMs() const
{
    return millis() - lastNtpSyncMs;
}

uint32_t Telemetry::getWifiDisconnectCount() const
{
    return 0;
}

int Telemetry::getLastDisconnectReason() const
{
    return 0;
}

unsigned long Telemetry::getWifiConnectionUptimeMs() const
{
    return 0;
}

uint32_t Telemetry::getMqttMessagesReceived() const
{
    return mqttMessagesReceived;
}

uint32_t Telemetry::getMqttPublishFailures() const
{
    return mqttPublishFailures;
}

const char* Telemetry::getHealthState() const
{
    return health.state;
}

const char* Telemetry::getControlState() const
{
    return controlState[0] ? controlState : "unknown";
}

const char* Telemetry::getControlDesiredState() const
{
    return controlDesiredState[0] ? controlDesiredState : nullptr;
}

const char* Telemetry::getControlMode() const
{
    return controlMode[0] ? controlMode : "manual";
}

const char* Telemetry::getControlLastCommandSource() const
{
    return controlLastSource;
}

const char* Telemetry::getControlLastCommandName() const
{
    return controlLastCommand;
}

unsigned long Telemetry::getControlLastCommandTimestampMs() const
{
    return controlLastChanged;
}

const char* Telemetry::getControlLastCommandResult() const
{
    return "ok";
}

const char* Telemetry::getControlLastChanged() const
{
    return controlLastChanged > 0 ? String(controlLastChanged).c_str() : "0";
}

const char* Telemetry::getControlLastActivated() const
{
    return controlLastActivated > 0 ? String(controlLastActivated).c_str() : "0";
}

const char* Telemetry::getControlLastDeactivated() const
{
    return controlLastDeactivated > 0 ? String(controlLastDeactivated).c_str() : "0";
}

const char* Telemetry::getRelayState(const char* componentId) const
{
    for (size_t i = 0; i < relayCount; i++) {
        if (strcmp(relays[i].config.id, componentId) == 0) {
            return relays[i].state ? "on" : "off";
        }
    }
    return "unknown";
}

uint32_t Telemetry::getRelaySwitchCount(const char* componentId) const
{
    for (size_t i = 0; i < relayCount; i++) {
        if (strcmp(relays[i].config.id, componentId) == 0) {
            return relays[i].switchCount;
        }
    }
    return 0;
}

unsigned long Telemetry::getRelayLastChangedMs(const char* componentId) const
{
    for (size_t i = 0; i < relayCount; i++) {
        if (strcmp(relays[i].config.id, componentId) == 0) {
            return relays[i].lastChanged;
        }
    }
    return 0;
}

float Telemetry::getTemperatureValueC(const char* componentId) const
{
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        if (strcmp(temperatureProbes[i].config.id, componentId) == 0) {
            return temperatureProbes[i].valueC;
        }
    }
    return NAN;
}

bool Telemetry::getTemperatureValid(const char* componentId) const
{
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        if (strcmp(temperatureProbes[i].config.id, componentId) == 0) {
            return temperatureProbes[i].valid;
        }
    }
    return false;
}

unsigned long Telemetry::getTemperatureProbeLastReadMs(const char* componentId) const
{
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        if (strcmp(temperatureProbes[i].config.id, componentId) == 0) {
            return temperatureProbes[i].lastRead;
        }
    }
    return 0;
}

uint32_t Telemetry::getTemperatureProbeErrorCount(const char* componentId) const
{
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        if (strcmp(temperatureProbes[i].config.id, componentId) == 0) {
            return temperatureProbes[i].errorCount;
        }
    }
    return 0;
}

bool Telemetry::getLedStripEnabled(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].enabled;
        }
    }
    return false;
}

uint8_t Telemetry::getLedStripBrightness(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].brightness;
        }
    }
    return 0;
}

uint8_t Telemetry::getLedStripRed(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].red;
        }
    }
    return 0;
}

uint8_t Telemetry::getLedStripGreen(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].green;
        }
    }
    return 0;
}

uint8_t Telemetry::getLedStripBlue(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].blue;
        }
    }
    return 0;
}

const char* Telemetry::getLedStripEffect(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].effect;
        }
    }
    return "none";
}

unsigned long Telemetry::getLedStripLastChangedMs(const char* componentId) const
{
    for (size_t i = 0; i < ledStripCount; i++) {
        if (strcmp(ledStrips[i].config.id, componentId) == 0) {
            return ledStrips[i].lastChanged;
        }
    }
    return 0;
}

// getLedBrightnessLimit not declared in header - removed

uint32_t Telemetry::getWifiChannel() const
{
    return WiFi.channel();
}

const char* Telemetry::getWifiInterface() const
{
    return "wifi";
}

const char* Telemetry::getMacAddress() const
{
    static char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             WiFi.macAddress()[0], WiFi.macAddress()[1], WiFi.macAddress()[2],
             WiFi.macAddress()[3], WiFi.macAddress()[4], WiFi.macAddress()[5]);
    return macStr;
}

size_t Telemetry::getFlashSizeBytes() const
{
    uint32_t flashSize = ESP.getFlashChipSize();
    return flashSize;
}

uint32_t Telemetry::getCpuFrequencyMhz() const
{
    return ESP.getCpuFreqMHz();
}

void Telemetry::serializeDiscovery(JsonDocument& doc) const
{
    JsonObject root = doc.to<JsonObject>();
    
    // Device information
    root["schema"] = "kjdev.esp.discovery";
    root["schema_version"] = 1;
    root["device_id"] = deviceInfo.id;
    root["device_name"] = deviceInfo.name;
    root["device_type"] = deviceInfo.type;
    root["device_model"] = getChipModel();
    root["device_revision"] = getChipRevision();
    root["device_fw_version"] = deviceInfo.fwVersion;
    root["device_hw_version"] = deviceInfo.hwVersion;
    
    // Components array
    JsonArray components = root["components"];
    
    // Add relay components
    for (size_t i = 0; i < relayCount; i++) {
        JsonObject component = components.add<JsonObject>();
        component["id"] = relays[i].config.id;
        component["name"] = relays[i].config.name;
        component["type"] = "relay";
        component["topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/relay/" + relays[i].config.id;
        component["state_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/relay/" + relays[i].config.id + "/state";
        component["command_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/relay/" + relays[i].config.id + "/set";
        component["availability_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/availability";
        component["unique_id"] = String(deviceInfo.id) + "_relay_" + relays[i].config.id;
    }
    
    // Add temperature probe components
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        JsonObject component = components.add<JsonObject>();
        component["id"] = temperatureProbes[i].config.id;
        component["name"] = temperatureProbes[i].config.name;
        component["type"] = "temperature_sensor";
        component["topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/temperature/" + temperatureProbes[i].config.id;
        component["state_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/temperature/" + temperatureProbes[i].config.id + "/state";
        component["availability_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/availability";
        component["unique_id"] = String(deviceInfo.id) + "_temp_" + temperatureProbes[i].config.id;
        component["device_class"] = "temperature";
        component["unit_of_measurement"] = "°C";
    }
    
    // Add LED strip components
    for (size_t i = 0; i < ledStripCount; i++) {
        JsonObject component = components.add<JsonObject>();
        component["id"] = ledStrips[i].config.id;
        component["name"] = ledStrips[i].config.name;
        component["type"] = "light";
        component["topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/light/" + ledStrips[i].config.id;
        component["state_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/light/" + ledStrips[i].config.id + "/state";
        component["command_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/light/" + ledStrips[i].config.id + "/set";
        component["availability_topic"] = String(TELEMETRY_TOPIC_PREFIX) + deviceInfo.id + "/availability";
        component["unique_id"] = String(deviceInfo.id) + "_light_" + ledStrips[i].config.id;
        component["brightness"] = true;
        component["brightness_scale"] = 255;
        component["effect"] = true;
    }
}

String Telemetry::serializeDiscovery() const
{
    JsonDocument doc;
    serializeDiscovery(doc);
    String buffer;
    serializeJson(doc, buffer);
    return buffer;
}

void Telemetry::serializeHeartbeat(JsonDocument& doc) const
{
    JsonObject root = doc.to<JsonObject>();
    
    root["schema"] = "kjdev.esp.heartbeat";
    root["schema_version"] = 1;
    root["device_id"] = deviceInfo.id;
    root["timestamp"] = formatTimestamp();
    root["uptime_seconds"] = uptimeSeconds();
    root["health_state"] = getHealthState();
    root["control_state"] = getControlState();
    root["control_mode"] = getControlMode();
    root["wifi_rssi_dbm"] = getRssiDbm();
    root["wifi_signal_quality"] = calculateSignalQuality(getRssiDbm());
    root["free_heap_bytes"] = getFreeHeapBytes();
    root["min_free_heap_bytes"] = getMinFreeHeapBytes();
    root["max_block_bytes"] = getMaxFreeBlockBytes();
    root["mqtt_messages_received"] = getMqttMessagesReceived();
    root["mqtt_publish_failures"] = getMqttPublishFailures();
    root["wifi_disconnect_count"] = getWifiDisconnectCount();
    root["mqtt_reconnect_count"] = mqttReconnectCount;
}

String Telemetry::serializeHeartbeat() const
{
    JsonDocument doc;
    serializeHeartbeat(doc);
    String buffer;
    serializeJson(doc, buffer);
    return buffer;
}

void Telemetry::serializeStatus(JsonDocument& doc) const
{
    JsonObject root = doc.to<JsonObject>();
    
    root["schema"] = "kjdev.esp.status";
    root["schema_version"] = 1;
    root["device_id"] = deviceInfo.id;
    root["timestamp"] = formatTimestamp();
    root["uptime_seconds"] = uptimeSeconds();
    root["health_state"] = getHealthState();
    root["control_state"] = getControlState();
    root["control_mode"] = getControlMode();
    root["control_desired_state"] = getControlDesiredState();
    root["control_last_command_source"] = getControlLastCommandSource();
    root["control_last_command_name"] = getControlLastCommandName();
    root["control_last_command_result"] = getControlLastCommandResult();
    root["control_last_changed"] = getControlLastCommandTimestampMs();
    root["control_last_activated"] = getControlLastActivated();
    root["control_last_deactivated"] = getControlLastDeactivated();
    
    // System information
    JsonObject system = root["system"];
    system["chip_model"] = getChipModel();
    system["chip_revision"] = getChipRevision();
    system["fw_version"] = deviceInfo.fwVersion;
    system["hw_version"] = deviceInfo.hwVersion;
    system["cpu_frequency_mhz"] = getCpuFrequencyMhz();
    system["flash_size_bytes"] = getFlashSizeBytes();
    system["free_heap_bytes"] = getFreeHeapBytes();
    system["min_free_heap_bytes"] = getMinFreeHeapBytes();
    system["max_block_bytes"] = getMaxFreeBlockBytes();
    system["wifi_interface"] = getWifiInterface();
    system["wifi_channel"] = getWifiChannel();
    system["wifi_rssi_dbm"] = getRssiDbm();
    system["wifi_signal_quality"] = calculateSignalQuality(getRssiDbm());
    system["wifi_disconnect_count"] = getWifiDisconnectCount();
    system["wifi_connection_uptime_ms"] = getWifiConnectionUptimeMs();
    system["mqtt_messages_received"] = getMqttMessagesReceived();
    system["mqtt_publish_failures"] = getMqttPublishFailures();
    system["mqtt_reconnect_count"] = mqttReconnectCount;
    
    // Time information
    JsonObject timeInfo = root["time"];
    timeInfo["time_source"] = getTimeSource();
    timeInfo["last_ntp_sync_epoch"] = getLastNtpSyncEpoch();
    timeInfo["ntp_sync_age_ms"] = getNtpSyncAgeMs();
    
    // File system information
    JsonObject filesystem = root["filesystem"];
    filesystem["type"] = getFileSystemType();
    filesystem["total_bytes"] = getFileSystemTotalBytes();
    filesystem["used_bytes"] = getFileSystemUsedBytes();
    filesystem["free_bytes"] = getFileSystemFreeBytes();
    
    // Relay components
    JsonArray relaysArray = root["relays"];
    for (size_t i = 0; i < relayCount; i++) {
        JsonObject relay = relaysArray.add<JsonObject>();
        relay["id"] = relays[i].config.id;
        relay["name"] = relays[i].config.name;
        relay["state"] = relays[i].state ? "on" : "off";
        relay["switch_count"] = relays[i].switchCount;
        relay["last_changed_ms"] = relays[i].lastChanged;
    }
    
    // Temperature probe components
    JsonArray temperatureProbesArray = root["temperature_probes"];
    for (size_t i = 0; i < temperatureProbeCount; i++) {
        JsonObject probe = temperatureProbesArray.add<JsonObject>();
        probe["id"] = temperatureProbes[i].config.id;
        probe["name"] = temperatureProbes[i].config.name;
        probe["value_c"] = temperatureProbes[i].valueC;
        probe["valid"] = temperatureProbes[i].valid;
        probe["last_read_ms"] = temperatureProbes[i].lastRead;
        probe["error_count"] = temperatureProbes[i].errorCount;
    }
    
    // LED strip components
    JsonArray ledStripsArray = root["led_strips"];
    for (size_t i = 0; i < ledStripCount; i++) {
        JsonObject strip = ledStripsArray.add<JsonObject>();
        strip["id"] = ledStrips[i].config.id;
        strip["name"] = ledStrips[i].config.name;
        strip["enabled"] = ledStrips[i].enabled;
        strip["brightness"] = ledStrips[i].brightness;
        strip["red"] = ledStrips[i].red;
        strip["green"] = ledStrips[i].green;
        strip["blue"] = ledStrips[i].blue;
        strip["effect"] = ledStrips[i].effect;
        strip["last_changed_ms"] = ledStrips[i].lastChanged;
    }
}

String Telemetry::serializeStatus() const
{
    JsonDocument doc;
    serializeStatus(doc);
    String buffer;
    serializeJson(doc, buffer);
    return buffer;
}

// Serialize an event to JSON
String Telemetry::serializeEvent(const char* eventType, const char* componentId, const char* source, const char* value)
{
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    
    root["event_type"] = eventType;
    root["component_id"] = componentId;
    root["source"] = source;
    root["value"] = value;
    root["timestamp"] = formatTimestamp();
    root["uptime_ms"] = uptimeSeconds() * 1000;
    
    String output;
    serializeJson(doc, output);
    return output;
}
