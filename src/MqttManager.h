/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "AppConfig.h"

class MqttManager
{
public:
    using OperationHandler = void (*)(const String &element, const String &operation, const String &message);
    using BoolGetter = bool (*)();
    using BoolSetter = bool (*)(bool on);
    using StringGetter = String (*)();
    using IntGetter = int (*)();
    using FloatGetter = float (*)();
    using Uint8Getter = uint8_t (*)();
    using Uint8Setter = void (*)(uint8_t value);

    MqttManager();

    void begin();
    void maintain(bool wifiConnected, bool relayOn);
    void publishRelayState(bool relayOn);
    void publishLed1State(bool on);
    void publishLed2State(bool on);
    void publishLedStripState();
    void publishDeviceState();
    void publishStatusAndTemperature(bool relayOn);
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setOperationHandler(OperationHandler handler);
    void setElementHandlers(BoolGetter relayGetter, BoolSetter led1Setter, BoolGetter led1Getter, BoolSetter led2Setter, BoolGetter led2Getter, StringGetter deviceNameGetter);
    void setLedStripHandlers(Uint8Getter masterBrightnessGetter, Uint8Setter masterBrightnessSetter, BoolGetter bootAnimationGetter, BoolSetter bootAnimationSetter);
    void setTemperatureTelemetryGetters(BoolGetter probePresentGetter, IntGetter probeRawGetter, IntGetter currentRawGetter, FloatGetter currentTempCGetter);
    void setClientId(const String &clientId);
    void setServer(const String &host, int port);
    void setCredentials(const String &username, const String &password);
    String clientId() const;
    String serverHost() const;
    int serverPort() const;
    String username() const;
    String password() const;
    bool passwordSet() const;
    bool nvsReady() const;
    bool isConnected();
    int state();
    void handleMessage(char *topic, byte *payload, unsigned int length);

    static const char *stateName(int state);

private:
    void rebuildTopics();
    void connectIfNeeded(bool relayOn);
    void publishElementState(const String &element, const String &value);

    WiFiClient wifiClient;
    PubSubClient mqtt;
    OperationHandler operationHandler = nullptr;
    String mqttClientId;
    String mqttHost = MQTT_HOST;
    int mqttPort = MQTT_PORT;
    String mqttUser = MQTT_USER;
    String mqttPass = MQTT_PASS;
    String topicRoot;
    String topicOpsWildcard;
    String topicRelayState;
    String topicLed1State;
    String topicLed2State;
    String topicDeviceState;
    String topicAvail;
    String topicStatus;
    String topicTemp;
    unsigned long lastMqttRetry = 0;
    unsigned long lastTelemetryPublish = 0;
    bool settingsLoaded = false;
    bool nvsReadyFlag = false;
    bool mqttEnabled = true;
    BoolGetter getRelayState = nullptr;
    BoolSetter setLed1State = nullptr;
    BoolGetter getLed1State = nullptr;
    BoolSetter setLed2State = nullptr;
    BoolGetter getLed2State = nullptr;
    StringGetter getDeviceName = nullptr;
    Uint8Getter getLedStripMasterBrightness = nullptr;
    Uint8Setter setLedStripMasterBrightness = nullptr;
    BoolGetter getLedStripBootAnimation = nullptr;
    BoolSetter setLedStripBootAnimation = nullptr;
    BoolGetter getProbePresent = nullptr;
    IntGetter getProbeRaw = nullptr;
    IntGetter getCurrentProbeRaw = nullptr;
    FloatGetter getCurrentProbeTempC = nullptr;

    void loadSettings();
};