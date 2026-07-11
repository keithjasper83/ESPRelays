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
#include <time.h>

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
#include "TemperatureProbeManager.h"
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
TemperatureProbeManager temperatureProbeManager;

unsigned long lastHeartbeat = 0;
unsigned long lastTimestampLog = 0;
unsigned long bootTime = 0;
unsigned long lastSerialInputMs = 0;
String serialBuffer;
String deviceHostname = DEVICE_HOSTNAME_DEFAULT;
bool mdnsStarted = false;
bool lastDiscoveryWifiConnected = false;
bool deviceHostnameNvsReady = false;
bool otaAutoScheduleEnabled = true;
bool otaAutoScheduleNvsReady = false;

namespace
{
    constexpr char DEVICE_PREF_NAMESPACE[] = "device_cfg";
    constexpr char DEVICE_PREF_HOSTNAME[] = "hostname";
    constexpr char DEVICE_PREF_OTA_AUTO_SCHEDULE[] = "ota_auto_sched";
    constexpr unsigned long LED_TEST_DURATION_MS = 5000;
    constexpr unsigned long SERIAL_IDLE_SUBMIT_MS = 1200;
    constexpr size_t SERIAL_MAX_COMMAND_LEN = 128;

    bool parseOnOff(const String &payload, bool &on)
    {
        String normalized = payload;
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
void printTimestampLine();
void applyWeeklyOtaUpdateSchedule();
bool getOtaAutoScheduleEnabled();
bool setOtaAutoScheduleEnabled(bool enabled, String &error);
int getRelayAutoOffMinutes();
bool setRelayAutoOffMinutes(int minutes, String &error);
bool getRelayAutoOffArmed();
long getRelayAutoOffRemainingSeconds();
bool getTemperatureProbePresent();
int getTemperatureProbeRaw();
int getCurrentTemperatureRaw();
float getCurrentTemperatureC();
bool getTemperatureCalibrationReady();
bool getLowCalibrationValid();
bool getHighCalibrationValid();
int getLowCalibrationRaw();
int getHighCalibrationRaw();
float getLowCalibrationTempC();
float getHighCalibrationTempC();
float getTemperatureTrimOffsetC();
bool startRelayLedTest();
bool startWifiLedTest();
bool startAllLedTests();
bool getRelayLedTestActive();
bool getWifiLedTestActive();
bool getLedActiveHigh();
bool setLedActiveHigh(bool activeHigh, String &error);
bool captureLowCalibration(float knownTempC, String &error);
bool captureHighCalibration(float knownTempC, String &error);
bool resetTemperatureCalibration(String &error);
bool setTemperatureTrimOffsetC(float offsetC, String &error);
bool captureTempLowFromSaved();
bool captureTempHighFromSaved();
void handleMqttOperation(const String &element, const String &operation, const String &payload);
bool mqttSetLed1(bool on);
bool mqttGetLed1();
bool mqttSetLed2(bool on);
bool mqttGetLed2();
bool mqttGetRelay();
String mqttGetDeviceName();

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
        otaAutoScheduleEnabled = true;
        otaAutoScheduleNvsReady = false;
        return;
    }

    deviceHostname = preferences.getString(DEVICE_PREF_HOSTNAME, DEVICE_HOSTNAME_DEFAULT);
    deviceHostnameNvsReady = true;

    otaAutoScheduleEnabled = preferences.isKey(DEVICE_PREF_OTA_AUTO_SCHEDULE)
                                 ? preferences.getBool(DEVICE_PREF_OTA_AUTO_SCHEDULE, true)
                                 : true;
    otaAutoScheduleNvsReady = true;

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

    Serial.print("Relay auto-off (minutes): ");
    Serial.println(relayController.autoOffMinutes());
    Serial.print("Relay auto-off armed: ");
    Serial.println(relayController.autoOffArmed() ? "yes" : "no");
    Serial.print("Relay auto-off remaining (s): ");
    Serial.println(relayController.autoOffRemainingSeconds());
}

String getDeviceHostname()
{
    return deviceHostname;
}

int getRelayAutoOffMinutes()
{
    return relayController.autoOffMinutes();
}

bool setRelayAutoOffMinutes(int minutes, String &error)
{
    return relayController.setAutoOffMinutes(minutes, error);
}

bool getRelayAutoOffArmed()
{
    return relayController.autoOffArmed();
}

long getRelayAutoOffRemainingSeconds()
{
    return relayController.autoOffRemainingSeconds();
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
    health += ", ota_sched=";
    health += otaAutoScheduleNvsReady ? "ok" : "default";
    return health;
}

bool getOtaAutoScheduleEnabled()
{
    return otaAutoScheduleEnabled;
}

bool setOtaAutoScheduleEnabled(bool enabled, String &error)
{
    Preferences preferences;
    if (!preferences.begin(DEVICE_PREF_NAMESPACE, false))
    {
        otaAutoScheduleNvsReady = false;
        error = "Failed to open device settings";
        return false;
    }

    if (!preferences.putBool(DEVICE_PREF_OTA_AUTO_SCHEDULE, enabled))
    {
        preferences.end();
        otaAutoScheduleNvsReady = false;
        error = "Failed to persist OTA auto schedule setting";
        return false;
    }

    preferences.end();

    otaAutoScheduleEnabled = enabled;
    otaAutoScheduleNvsReady = true;
    applyWeeklyOtaUpdateSchedule();
    return true;
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

void handleMqttOperation(const String &element, const String &operation, const String &payload)
{
    if (element == "relay" && operation == "set")
    {
        bool on = false;
        if (parseOnOff(payload, on))
        {
            relayController.set(on);
        }
        return;
    }

    if (element == "relay" && (operation == "get" || operation == "state"))
    {
        mqttManager.publishRelayState(relayController.isOn());
        return;
    }
}

bool mqttSetLed1(bool on)
{
    return indicatorLeds.setRelayLed(on);
}

bool mqttGetLed1()
{
    return indicatorLeds.relayLedState();
}

bool mqttSetLed2(bool on)
{
    return indicatorLeds.setWifiLed(on);
}

bool mqttGetLed2()
{
    return indicatorLeds.wifiLedState();
}

bool mqttGetRelay()
{
    return relayController.isOn();
}

String mqttGetDeviceName()
{
    return deviceHostname;
}

bool startRelayLedTest()
{
    return indicatorLeds.startRelayLedTest(millis(), LED_TEST_DURATION_MS);
}

bool startWifiLedTest()
{
    return indicatorLeds.startWifiLedTest(millis(), LED_TEST_DURATION_MS);
}

bool startAllLedTests()
{
    return indicatorLeds.startAllLedTests(millis(), LED_TEST_DURATION_MS);
}

bool getRelayLedTestActive()
{
    return indicatorLeds.relayLedTestActive();
}

bool getWifiLedTestActive()
{
    return indicatorLeds.wifiLedTestActive();
}

bool getLedActiveHigh()
{
    return indicatorLeds.isActiveHigh();
}

bool setLedActiveHigh(bool activeHigh, String &error)
{
    if (!indicatorLeds.setActiveHigh(activeHigh))
    {
        error = "Failed to persist LED polarity";
        return false;
    }

    return true;
}

void handleSerial()
{
    auto submitSerialBuffer = []()
    {
        if (serialBuffer.length() == 0)
        {
            return;
        }

        handleCommand(serialBuffer);
        serialBuffer = "";
    };

    while (Serial.available())
    {
        const char c = static_cast<char>(Serial.read());
        lastSerialInputMs = millis();

        if (c == '\n' || c == '\r' || c == '\0')
        {
            submitSerialBuffer();
            continue;
        }

        if (c == '\b' || static_cast<unsigned char>(c) == 127)
        {
            if (serialBuffer.length() > 0)
            {
                serialBuffer.remove(serialBuffer.length() - 1);
            }
            continue;
        }

        if (static_cast<unsigned char>(c) < 32)
        {
            continue;
        }

        if (serialBuffer.length() < SERIAL_MAX_COMMAND_LEN)
        {
            serialBuffer += c;
        }
    }

    if (serialBuffer.length() > 0 && (millis() - lastSerialInputMs) >= SERIAL_IDLE_SUBMIT_MS)
    {
        submitSerialBuffer();
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

    if (normalized.startsWith("temp-") && !temperatureProbeManager.shouldRunTemperatureDependentFunctions())
    {
        if (debugLogging)
        {
            Serial.println("Skipping temperature-dependent command (probe not detected).");
        }
        return true;
    }

    return commandRouter.dispatch(normalized);
}

bool getTemperatureProbePresent()
{
    return temperatureProbeManager.isPresent();
}

int getTemperatureProbeRaw()
{
    return temperatureProbeManager.rawReading();
}

int getCurrentTemperatureRaw()
{
    return temperatureProbeManager.currentTemperatureRaw();
}

float getCurrentTemperatureC()
{
    return temperatureProbeManager.currentTemperatureC();
}

bool getTemperatureCalibrationReady()
{
    return temperatureProbeManager.calibrationReady();
}

bool getLowCalibrationValid()
{
    return temperatureProbeManager.lowPointValid();
}

bool getHighCalibrationValid()
{
    return temperatureProbeManager.highPointValid();
}

int getLowCalibrationRaw()
{
    return temperatureProbeManager.lowPointRaw();
}

int getHighCalibrationRaw()
{
    return temperatureProbeManager.highPointRaw();
}

float getLowCalibrationTempC()
{
    return temperatureProbeManager.lowPointTempC();
}

float getHighCalibrationTempC()
{
    return temperatureProbeManager.highPointTempC();
}

float getTemperatureTrimOffsetC()
{
    return temperatureProbeManager.trimOffsetC();
}

bool captureLowCalibration(float knownTempC, String &error)
{
    return temperatureProbeManager.captureLow(knownTempC, error);
}

bool captureHighCalibration(float knownTempC, String &error)
{
    return temperatureProbeManager.captureHigh(knownTempC, error);
}

bool resetTemperatureCalibration(String &error)
{
    return temperatureProbeManager.resetCalibration(error);
}

bool setTemperatureTrimOffsetC(float offsetC, String &error)
{
    return temperatureProbeManager.setTrimOffsetC(offsetC, error);
}

bool captureTempLowFromSaved()
{
    String error;
    const bool ok = temperatureProbeManager.captureLowUsingSavedTemp(error);
    if (!ok)
    {
        Serial.print("Low temperature capture failed: ");
        Serial.println(error);
    }
    return ok;
}

bool captureTempHighFromSaved()
{
    String error;
    const bool ok = temperatureProbeManager.captureHighUsingSavedTemp(error);
    if (!ok)
    {
        Serial.print("High temperature capture failed: ");
        Serial.println(error);
    }
    return ok;
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

void printTimestampLine()
{
    const unsigned long nowMs = millis();
    if (nowMs - lastTimestampLog < 60000)
    {
        return;
    }

    lastTimestampLog = nowMs;

    const time_t now = time(nullptr);
    Serial.print("[STATUS] version=");
    Serial.print(FIRMWARE_VERSION);
    Serial.print(" | ");

    if (now <= 0)
    {
        Serial.print("time=unsynced");
    }
    else
    {
        struct tm localTime;
        if (localtime_r(&now, &localTime))
        {
            char buffer[48];
            if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &localTime) > 0)
            {
                Serial.print("time=");
                Serial.print(buffer);
            }
            else
            {
                Serial.print("time_epoch=");
                Serial.print(static_cast<unsigned long>(now));
            }
        }
        else
        {
            Serial.print("time_epoch=");
            Serial.print(static_cast<unsigned long>(now));
        }
    }

    Serial.print(" | uptime=");
    Serial.print((nowMs - bootTime) / 1000);
    Serial.print("s | relay=");
    Serial.print(relayController.isOn() ? "ON" : "OFF");
    Serial.print(" | wifi=");
    Serial.print(WiFiManager::statusName(wifiManager.status()));
    Serial.print(" | mqtt=");
    Serial.println(mqttManager.isConnected() ? "CONNECTED" : "DISCONNECTED");
}

void applyWeeklyOtaUpdateSchedule()
{
    ScheduleManager::EventData weeklyOta;
    weeklyOta.enabled = true;
    weeklyOta.recurring = true;
    weeklyOta.hour = 10;
    weeklyOta.minute = 30;
    weeklyOta.dowMask = 0x01; // Monday (bit0)
    weeklyOta.command = "ota-update";

    uint8_t eventId = 0;
    bool changed = false;
    String error;

    if (otaAutoScheduleEnabled)
    {
        if (!scheduleManager.ensureRecurringEvent(weeklyOta, eventId, changed, error))
        {
            Serial.print("[SCHEDULE] Weekly OTA schedule apply failed: ");
            Serial.println(error.length() > 0 ? error : String("unknown error"));
            return;
        }

        Serial.print("[SCHEDULE] Weekly OTA auto-update enabled (Monday 10:30, event #");
        Serial.print(eventId);
        Serial.print(") ");
        Serial.println(changed ? "created" : "already present");
        return;
    }

    if (!scheduleManager.removeRecurringEvent(weeklyOta, eventId, changed, error))
    {
        Serial.print("[SCHEDULE] Weekly OTA schedule cleanup failed: ");
        Serial.println(error.length() > 0 ? error : String("unknown error"));
        return;
    }

    if (changed)
    {
        Serial.print("[SCHEDULE] Weekly OTA auto-update disabled; removed event #");
        Serial.println(eventId);
    }
    else
    {
        Serial.println("[SCHEDULE] Weekly OTA auto-update disabled; no event found");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2500);

    setupDebugLogging();

    bootTime = millis();

    Serial.println();
    Serial.println("ESP32-C3 RELAY WIFI MQTT");
    Serial.print("Firmware version: ");
    Serial.println(FIRMWARE_VERSION);
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
    commandContext.captureTempLow = captureTempLowFromSaved;
    commandContext.captureTempHigh = captureTempHighFromSaved;

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
    webContext.getOtaAutoScheduleEnabled = getOtaAutoScheduleEnabled;
    webContext.setOtaAutoScheduleEnabled = setOtaAutoScheduleEnabled;
    webContext.getRelayAutoOffMinutes = getRelayAutoOffMinutes;
    webContext.setRelayAutoOffMinutes = setRelayAutoOffMinutes;
    webContext.getRelayAutoOffArmed = getRelayAutoOffArmed;
    webContext.getRelayAutoOffRemainingSeconds = getRelayAutoOffRemainingSeconds;
    webContext.getTemperatureProbePresent = getTemperatureProbePresent;
    webContext.getTemperatureProbeRaw = getTemperatureProbeRaw;
    webContext.getCurrentTemperatureRaw = getCurrentTemperatureRaw;
    webContext.getCurrentTemperatureC = getCurrentTemperatureC;
    webContext.getTemperatureCalibrationReady = getTemperatureCalibrationReady;
    webContext.getLowCalibrationValid = getLowCalibrationValid;
    webContext.getHighCalibrationValid = getHighCalibrationValid;
    webContext.getLowCalibrationRaw = getLowCalibrationRaw;
    webContext.getHighCalibrationRaw = getHighCalibrationRaw;
    webContext.getLowCalibrationTempC = getLowCalibrationTempC;
    webContext.getHighCalibrationTempC = getHighCalibrationTempC;
    webContext.getTemperatureTrimOffsetC = getTemperatureTrimOffsetC;
    webContext.captureLowCalibration = captureLowCalibration;
    webContext.captureHighCalibration = captureHighCalibration;
    webContext.resetTemperatureCalibration = resetTemperatureCalibration;
    webContext.setTemperatureTrimOffsetC = setTemperatureTrimOffsetC;
    webContext.startRelayLedTest = startRelayLedTest;
    webContext.startWifiLedTest = startWifiLedTest;
    webContext.startAllLedTests = startAllLedTests;
    webContext.getRelayLedTestActive = getRelayLedTestActive;
    webContext.getWifiLedTestActive = getWifiLedTestActive;
    webContext.getLedActiveHigh = getLedActiveHigh;
    webContext.setLedActiveHigh = setLedActiveHigh;
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
    mqttManager.setOperationHandler(handleMqttOperation);
    mqttManager.setElementHandlers(mqttGetRelay, mqttSetLed1, mqttGetLed1, mqttSetLed2, mqttGetLed2, mqttGetDeviceName);
    mqttManager.setTemperatureTelemetryGetters(getTemperatureProbePresent, getTemperatureProbeRaw, getCurrentTemperatureRaw, getCurrentTemperatureC);

    temperatureProbeManager.begin();

    timeSyncManager.begin();
    scheduleManager.begin();
    applyWeeklyOtaUpdateSchedule();
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
    relayController.maintain(millis());
    maintainMdns();
    webControlServer.beginIfNeeded(wifiManager.isConnected());
    webControlServer.handleClient();
    mqttManager.maintain(wifiManager.isConnected(), relayController.isOn());
    temperatureProbeManager.maintain(millis());
    udpDiscovery.loop(wifiManager.isConnected());

    if (wifiManager.isConnected() && !lastDiscoveryWifiConnected)
    {
        udpDiscovery.advertiseNow();
    }

    lastDiscoveryWifiConnected = wifiManager.isConnected();

    indicatorLeds.update(millis(), relayController.isOn(), wifiManager.isConnected());
    printTimestampLine();
    heartbeat();
}