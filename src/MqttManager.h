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
    using CommandHandler = void (*)(const String &message);

    MqttManager();

    void begin();
    void maintain(bool wifiConnected, bool relayOn);
    void publishRelayState(bool relayOn);
    void publishStatusAndTemperature(bool relayOn);
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setCommandHandler(CommandHandler handler);
    void setClientId(const String &clientId);
    void setServer(const String &host, int port);
    String clientId() const;
    String serverHost() const;
    int serverPort() const;
    bool nvsReady() const;
    bool isConnected();
    int state();
    void handleMessage(char *topic, byte *payload, unsigned int length);

    static const char *stateName(int state);

private:
    void rebuildTopics();
    void connectIfNeeded(bool relayOn);

    WiFiClient wifiClient;
    PubSubClient mqtt;
    CommandHandler commandHandler = nullptr;
    String mqttClientId;
    String mqttHost = MQTT_HOST;
    int mqttPort = MQTT_PORT;
    String topicCmd;
    String topicState;
    String topicAvail;
    String topicStatus;
    String topicTemp;
    String topicLegacyCmd;
    unsigned long lastMqttRetry = 0;
    unsigned long lastTelemetryPublish = 0;
    bool settingsLoaded = false;
    bool nvsReadyFlag = false;
    bool mqttEnabled = true;

    void loadSettings();
};