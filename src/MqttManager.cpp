/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "MqttManager.h"

#include <math.h>
#include <Preferences.h>

#include "AppConfig.h"

extern bool debugLogging;

namespace
{
    constexpr char MQTT_PREF_NAMESPACE[] = "mqtt_cfg";
    constexpr char MQTT_PREF_HOST[] = "host";
    constexpr char MQTT_PREF_PORT[] = "port";
    constexpr char MQTT_PREF_ENABLED[] = "enabled";
    constexpr unsigned long MQTT_TELEMETRY_INTERVAL_MS = 60000;
}

namespace
{
    MqttManager *gMqttManager = nullptr;

    void mqttCallbackThunk(char *topic, byte *payload, unsigned int length)
    {
        if (gMqttManager != nullptr)
        {
            gMqttManager->handleMessage(topic, payload, length);
        }
    }
} // namespace

const char *MqttManager::stateName(int state)
{
    switch (state)
    {
    case MQTT_CONNECTION_TIMEOUT:
        return "MQTT_CONNECTION_TIMEOUT";
    case MQTT_CONNECTION_LOST:
        return "MQTT_CONNECTION_LOST";
    case MQTT_CONNECT_FAILED:
        return "MQTT_CONNECT_FAILED";
    case MQTT_DISCONNECTED:
        return "MQTT_DISCONNECTED";
    case MQTT_CONNECTED:
        return "MQTT_CONNECTED";
    case MQTT_CONNECT_BAD_PROTOCOL:
        return "MQTT_CONNECT_BAD_PROTOCOL";
    case MQTT_CONNECT_BAD_CLIENT_ID:
        return "MQTT_CONNECT_BAD_CLIENT_ID";
    case MQTT_CONNECT_UNAVAILABLE:
        return "MQTT_CONNECT_UNAVAILABLE";
    case MQTT_CONNECT_BAD_CREDENTIALS:
        return "MQTT_CONNECT_BAD_CREDENTIALS";
    case MQTT_CONNECT_UNAUTHORIZED:
        return "MQTT_CONNECT_UNAUTHORIZED";
    default:
        return "UNKNOWN";
    }
}

MqttManager::MqttManager()
    : mqtt(wifiClient)
{
}

void MqttManager::loadSettings()
{
    if (settingsLoaded)
    {
        return;
    }

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: MQTT preferences unavailable; using compiled defaults.");
        mqttHost = MQTT_HOST;
        mqttPort = MQTT_PORT;
        nvsReadyFlag = false;
        settingsLoaded = true;
        return;
    }

    mqttHost = preferences.getString(MQTT_PREF_HOST, MQTT_HOST);
    mqttPort = preferences.getInt(MQTT_PREF_PORT, MQTT_PORT);
    mqttEnabled = preferences.getBool(MQTT_PREF_ENABLED, true);
    nvsReadyFlag = true;
    preferences.end();

    if (mqttHost.length() == 0)
    {
        mqttHost = MQTT_HOST;
    }

    settingsLoaded = true;
}

void MqttManager::rebuildTopics()
{
    topicCmd = "home/" + mqttClientId + "/command";
    topicState = "home/" + mqttClientId + "/state";
    topicAvail = "home/" + mqttClientId + "/availability";
    topicStatus = "home/" + mqttClientId + "/status";
    topicTemp = "home/" + mqttClientId + "/temperature";
}

void MqttManager::begin()
{
    gMqttManager = this;
    mqttClientId = DEVICE_HOSTNAME_DEFAULT;
    loadSettings();
    rebuildTopics();
    mqtt.setServer(mqttHost.c_str(), mqttPort);
    mqtt.setCallback(mqttCallbackThunk);
}

void MqttManager::setCommandHandler(CommandHandler handler)
{
    commandHandler = handler;
}

void MqttManager::setTemperatureTelemetryGetters(BoolGetter probePresentGetter, IntGetter probeRawGetter, IntGetter currentRawGetter, FloatGetter currentTempCGetter)
{
    getProbePresent = probePresentGetter;
    getProbeRaw = probeRawGetter;
    getCurrentProbeRaw = currentRawGetter;
    getCurrentProbeTempC = currentTempCGetter;
}

void MqttManager::setClientId(const String &clientId)
{
    if (clientId.length() == 0 || clientId == mqttClientId)
    {
        return;
    }

    mqttClientId = clientId;
    rebuildTopics();
    lastMqttRetry = 0;

    if (mqtt.connected())
    {
        mqtt.disconnect();
    }
}

void MqttManager::setServer(const String &host, int port)
{
    String cleanHost = host;
    cleanHost.trim();

    if (cleanHost.length() == 0)
    {
        return;
    }

    loadSettings();

    mqttHost = cleanHost;
    mqttPort = port > 0 ? port : MQTT_PORT;

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save MQTT settings.");
        nvsReadyFlag = false;
        return;
    }

    preferences.putString(MQTT_PREF_HOST, mqttHost);
    preferences.putInt(MQTT_PREF_PORT, mqttPort);
    nvsReadyFlag = true;
    preferences.end();

    mqtt.setServer(mqttHost.c_str(), mqttPort);

    if (mqtt.connected())
    {
        mqtt.disconnect();
    }

    lastMqttRetry = 0;
}

void MqttManager::setEnabled(bool enabled)
{
    loadSettings();

    mqttEnabled = enabled;

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save MQTT enabled state.");
        nvsReadyFlag = false;
        return;
    }

    preferences.putBool(MQTT_PREF_ENABLED, mqttEnabled);
    nvsReadyFlag = true;
    preferences.end();

    if (!mqttEnabled && mqtt.connected())
    {
        mqtt.disconnect();
    }
}

bool MqttManager::isEnabled() const
{
    return mqttEnabled;
}

String MqttManager::clientId() const
{
    return mqttClientId;
}

String MqttManager::serverHost() const
{
    return mqttHost;
}

int MqttManager::serverPort() const
{
    return mqttPort;
}

bool MqttManager::nvsReady() const
{
    return nvsReadyFlag;
}

bool MqttManager::isConnected()
{
    return mqtt.connected();
}

int MqttManager::state()
{
    return mqtt.state();
}

void MqttManager::publishRelayState(bool relayOn)
{
    if (mqttEnabled && mqtt.connected())
    {
        mqtt.publish(topicState.c_str(), relayOn ? "ON" : "OFF", true);
    }
}

void MqttManager::publishStatusAndTemperature(bool relayOn)
{
    if (!mqttEnabled || !mqtt.connected())
    {
        return;
    }

    String statusJson = "{";
    statusJson += "\"relay\":\"";
    statusJson += relayOn ? "on" : "off";
    statusJson += "\",";
    statusJson += "\"wifi\":\"";
    statusJson += WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    statusJson += "\",";
    statusJson += "\"uptime_ms\":";
    statusJson += millis();
    statusJson += "}";

    mqtt.publish(topicStatus.c_str(), statusJson.c_str(), true);

    const bool probePresent = getProbePresent != nullptr ? getProbePresent() : false;
    const int probeRaw = getProbeRaw != nullptr ? getProbeRaw() : -1;
    const int currentProbeRaw = getCurrentProbeRaw != nullptr ? getCurrentProbeRaw() : -1;
    const float probeTempC = getCurrentProbeTempC != nullptr ? getCurrentProbeTempC() : NAN;
    const float chipTempC = temperatureRead();

    String tempJson = "{";
    tempJson += "\"probe_present\":";
    tempJson += probePresent ? "true" : "false";
    tempJson += ",\"probe_raw\":";
    tempJson += probeRaw;
    tempJson += ",\"probe_current_raw\":";
    tempJson += currentProbeRaw;
    tempJson += ",\"probe_temperature_c\":";
    if (isnan(probeTempC))
    {
        tempJson += "null";
    }
    else
    {
        tempJson += String(probeTempC, 2);
    }
    tempJson += ",\"esp_temperature_c\":";
    if (isnan(chipTempC))
    {
        tempJson += "null";
    }
    else
    {
        tempJson += String(chipTempC, 1);
    }
    tempJson += "}";

    mqtt.publish(topicTemp.c_str(), tempJson.c_str(), true);
}

void MqttManager::handleMessage(char *topic, byte *payload, unsigned int length)
{
    String message;

    for (unsigned int i = 0; i < length; i++)
    {
        message += static_cast<char>(payload[i]);
    }

    if (debugLogging)
    {
        Serial.print("MQTT command on ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(message);
    }

    if (commandHandler != nullptr)
    {
        commandHandler(message);
    }
}

void MqttManager::connectIfNeeded(bool relayOn)
{
    if (!mqttEnabled)
    {
        if (mqtt.connected())
        {
            mqtt.disconnect();
        }
        return;
    }

    if (mqtt.connected())
    {
        mqtt.loop();
        return;
    }

    const unsigned long now = millis();
    if (now - lastMqttRetry < 5000)
    {
        return;
    }

    lastMqttRetry = now;

    if (debugLogging)
    {
        Serial.println();
        Serial.println("========== MQTT CONNECT ==========");
        Serial.print("MQTT host: ");
        Serial.print(mqttHost);
        Serial.print(":");
        Serial.println(mqttPort);
        Serial.print("Client ID: ");
        Serial.println(mqttClientId);
        Serial.print("Topic CMD: ");
        Serial.println(topicCmd);
        Serial.print("Topic STATE: ");
        Serial.println(topicState);
        Serial.print("Topic AVAIL: ");
        Serial.println(topicAvail);
    }

    const bool connected = mqtt.connect(
        mqttClientId.c_str(),
        topicAvail.c_str(),
        1,
        true,
        "offline");

    if (connected)
    {
        if (debugLogging)
        {
            Serial.println("MQTT connected.");
        }

        const bool primarySubscribed = mqtt.subscribe(topicCmd.c_str());

        mqtt.publish(topicAvail.c_str(), "online", true);
        mqtt.publish(topicState.c_str(), relayOn ? "ON" : "OFF", true);
        publishStatusAndTemperature(relayOn);
        lastTelemetryPublish = millis();
        mqtt.loop();

        if (debugLogging)
        {
            Serial.print("Subscribed: ");
            Serial.println(topicCmd);
            if (!primarySubscribed)
            {
                Serial.println("Warning: MQTT subscription failed.");
            }
        }
    }
    else if (debugLogging)
    {
        Serial.print("MQTT failed. State code: ");
        Serial.println(mqtt.state());
        Serial.print("MQTT failed. State text: ");
        Serial.println(stateName(mqtt.state()));
    }

    if (debugLogging)
    {
        Serial.println("==================================");
    }
}

void MqttManager::maintain(bool wifiConnected, bool relayOn)
{
    if (!mqttEnabled)
    {
        if (mqtt.connected())
        {
            mqtt.disconnect();
        }
        return;
    }

    if (!wifiConnected)
    {
        return;
    }

    connectIfNeeded(relayOn);

    if (!mqtt.connected())
    {
        return;
    }

    const unsigned long now = millis();
    if (now - lastTelemetryPublish >= MQTT_TELEMETRY_INTERVAL_MS)
    {
        publishStatusAndTemperature(relayOn);
        lastTelemetryPublish = now;
    }
}