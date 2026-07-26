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
    topicRelayState = topicRoot + "/relay/state";
    topicLed1State = topicRoot + "/led1/state";
    topicLed2State = topicRoot + "/led2/state";
    topicDeviceState = topicRoot + "/device/state";
    topicAvail = topicRoot + "/availability";
    topicStatus = topicRoot + "/status";
    topicTemp = topicRoot + "/temperature";

    const String matterbridgeDeviceId = "esp-relay-" + mqttClientId;
    const String matterbridgeRoot = String(MATTERBRIDGE_MQTT_TOPIC) + "/" + matterbridgeDeviceId;
    topicMatterbridgeConfig = matterbridgeRoot + "/config/root";
    topicMatterbridgeState = matterbridgeRoot + "/state/root";
    topicMatterbridgeSubscribe = matterbridgeRoot + "/subscribe/root";
    topicMatterbridgeWrite = matterbridgeRoot + "/write/root";
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

    (void)masterBrightness;
    (void)bootAnimation;
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
    if (clientId == mqttClientId)
    {
        return;
    }

    // Matterbridge treats an empty retained config as endpoint removal.  Clear
    // the old hostname-derived endpoint before publishing the replacement.
    if (mqtt.connected() && topicMatterbridgeConfig.length() > 0)
    {
        mqtt.publish(topicMatterbridgeConfig.c_str(), "", true);
        mqtt.disconnect();
    }

    mqttClientId = clientId;
    rebuildTopics();
    legacyHomeTopicsCleared = false;
    lastMqttRetry = 0;
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

void MqttManager::publishRelayState(bool relayOn)
{
    publishMatterbridgeRelay(relayOn);
    if (mqttEnabled && mqtt.connected())
    {
        Serial.print("[MQTT] TX ");
        Serial.print(topicMatterbridgeState);
        Serial.print(" => ");
        Serial.println(relayOn ? "{\"OnOff\":{\"onOff\":true}}" : "{\"OnOff\":{\"onOff\":false}}");
    }
}

void MqttManager::publishMatterbridgeMetadata()
{
    if (!mqttEnabled || !mqtt.connected() || mqttClientId.length() == 0)
    {
        return;
    }

    const String name = getDeviceName != nullptr ? getDeviceName() : mqttClientId;
    const String config = String("{\"deviceTypes\":[\"OnOffPlugInUnit\"],\"clusters\":{\"BridgedDeviceBasicInformation\":{\"nodeLabel\":\"") +
                          name + "\",\"serialNumber\":\"" + mqttClientId + "\"}}}";
    mqtt.publish(topicMatterbridgeConfig.c_str(), config.c_str(), true);
    mqtt.publish(topicMatterbridgeSubscribe.c_str(), "{\"OnOff\":[\"onOff\"]}", true);
}

void MqttManager::clearLegacyHomeRetainedTopics()
{
    if (legacyHomeTopicsCleared)
    {
        return;
    }

    const String legacyTopics[] = {
        topicRelayState, topicLed1State, topicLed2State, topicDeviceState,
        topicAvail, topicStatus, topicTemp};
    for (const String &topic : legacyTopics)
    {
        mqtt.publish(topic.c_str(), "", true);
    }

    legacyHomeTopicsCleared = true;
    Serial.println("[MQTT] Cleared legacy retained /home topics.");
}

void MqttManager::publishMatterbridgeRelay(bool relayOn)
{
    if (!mqttEnabled || mqttClientId.length() == 0)
    {
        return;
    }

    if (!mqtt.connected())
    {
        // Preserve an actual state transition that occurred while MQTT was
        // unavailable; it is sent once after the next successful reconnect.
        matterbridgeStateDirty = true;
        return;
    }

    const String state = String("{\"OnOff\":{\"onOff\":") + (relayOn ? "true" : "false") + "}}";
    mqtt.publish(topicMatterbridgeState.c_str(), state.c_str(), true);
    matterbridgeStateDirty = false;
}

bool MqttManager::handleMatterbridgeWrite(const String &topic, const String &message)
{
    if (topic != topicMatterbridgeWrite)
    {
        return false;
    }

    const int onOffIndex = message.indexOf("\"onOff\"");
    const int colonIndex = onOffIndex < 0 ? -1 : message.indexOf(':', onOffIndex);
    if (colonIndex >= 0 && operationHandler != nullptr)
    {
        String value = message.substring(colonIndex + 1);
        value.trim();
        bool on = false;
        if (value.startsWith("true"))
        {
            on = true;
            operationHandler("relay", "set", "on");
        }
        else if (value.startsWith("false"))
        {
            operationHandler("relay", "set", "off");
        }
        else if (parseOnOff(value, on))
        {
            operationHandler("relay", "set", on ? "on" : "off");
        }
    }
    return true;
}

void MqttManager::publishLed1State(bool on)
{
    (void)on;
}

void MqttManager::publishLed2State(bool on)
{
    (void)on;
}

void MqttManager::publishDeviceState()
{
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
    Serial.print("[MQTT] RX ");
    Serial.print(topicStr);
    Serial.print(" => ");
    Serial.println(message);

    if (handleMatterbridgeWrite(topicStr, message))
    {
        return;
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
                               ? mqtt.connect(mqttClientId.c_str(), mqttUser.c_str(), mqttPass.c_str())
                               : mqtt.connect(mqttClientId.c_str());

    if (connected)
    {
        const bool matterbridgeSubscribed = mqtt.subscribe(topicMatterbridgeWrite.c_str());

        clearLegacyHomeRetainedTopics();
        publishMatterbridgeMetadata();
        if (matterbridgeStateDirty)
        {
            publishMatterbridgeRelay(relayOn);
        }
        mqtt.loop();

        Serial.println("[MQTT] Subscribed command topic:");
        Serial.print("  ");
        Serial.println(topicMatterbridgeWrite);
        if (!matterbridgeSubscribed)
        {
            Serial.println("[MQTT] Warning: one or more subscriptions failed.");
        }

        if (debugLogging)
        {
            Serial.println("==================================");
            Serial.println("MQTT connected");
            Serial.print("Broker: ");
            Serial.print(mqttHost);
            Serial.print(":");
            Serial.println(mqttPort);
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

}
