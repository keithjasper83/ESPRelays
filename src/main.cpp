/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ctype.h>

#include "AppConfig.h"
#include "ButtonManager.h"
#include "CommandRouter.h"
#include "DeviceCommands.h"
#include "IndicatorLeds.h"
#include "MqttManager.h"
#include "OtaUpdateManager.h"
#include "RelayController.h"
#include "ScheduleManager.h"
#include "TimeSyncManager.h"
#include "WebControlServer.h"
#include "WiFiManager.h"
#include "discovery/udp_discovery.h"

bool debugLogging = false;

RelayController relayController;
IndicatorLeds indicatorLeds;
WiFiManager wifiManager;
MqttManager mqttManager;
ButtonManager buttonManager;
CommandRouter commandRouter;
DeviceCommandContext commandContext;
WebControlServer webControlServer;
UdpDiscovery udpDiscovery;
TimeSyncManager timeSyncManager;
ScheduleManager scheduleManager;
OtaUpdateManager otaUpdateManager;

unsigned long lastHeartbeat = 0;
unsigned long bootTime = 0;
String serialBuffer;
String deviceHostname = DEVICE_HOSTNAME_DEFAULT;
bool mdnsStarted = false;
bool lastDiscoveryWifiConnected = false;
bool deviceHostnameNvsReady = false;

namespace
{
    constexpr char DEVICE_PREF_NAMESPACE[] = "device_cfg";
    constexpr char DEVICE_PREF_HOSTNAME[] = "hostname";
}

const DiscoveryEndpoint DISCOVERY_ENDPOINTS[] = {
    {"ui", "GET", "/"},
    {"status", "GET", "/status"},
    {"on", "POST", "/on"},
    {"off", "POST", "/off"},
    {"toggle", "POST", "/toggle"},
    {"hostname", "POST", "/hostname"},
};

void printStatus();
String getDeviceHostname();
String getMqttClientId();
String getNvsHealth();
bool updateDeviceHostname(const String &requested, String &error);
void maintainMdns();
void loadDeviceHostname();
void saveDeviceHostname(const String &hostname);
String getDiscoveryDeviceName();
String getDiscoveryDeviceType();
String getDiscoveryFirmwareName();
String getDiscoveryFirmwareVersion();
String getDiscoveryStateJson();
String getDiscoveryCapabilitiesJson();
String getDiscoveryModel();
bool dispatchScheduledCommand(const String &command);

String sanitizeHostname(const String &requested)
{
    String clean;
    clean.reserve(requested.length());

    for (size_t i = 0; i < requested.length(); i++)
    {
        const char c = static_cast<char>(tolower(requested[i]));
        const bool alphaNum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (alphaNum || c == '-')
        {
            clean += c;
        }
    }

    return clean;
}

void setupDebugLogging()
{
    pinMode(DEBUG_JUMPER_PIN, INPUT_PULLUP);
    delay(50);

    debugLogging = digitalRead(DEBUG_JUMPER_PIN) == LOW;

    Serial.print("Debug jumper GPIO");
    Serial.print(DEBUG_JUMPER_PIN);
    Serial.print(": ");
    Serial.println(debugLogging ? "FITTED - DEBUG ON" : "OPEN - DEBUG OFF");
}

void debugLine(const char *msg)
{
    if (debugLogging)
    {
        Serial.println(msg);
    }
}

void debugPrint(const char *msg)
{
    if (debugLogging)
    {
        Serial.print(msg);
    }
}

void onRelayStateChanged(bool state)
{
    mqttManager.publishRelayState(state);
    udpDiscovery.advertiseNow();
}

void loadDeviceHostname()
{
    Preferences preferences;
    if (!preferences.begin(DEVICE_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: device hostname preferences unavailable; using default hostname.");
        deviceHostname = DEVICE_HOSTNAME_DEFAULT;
        deviceHostnameNvsReady = false;
        return;
    }

    deviceHostname = preferences.getString(DEVICE_PREF_HOSTNAME, DEVICE_HOSTNAME_DEFAULT);
    deviceHostnameNvsReady = true;
    preferences.end();

    if (deviceHostname.length() == 0)
    {
        deviceHostname = DEVICE_HOSTNAME_DEFAULT;
    }
}

void saveDeviceHostname(const String &hostname)
{
    Preferences preferences;
    if (!preferences.begin(DEVICE_PREF_NAMESPACE, false))
    {
        Serial.println("[NVS] Warning: failed to save device hostname.");
        deviceHostnameNvsReady = false;
        return;
    }

    preferences.putString(DEVICE_PREF_HOSTNAME, hostname);
    deviceHostnameNvsReady = true;
    preferences.end();
}

void handleRelayButtonPress()
{
    relayController.toggle();
}

void handleResetButtonPress()
{
    Serial.println("Reset button pressed. Restarting ESP32...");
    delay(100);
    ESP.restart();
}

void printStatus()
{
    Serial.print("Relay: ");
    Serial.println(relayController.isOn() ? "ON" : "OFF");

    Serial.print("WiFi: ");
    Serial.print(WiFiManager::statusName(wifiManager.status()));
    Serial.print(" / ");
    Serial.println(wifiManager.status());

    if (wifiManager.isConnected())
    {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    }

    Serial.print("MQTT: ");
    Serial.print(mqttManager.isConnected() ? "CONNECTED" : "DISCONNECTED");
    Serial.print(" / ");
    Serial.println(MqttManager::stateName(mqttManager.state()));

    Serial.print("Debug logging: ");
    Serial.println(debugLogging ? "ON" : "OFF");

    Serial.print("Hostname: ");
    Serial.println(deviceHostname);
}

String getDeviceHostname()
{
    return deviceHostname;
}

String getMqttClientId()
{
    return mqttManager.clientId();
}

String getNvsHealth()
{
    String health = "device=";
    health += deviceHostnameNvsReady ? "ok" : "default";
    health += ", wifi=";
    health += wifiManager.nvsReady() ? "ok" : "default";
    health += ", mqtt=";
    health += mqttManager.nvsReady() ? "ok" : "default";
    return health;
}

bool updateDeviceHostname(const String &requested, String &error)
{
    String normalized = requested;
    normalized.trim();
    normalized.toLowerCase();
    if (normalized.endsWith(".local"))
    {
        normalized.remove(normalized.length() - 6);
    }

    const String clean = sanitizeHostname(normalized);
    if (clean.length() == 0)
    {
        error = "Hostname must contain letters, digits, or hyphens";
        return false;
    }

    if (clean.length() > 32)
    {
        error = "Hostname must be 32 characters or fewer";
        return false;
    }

    if (clean == deviceHostname)
    {
        return true;
    }

    deviceHostname = clean;
    saveDeviceHostname(deviceHostname);
    WiFi.setHostname(deviceHostname.c_str());
    mqttManager.setClientId(deviceHostname);

    Serial.print("Hostname updated to: ");
    Serial.println(deviceHostname);

    if (wifiManager.isConnected())
    {
        wifiManager.forceReconnect();

        if (mdnsStarted)
        {
            MDNS.end();
            mdnsStarted = false;
        }

        if (MDNS.begin(deviceHostname.c_str()))
        {
            MDNS.addService("http", "tcp", 80);
            mdnsStarted = true;
            Serial.print("mDNS restarted: http://");
            Serial.print(deviceHostname);
            Serial.println(".local");
        }
        else
        {
            error = "Hostname updated, but mDNS failed to restart";
            Serial.println("mDNS restart failed");
        }
    }

    udpDiscovery.advertiseNow();

    return true;
}

String getDiscoveryDeviceName()
{
    return DEVICE_NAME_DEFAULT;
}

String getDiscoveryDeviceType()
{
    return DEVICE_TYPE_DEFAULT;
}

String getDiscoveryFirmwareName()
{
    return FIRMWARE_NAME;
}

String getDiscoveryFirmwareVersion()
{
    return FIRMWARE_VERSION;
}

String getDiscoveryStateJson()
{
    String state = "{\"relay\":\"";
    state += relayController.isOn() ? "on" : "off";
    state += "\"}";
    return state;
}

String getDiscoveryCapabilitiesJson()
{
    return "[\"relay\",\"http\",\"status\",\"toggle\"]";
}

String getDiscoveryModel()
{
    return "esp32-c3";
}

void maintainMdns()
{
    if (wifiManager.isConnected())
    {
        if (!mdnsStarted)
        {
            if (MDNS.begin(deviceHostname.c_str()))
            {
                MDNS.addService("http", "tcp", 80);
                mdnsStarted = true;
                Serial.print("mDNS started: http://");
                Serial.print(deviceHostname);
                Serial.println(".local");
            }
            else
            {
                Serial.println("mDNS start failed");
            }
        }

        return;
    }

    if (mdnsStarted)
    {
        MDNS.end();
        mdnsStarted = false;
        Serial.println("mDNS stopped (Wi-Fi disconnected)");
    }
}

void handleCommand(const String &cmd)
{
    String normalized = cmd;
    normalized.trim();
    normalized.toLowerCase();

    if (normalized.length() == 0)
    {
        return;
    }

    if (debugLogging)
    {
        Serial.print("Command received: ");
        Serial.println(normalized);
    }

    if (commandRouter.dispatch(normalized))
    {
        return;
    }

    Serial.println("Unknown command. Type 'help' to list commands.");
    commandRouter.printHelp(Serial);
}

void handleSerial()
{
    while (Serial.available())
    {
        const char c = static_cast<char>(Serial.read());

        if (c == '\n' || c == '\r')
        {
            if (serialBuffer.length() > 0)
            {
                handleCommand(serialBuffer);
                serialBuffer = "";
            }
        }
        else
        {
            serialBuffer += c;
        }
    }
}

bool dispatchScheduledCommand(const String &command)
{
    String normalized = command;
    normalized.trim();
    normalized.toLowerCase();

    if (normalized.length() == 0)
    {
        return false;
    }

    if (debugLogging)
    {
        Serial.print("Scheduled command received: ");
        Serial.println(normalized);
    }

    return commandRouter.dispatch(normalized);
}

void heartbeat()
{
    if (!debugLogging)
    {
        return;
    }

    const unsigned long now = millis();
    if (now - lastHeartbeat < 1000)
    {
        return;
    }

    lastHeartbeat = now;

    Serial.print("Alive ");
    Serial.print((now - bootTime) / 1000);
    Serial.print("s | Relay=");
    Serial.print(relayController.isOn() ? "ON" : "OFF");
    Serial.print(" | WiFi=");
    Serial.print(WiFiManager::statusName(wifiManager.status()));
    Serial.print("(");
    Serial.print(wifiManager.status());
    Serial.print(")");
    Serial.print(" | MQTT=");
    Serial.println(mqttManager.isConnected() ? "OK" : "NO");
}

void setup()
{
    Serial.begin(115200);
    delay(2500);

    setupDebugLogging();

    bootTime = millis();

    Serial.println();
    Serial.println("ESP32-C3 RELAY WIFI MQTT");
    Serial.print("Debug logging: ");
    Serial.println(debugLogging ? "ON" : "OFF");
    Serial.println("Type 'help' for available commands.");
    Serial.println("Relay LED follows relay state. Wi-Fi LED stays on when disconnected and blinks every five seconds when connected.");

    loadDeviceHostname();

    buttonManager.begin();
    buttonManager.setRelayButtonCallback(handleRelayButtonPress);
    buttonManager.setResetButtonCallback(handleResetButtonPress);

    relayController.begin();
    relayController.setStateChangedCallback(onRelayStateChanged);

    commandContext.relay = &relayController;
    commandContext.wifi = &wifiManager;
    commandContext.ota = &otaUpdateManager;
    commandContext.printStatus = printStatus;

    DeviceCommands::begin(commandRouter, commandContext);

    WebControlContext webContext;
    webContext.router = &commandRouter;
    webContext.relay = &relayController;
    webContext.wifi = &wifiManager;
    webContext.mqtt = &mqttManager;
    webContext.timeSync = &timeSyncManager;
    webContext.schedule = &scheduleManager;
    webContext.ota = &otaUpdateManager;
    webContext.getHostname = getDeviceHostname;
    webContext.setHostname = updateDeviceHostname;
    webContext.getMqttClientId = getMqttClientId;
    webContext.getNvsHealth = getNvsHealth;
    webControlServer.configure(webContext);

    indicatorLeds.begin();
    indicatorLeds.update(millis(), relayController.isOn(), wifiManager.isConnected());

    if (debugLogging)
    {
        wifiManager.scanWifi();
    }

    WiFi.setHostname(deviceHostname.c_str());
    wifiManager.begin();

    mqttManager.begin();
    mqttManager.setClientId(deviceHostname);
    mqttManager.setCommandHandler(handleCommand);

    timeSyncManager.begin();
    scheduleManager.begin();
    otaUpdateManager.begin();

    DiscoveryConfig discoveryConfig;
    discoveryConfig.multicastGroup = "239.255.42.99";
    discoveryConfig.port = 42424;
    discoveryConfig.advertiseIntervalMs = 60000;
    discoveryConfig.protocol = "kj-esp-discovery";
    discoveryConfig.protocolVersion = 1;
    discoveryConfig.httpPort = 80;
    discoveryConfig.deviceNameProvider = getDiscoveryDeviceName;
    discoveryConfig.deviceTypeProvider = getDiscoveryDeviceType;
    discoveryConfig.firmwareNameProvider = getDiscoveryFirmwareName;
    discoveryConfig.firmwareVersionProvider = getDiscoveryFirmwareVersion;
    discoveryConfig.hostnameProvider = getDeviceHostname;
    discoveryConfig.stateJsonProvider = getDiscoveryStateJson;
    discoveryConfig.capabilitiesJsonProvider = getDiscoveryCapabilitiesJson;
    discoveryConfig.modelProvider = getDiscoveryModel;
    discoveryConfig.endpoints = DISCOVERY_ENDPOINTS;
    discoveryConfig.endpointCount = sizeof(DISCOVERY_ENDPOINTS) / sizeof(DISCOVERY_ENDPOINTS[0]);
    udpDiscovery.begin(discoveryConfig);

    commandRouter.printHelp(Serial);
}

void loop()
{
    handleSerial();
    buttonManager.update(millis());
    wifiManager.maintain();
    timeSyncManager.maintain(wifiManager.isConnected());
    scheduleManager.maintain(timeSyncManager.isTimeValid(), dispatchScheduledCommand);
    maintainMdns();
    webControlServer.beginIfNeeded(wifiManager.isConnected());
    webControlServer.handleClient();
    mqttManager.maintain(wifiManager.isConnected(), relayController.isOn());
    udpDiscovery.loop(wifiManager.isConnected());

    if (wifiManager.isConnected() && !lastDiscoveryWifiConnected)
    {
        udpDiscovery.advertiseNow();
    }

    lastDiscoveryWifiConnected = wifiManager.isConnected();

    indicatorLeds.update(millis(), relayController.isOn(), wifiManager.isConnected());
    heartbeat();
}