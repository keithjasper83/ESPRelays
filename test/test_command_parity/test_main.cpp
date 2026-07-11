/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include <fstream>
#include <string>
#include <unity.h>

#include "Arduino.h"
#include "CommandRouter.h"
#include "DeviceCommands.h"
#include "OtaUpdateManager.h"
#include "RelayController.h"
#include "WiFiManager.h"

#include "../../src/CommandRouter.cpp"
#include "../../src/DeviceCommands.cpp"

namespace
{
    bool gRelayState = false;
    int gRelaySetCalls = 0;
    int gRelayToggleCalls = 0;
    int gWifiDetailsCalls = 0;
    int gWifiScanCalls = 0;
    int gWifiReconnectCalls = 0;
    int gOtaCheckCalls = 0;
    int gOtaUpdateCalls = 0;
    int gStatusCalls = 0;
    int gTempLowCaptureCalls = 0;
    int gTempHighCaptureCalls = 0;

    bool gPerformUpdateResult = false;

    bool readFileText(const char *path, std::string &out)
    {
        std::ifstream input(path, std::ios::in | std::ios::binary);
        if (!input.is_open())
        {
            return false;
        }

        out.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        return true;
    }

    std::string readWorkspaceFile(const char *relativePath)
    {
        static const char *const prefixes[] = {"", "../", "../../", "../../../"};
        std::string text;
        for (const char *prefix : prefixes)
        {
            const std::string candidate = std::string(prefix) + relativePath;
            if (readFileText(candidate.c_str(), text))
            {
                return text;
            }
        }

        return std::string();
    }

    void printStatusStub()
    {
        gStatusCalls++;
    }

    bool captureTempLowStub()
    {
        gTempLowCaptureCalls++;
        return true;
    }

    bool captureTempHighStub()
    {
        gTempHighCaptureCalls++;
        return true;
    }
}

void RelayController::begin() {}
void RelayController::maintain(unsigned long) {}
bool RelayController::isOn() const { return gRelayState; }
void RelayController::set(bool on)
{
    gRelayState = on;
    gRelaySetCalls++;
}
void RelayController::toggle()
{
    gRelayState = !gRelayState;
    gRelayToggleCalls++;
}
void RelayController::setStateChangedCallback(StateChangedCallback) {}
int RelayController::autoOffMinutes() const { return 0; }
bool RelayController::setAutoOffMinutes(int, String &) { return true; }
bool RelayController::autoOffArmed() const { return false; }
long RelayController::autoOffRemainingSeconds() const { return 0; }

void WiFiManager::begin() {}
void WiFiManager::maintain() {}
void WiFiManager::forceReconnect() { gWifiReconnectCalls++; }
void WiFiManager::setCredentials(const String &, const String &) {}
String WiFiManager::ssid() const { return "test"; }
bool WiFiManager::nvsReady() const { return true; }
void WiFiManager::printFullDetails() const { gWifiDetailsCalls++; }
void WiFiManager::scanWifi() const { gWifiScanCalls++; }
String WiFiManager::scanWifiJson() const { return "{}"; }
bool WiFiManager::isConnected() const { return true; }
wl_status_t WiFiManager::status() const { return WL_CONNECTED; }
const char *WiFiManager::statusName(wl_status_t) { return "WL_CONNECTED"; }

void OtaUpdateManager::begin() {}
OtaCheckResult OtaUpdateManager::checkForUpdate()
{
    gOtaCheckCalls++;
    OtaCheckResult result;
    result.ok = true;
    result.currentVersion = "1.0.0";
    result.latestVersion = "1.0.1";
    result.updateAvailable = true;
    return result;
}
bool OtaUpdateManager::performUpdate(String &message)
{
    gOtaUpdateCalls++;
    message = gPerformUpdateResult ? "ok" : "failed";
    return gPerformUpdateResult;
}
bool OtaUpdateManager::isConfigured() const { return true; }
bool OtaUpdateManager::lastCheckOk() const { return true; }
bool OtaUpdateManager::updateAvailable() const { return true; }
String OtaUpdateManager::currentVersion() const { return "1.0.0"; }
String OtaUpdateManager::latestVersion() const { return "1.0.1"; }
String OtaUpdateManager::lastMessage() const { return "ok"; }
String OtaUpdateManager::selectedAssetUrl() const { return "https://example.com/fw.bin"; }

void setUp()
{
    gRelayState = false;
    gRelaySetCalls = 0;
    gRelayToggleCalls = 0;
    gWifiDetailsCalls = 0;
    gWifiScanCalls = 0;
    gWifiReconnectCalls = 0;
    gOtaCheckCalls = 0;
    gOtaUpdateCalls = 0;
    gStatusCalls = 0;
    gTempLowCaptureCalls = 0;
    gTempHighCaptureCalls = 0;
    gPerformUpdateResult = false;
    ESP.restarted = false;
}

void tearDown()
{
}

void test_device_commands_dispatch_and_aliases_work()
{
    CommandRouter router;
    RelayController relay;
    WiFiManager wifi;
    OtaUpdateManager ota;

    DeviceCommandContext context;
    context.relay = &relay;
    context.wifi = &wifi;
    context.ota = &ota;
    context.printStatus = printStatusStub;
    context.captureTempLow = captureTempLowStub;
    context.captureTempHigh = captureTempHighStub;

    DeviceCommands::begin(router, context);

    TEST_ASSERT_EQUAL(12u, router.count());

    TEST_ASSERT_TRUE(router.dispatch("on"));
    TEST_ASSERT_TRUE(gRelayState);

    TEST_ASSERT_TRUE(router.dispatch("0"));
    TEST_ASSERT_FALSE(gRelayState);

    TEST_ASSERT_TRUE(router.dispatch("t"));
    TEST_ASSERT_EQUAL(1, gRelayToggleCalls);

    TEST_ASSERT_TRUE(router.dispatch("state"));
    TEST_ASSERT_EQUAL(1, gStatusCalls);

    TEST_ASSERT_TRUE(router.dispatch("wifi"));
    TEST_ASSERT_EQUAL(1, gWifiDetailsCalls);

    TEST_ASSERT_TRUE(router.dispatch("scan"));
    TEST_ASSERT_EQUAL(1, gWifiScanCalls);

    TEST_ASSERT_TRUE(router.dispatch("reconnect"));
    TEST_ASSERT_EQUAL(1, gWifiReconnectCalls);

    TEST_ASSERT_TRUE(router.dispatch("ota-check"));
    TEST_ASSERT_EQUAL(1, gOtaCheckCalls);

    TEST_ASSERT_TRUE(router.dispatch("update"));
    TEST_ASSERT_EQUAL(1, gOtaUpdateCalls);
    TEST_ASSERT_FALSE(ESP.restarted);

    TEST_ASSERT_TRUE(router.dispatch("help"));

    TEST_ASSERT_TRUE(router.dispatch("temp-capture-low"));
    TEST_ASSERT_EQUAL(1, gTempLowCaptureCalls);

    TEST_ASSERT_TRUE(router.dispatch("temp-capture-high"));
    TEST_ASSERT_EQUAL(1, gTempHighCaptureCalls);
}

void test_ota_update_success_restarts_device()
{
    CommandRouter router;
    RelayController relay;
    WiFiManager wifi;
    OtaUpdateManager ota;

    DeviceCommandContext context;
    context.relay = &relay;
    context.wifi = &wifi;
    context.ota = &ota;
    context.printStatus = printStatusStub;
    context.captureTempLow = captureTempLowStub;
    context.captureTempHigh = captureTempHighStub;

    DeviceCommands::begin(router, context);

    gPerformUpdateResult = true;
    TEST_ASSERT_TRUE(router.dispatch("ota-update"));
    TEST_ASSERT_TRUE(ESP.restarted);
}

void test_web_routes_and_mqtt_path_match_router_commands()
{
    const std::string web = readWorkspaceFile("src/WebControlServer.cpp");
    const std::string mqtt = readWorkspaceFile("src/MqttManager.cpp");
    const std::string mainText = readWorkspaceFile("src/main.cpp");

    TEST_ASSERT_FALSE(web.empty());
    TEST_ASSERT_FALSE(mqtt.empty());
    TEST_ASSERT_FALSE(mainText.empty());

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/on\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/off\", HTTP_POST"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("gServer.on(\"/toggle\", HTTP_POST"));

    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("dispatchCommand(\"on\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("dispatchCommand(\"off\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, web.find("dispatchCommand(\"toggle\")"));

    TEST_ASSERT_NOT_EQUAL(std::string::npos, mqtt.find("topicOpsWildcard = topicRoot + \"/+/+\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, mqtt.find("if (operation == \"set\")"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, mqtt.find("if (element == \"relay\" && (operation == \"get\" || operation == \"state\"))"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, mainText.find("mqttManager.setOperationHandler(handleMqttOperation);"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, mainText.find("mqttManager.setElementHandlers("));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_device_commands_dispatch_and_aliases_work);
    RUN_TEST(test_ota_update_success_restarts_device);
    RUN_TEST(test_web_routes_and_mqtt_path_match_router_commands);
    return UNITY_END();
}
