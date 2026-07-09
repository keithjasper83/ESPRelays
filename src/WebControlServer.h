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
    void handleSetHostname();
    void handleNotFound();

    void sendError(int statusCode, const char *error);

    WebControlContext context = {};
    bool started = false;
};