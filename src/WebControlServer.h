/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

class CommandRouter;
class RelayController;
class WiFiManager;
class MqttManager;
class TimeSyncManager;
class ScheduleManager;
class OtaUpdateManager;

using HostnameGetter = String (*)();
using HostnameSetter = bool (*)(const String &hostname, String &error);
using MqttClientIdGetter = String (*)();
using NvsHealthGetter = String (*)();
using OtaAutoScheduleGetter = bool (*)();
using OtaAutoScheduleSetter = bool (*)(bool enabled, String &error);
using RelayAutoOffMinutesGetter = int (*)();
using RelayAutoOffMinutesSetter = bool (*)(int minutes, String &error);
using RelayAutoOffRemainingSecondsGetter = long (*)();
using RelayAutoOffArmedGetter = bool (*)();
using TemperatureProbePresentGetter = bool (*)();
using TemperatureProbeRawGetter = int (*)();
using TemperatureProbeFloatGetter = float (*)();
using TemperatureCaptureSetter = bool (*)(float knownTempC, String &error);
using TemperatureCalibrationAction = bool (*)(String &error);
using TemperatureTrimSetter = bool (*)(float offsetC, String &error);
using LedTestAction = bool (*)();
using LedTestStatusGetter = bool (*)();

struct WebControlContext
{
    CommandRouter *router = nullptr;
    RelayController *relay = nullptr;
    WiFiManager *wifi = nullptr;
    MqttManager *mqtt = nullptr;
    TimeSyncManager *timeSync = nullptr;
    ScheduleManager *schedule = nullptr;
    OtaUpdateManager *ota = nullptr;
    HostnameGetter getHostname = nullptr;
    HostnameSetter setHostname = nullptr;
    MqttClientIdGetter getMqttClientId = nullptr;
    NvsHealthGetter getNvsHealth = nullptr;
    OtaAutoScheduleGetter getOtaAutoScheduleEnabled = nullptr;
    OtaAutoScheduleSetter setOtaAutoScheduleEnabled = nullptr;
    RelayAutoOffMinutesGetter getRelayAutoOffMinutes = nullptr;
    RelayAutoOffMinutesSetter setRelayAutoOffMinutes = nullptr;
    RelayAutoOffArmedGetter getRelayAutoOffArmed = nullptr;
    RelayAutoOffRemainingSecondsGetter getRelayAutoOffRemainingSeconds = nullptr;
    TemperatureProbePresentGetter getTemperatureProbePresent = nullptr;
    TemperatureProbeRawGetter getTemperatureProbeRaw = nullptr;
    TemperatureProbeRawGetter getCurrentTemperatureRaw = nullptr;
    TemperatureProbeFloatGetter getCurrentTemperatureC = nullptr;
    TemperatureProbePresentGetter getTemperatureCalibrationReady = nullptr;
    TemperatureProbePresentGetter getLowCalibrationValid = nullptr;
    TemperatureProbePresentGetter getHighCalibrationValid = nullptr;
    TemperatureProbeRawGetter getLowCalibrationRaw = nullptr;
    TemperatureProbeRawGetter getHighCalibrationRaw = nullptr;
    TemperatureProbeFloatGetter getLowCalibrationTempC = nullptr;
    TemperatureProbeFloatGetter getHighCalibrationTempC = nullptr;
    TemperatureProbeFloatGetter getTemperatureTrimOffsetC = nullptr;
    TemperatureCaptureSetter captureLowCalibration = nullptr;
    TemperatureCaptureSetter captureHighCalibration = nullptr;
    TemperatureCalibrationAction resetTemperatureCalibration = nullptr;
    TemperatureTrimSetter setTemperatureTrimOffsetC = nullptr;
    LedTestAction startRelayLedTest = nullptr;
    LedTestAction startWifiLedTest = nullptr;
    LedTestAction startAllLedTests = nullptr;
    LedTestStatusGetter getRelayLedTestActive = nullptr;
    LedTestStatusGetter getWifiLedTestActive = nullptr;
};

class WebControlServer
{
public:
    void configure(const WebControlContext &context);
    void beginIfNeeded(bool wifiConnected);
    void handleClient();
    bool isStarted() const;

private:
    void registerRoutes();
    bool dispatchCommand(const char *command) const;
    const char *relayStateText() const;

    void handleRoot();
    void handleOn();
    void handleOff();
    void handleToggle();
    void handleStatus();
    void handleConfig();
    void handleWifiScan();
    void handleConfigSave();
    void handleTimeStatus();
    void handleTimeConfigSave();
    void handleTimeSyncNow();
    void handleScheduleAdd();
    void handleScheduleUpdate();
    void handleScheduleDelete();
    void handleOtaCheck();
    void handleOtaUpdate();
    void handleTemperatureStatus();
    void handleTemperatureCaptureLow();
    void handleTemperatureCaptureHigh();
    void handleTemperatureCalibrationReset();
    void handleTemperatureTrimOffset();
    void handleRelayLedTest();
    void handleWifiLedTest();
    void handleAllLedTests();
    void handleSetHostname();
    void handleNotFound();

    void sendError(int statusCode, const char *error);

    WebControlContext context = {};
    bool started = false;
};