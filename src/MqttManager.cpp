#include "MqttManager.h"

#include <Preferences.h>

extern bool debugLogging;

namespace
{
    constexpr char MQTT_PREF_NAMESPACE[] = "mqtt_cfg";
    constexpr char MQTT_PREF_HOST[] = "host";
    constexpr char MQTT_PREF_PORT[] = "port";
    constexpr char MQTT_PREF_ENABLED[] = "enabled";
    constexpr char MQTT_PREF_USER[] = "user";
    constexpr char MQTT_PREF_PASS[] = "pass";
    constexpr unsigned long MQTT_RETRY_MS = 5000;
    constexpr unsigned long MQTT_TELEMETRY_INTERVAL_MS = 60000;

    bool parseOnOff(const String &message, bool &on)
    {
        String normalized = message;
        normalized.trim();
        normalized.toLowerCase();

        if (normalized == "on" || normalized == "1" || normalized == "true")
        {
            on = true;
            return true;
        }

        if (normalized == "off" || normalized == "0" || normalized == "false")
        {
            on = false;
            return true;
        }

        return false;
    }
}

MqttManager::MqttManager()
    : mqtt(wifiClient)
{
}

void MqttManager::begin()
{
    loadSettings();
    rebuildTopics();

    mqtt.setServer(mqttHost.c_str(), mqttPort);
    mqtt.setCallback([this](char *topic, byte *payload, unsigned int length)
                     { this->handleMessage(topic, payload, length); });
}

void MqttManager::loadSettings()
{
    if (settingsLoaded)
    {
        return;
    }

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, true))
    {
        mqttHost = MQTT_HOST;
        mqttPort = MQTT_PORT;
        mqttUser = MQTT_USER;
        mqttPass = MQTT_PASS;
        mqttEnabled = true;
        nvsReadyFlag = false;
        settingsLoaded = true;
        return;
    }

    mqttHost = preferences.getString(MQTT_PREF_HOST, MQTT_HOST);
    mqttPort = preferences.getInt(MQTT_PREF_PORT, MQTT_PORT);
    mqttEnabled = preferences.getBool(MQTT_PREF_ENABLED, true);
    mqttUser = preferences.getString(MQTT_PREF_USER, MQTT_USER);
    mqttPass = preferences.getString(MQTT_PREF_PASS, MQTT_PASS);
    nvsReadyFlag = true;
    preferences.end();

    settingsLoaded = true;
}

void MqttManager::rebuildTopics()
{
    topicRoot = "home/" + mqttClientId;
    topicOpsWildcard = topicRoot + "/+/+";
    topicRelayState = topicRoot + "/relay/state";
    topicLed1State = topicRoot + "/led1/state";
    topicLed2State = topicRoot + "/led2/state";
    topicDeviceState = topicRoot + "/device/state";
    topicAvail = topicRoot + "/availability";
    topicStatus = topicRoot + "/status";
    topicTemp = topicRoot + "/temperature";
}

void MqttManager::setOperationHandler(OperationHandler handler)
{
    operationHandler = handler;
}

void MqttManager::setElementHandlers(BoolGetter relayGetter, BoolSetter led1Setter, BoolGetter led1Getter, BoolSetter led2Setter, BoolGetter led2Getter, StringGetter deviceNameGetter)
{
    getRelayState = relayGetter;
    setLed1State = led1Setter;
    getLed1State = led1Getter;
    setLed2State = led2Setter;
    getLed2State = led2Getter;
    getDeviceName = deviceNameGetter;
}

void MqttManager::setLedStripHandlers(Uint8Getter masterBrightnessGetter, Uint8Setter masterBrightnessSetter, BoolGetter bootAnimationGetter, BoolSetter bootAnimationSetter)
{
    getLedStripMasterBrightness = masterBrightnessGetter;
    setLedStripMasterBrightness = masterBrightnessSetter;
    getLedStripBootAnimation = bootAnimationGetter;
    setLedStripBootAnimation = bootAnimationSetter;
}

void MqttManager::publishLedStripState()
{
    if (getLedStripMasterBrightness == nullptr || setLedStripMasterBrightness == nullptr ||
        getLedStripBootAnimation == nullptr || setLedStripBootAnimation == nullptr)
    {
        return;
    }

    uint8_t masterBrightness = 0;
    bool bootAnimation = false;

    if (getLedStripMasterBrightness())
    {
        masterBrightness = getLedStripMasterBrightness();
    }

    if (getLedStripBootAnimation())
    {
        bootAnimation = getLedStripBootAnimation();
    }

    String json = "{";
    json += String("\"master_brightness\":") + String(masterBrightness) + ",";
    json += String("\"boot_animation\":") + (bootAnimation ? "true" : "false");
    json += "}";

    publishElementState("led_strip", json);
}

void MqttManager::setTemperatureTelemetryGetters(BoolGetter probePresentGetter, IntGetter probeRawGetter, IntGetter currentRawGetter, FloatGetter currentTempCGetter)
{
    getProbePresent = probePresentGetter;
    getProbeRaw = probeRawGetter;
    getCurrentProbeRaw = currentRawGetter;
    getCurrentProbeTempC = currentTempCGetter;
}

const char *MqttManager::stateName(int state)
{
    switch (state)
    {
    case MQTT_CONNECTION_TIMEOUT:
        return "CONNECTION_TIMEOUT";
    case MQTT_CONNECTION_LOST:
        return "CONNECTION_LOST";
    case MQTT_CONNECT_FAILED:
        return "CONNECT_FAILED";
    case MQTT_DISCONNECTED:
        return "DISCONNECTED";
    case MQTT_CONNECTED:
        return "CONNECTED";
    case MQTT_CONNECT_BAD_PROTOCOL:
        return "BAD_PROTOCOL";
    case MQTT_CONNECT_BAD_CLIENT_ID:
        return "BAD_CLIENT_ID";
    case MQTT_CONNECT_UNAVAILABLE:
        return "UNAVAILABLE";
    case MQTT_CONNECT_BAD_CREDENTIALS:
        return "BAD_CREDENTIALS";
    case MQTT_CONNECT_UNAUTHORIZED:
        return "UNAUTHORIZED";
    default:
        return "UNKNOWN";
    }
}

void MqttManager::setClientId(const String &clientId)
{
    mqttClientId = clientId;
    rebuildTopics();
}

void MqttManager::setServer(const String &host, int port)
{
    loadSettings();

    mqttHost = host;
    mqttPort = port;

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save MQTT host/port.");
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

void MqttManager::setCredentials(const String &username, const String &password)
{
    loadSettings();

    mqttUser = username;
    mqttPass = password;

    Preferences preferences;
    if (!preferences.begin(MQTT_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save MQTT credentials.");
        nvsReadyFlag = false;
        return;
    }

    preferences.putString(MQTT_PREF_USER, mqttUser);
    preferences.putString(MQTT_PREF_PASS, mqttPass);
    nvsReadyFlag = true;
    preferences.end();

    if (mqtt.connected())
    {
        mqtt.disconnect();
    }

    lastMqttRetry = 0;
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

String MqttManager::username() const
{
    return mqttUser;
}

String MqttManager::password() const
{
    return mqttPass;
}

bool MqttManager::passwordSet() const
{
    return mqttPass.length() > 0;
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
    }
    else
    {
        preferences.putBool(MQTT_PREF_ENABLED, mqttEnabled);
        nvsReadyFlag = true;
        preferences.end();
    }

    if (!mqttEnabled && mqtt.connected())
    {
        mqtt.publish(topicAvail.c_str(), "offline", true);
        mqtt.disconnect();
    }
}

bool MqttManager::isEnabled() const
{
    return mqttEnabled;
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

void MqttManager::publishElementState(const String &element, const String &value)
{
    if (!mqttEnabled || !mqtt.connected())
    {
        return;
    }

    const String topic = topicRoot + "/" + element + "/state";
    mqtt.publish(topic.c_str(), value.c_str(), true);
}

void MqttManager::publishRelayState(bool relayOn)
{
    publishElementState("relay", relayOn ? "on" : "off");
}

void MqttManager::publishLed1State(bool on)
{
    publishElementState("led1", on ? "on" : "off");
}

void MqttManager::publishLed2State(bool on)
{
    publishElementState("led2", on ? "on" : "off");
}

void MqttManager::publishDeviceState()
{
    const String name = getDeviceName != nullptr ? getDeviceName() : String(mqttClientId);
    publishElementState("device", name);
}

void MqttManager::publishStatusAndTemperature(bool relayOn)
{
    if (!mqttEnabled || !mqtt.connected())
    {
        return;
    }

    const char *relayStatus = relayOn ? "ON" : "OFF";
    const String statusPayload = String("{\"relay\":\"") + relayStatus + "\",\"uptime\":" + String(millis() / 1000) + "}";
    mqtt.publish(topicStatus.c_str(), statusPayload.c_str(), true);

    if (getProbePresent != nullptr && getCurrentProbeRaw != nullptr && getCurrentProbeTempC != nullptr)
    {
        const bool probePresent = getProbePresent();
        if (probePresent)
        {
            const int raw = getCurrentProbeRaw();
            const float tempC = getCurrentProbeTempC();
            const String tempPayload = String("{\"adc_raw\":") + String(raw) + String(",\"temp_c\":") + String(tempC, 2) + "}";
            mqtt.publish(topicTemp.c_str(), tempPayload.c_str(), true);
        }
    }
}

void MqttManager::handleMessage(char *topic, byte *payload, unsigned int length)
{
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; ++i)
    {
        message += static_cast<char>(payload[i]);
    }

    const String topicStr(topic);
    const String prefix = topicRoot + "/";
    if (!topicStr.startsWith(prefix))
    {
        return;
    }

    const String suffix = topicStr.substring(prefix.length());
    const int sep = suffix.lastIndexOf('/');
    if (sep <= 0 || sep >= suffix.length() - 1)
    {
        return;
    }

    String element = suffix.substring(0, sep);
    String operation = suffix.substring(sep + 1);
    element.trim();
    operation.trim();
    element.toLowerCase();
    operation.toLowerCase();

    if (operationHandler != nullptr)
    {
        operationHandler(element, operation, message);
    }

    if (operation == "set")
    {
        if (element == "led1" && setLed1State != nullptr)
        {
            bool on = false;
            if (parseOnOff(message, on) && setLed1State(on))
            {
                publishLed1State(on);
            }
            return;
        }

        if (element == "led2" && setLed2State != nullptr)
        {
            bool on = false;
            if (parseOnOff(message, on) && setLed2State(on))
            {
                publishLed2State(on);
            }
            return;
        }

        return;
    }

    if (operation == "get" || operation == "state")
    {
        if (element == "relay" && getRelayState != nullptr)
        {
            publishRelayState(getRelayState());
            return;
        }

        if (element == "led1" && getLed1State != nullptr)
        {
            publishLed1State(getLed1State());
            return;
        }

        if (element == "led2" && getLed2State != nullptr)
        {
            publishLed2State(getLed2State());
            return;
        }

        if (element == "device")
        {
            publishDeviceState();
            return;
        }
    }
}

void MqttManager::connectIfNeeded(bool relayOn)
{
    if (mqtt.connected())
    {
        mqtt.loop();
        return;
    }

    const unsigned long now = millis();
    if (now - lastMqttRetry < MQTT_RETRY_MS)
    {
        return;
    }

    lastMqttRetry = now;

    const bool useCredentials = mqttUser.length() > 0;
    const bool connected = useCredentials
                               ? mqtt.connect(
                                     mqttClientId.c_str(),
                                     mqttUser.c_str(),
                                     mqttPass.c_str(),
                                     topicAvail.c_str(),
                                     1,
                                     true,
                                     "offline")
                               : mqtt.connect(
                                     mqttClientId.c_str(),
                                     topicAvail.c_str(),
                                     1,
                                     true,
                                     "offline");

    if (connected)
    {
        const bool subscribed = mqtt.subscribe(topicOpsWildcard.c_str());

        mqtt.publish(topicAvail.c_str(), "online", true);
        publishRelayState(relayOn);
        if (getLed1State != nullptr)
        {
            publishLed1State(getLed1State());
        }
        if (getLed2State != nullptr)
        {
            publishLed2State(getLed2State());
        }
        publishDeviceState();
        publishStatusAndTemperature(relayOn);
        lastTelemetryPublish = millis();
        mqtt.loop();

        if (debugLogging)
        {
            Serial.println("==================================");
            Serial.println("MQTT connected");
            Serial.print("Broker: ");
            Serial.print(mqttHost);
            Serial.print(":");
            Serial.println(mqttPort);
            Serial.print("Subscribed: ");
            Serial.println(topicOpsWildcard);
            if (!subscribed)
            {
                Serial.println("Warning: MQTT subscription failed.");
            }
            Serial.println("==================================");
        }
        return;
    }

    if (debugLogging)
    {
        Serial.println("==================================");
        Serial.print("MQTT connect failed, rc=");
        Serial.println(mqtt.state());
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
