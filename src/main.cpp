/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include <Arduino.h>
#include <ESPmDNS.h>
#include <UnifiedMdns.h>
#include "RelayManifest.h"
#include <Preferences.h>
#include <WiFi.h>
#include <string.h>
#include <esp_bt.h>
#include <esp_system.h>
#include <ctype.h>
#include <time.h>
#include <esp32-hal-cpu.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "ButtonManager.h"
#include "CommandRouter.h"
#include "DeviceCommands.h"
#include "OtaUpdateManager.h"
#include "RelayController.h"
#include "ResetDiagnostics.h"
#include "ScheduleManager.h"
#include "TimeSyncManager.h"
#include "TemperatureProbeManager.h"
#include "TemperatureCalibrationRecord.h"
#include "WebControlServer.h"
#include "UnifiedServerClient.h"
#include "WiFiManager.h"
#include "discovery/espnow_discovery_bridge.h"
#include "discovery/udp_discovery.h"

bool debugLogging = false;

RelayController relayController;
WiFiManager wifiManager;
ButtonManager buttonManager;
CommandRouter commandRouter;
DeviceCommandContext commandContext;
WebControlServer webControlServer;
UdpDiscovery udpDiscovery;
EspNowDiscoveryBridge espNowDiscoveryBridge;
TimeSyncManager timeSyncManager;
ScheduleManager scheduleManager;
OtaUpdateManager otaUpdateManager;
TemperatureProbeManager temperatureProbeManager;
UnifiedServerClient unifiedServerClient;

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
DeviceResetReason bootResetReason = DeviceResetReason::Unknown;
uint32_t brownoutCount = 0;
bool rebootPending = false;
unsigned long rebootAtMs = 0;
bool bootOtaCheckPending = true;
bool bootOtaCheckAttempted = false;

namespace
{
    constexpr char DEVICE_PREF_NAMESPACE[] = "device_cfg";
    constexpr char DEVICE_PREF_HOSTNAME[] = "hostname";
    constexpr char DEVICE_PREF_OTA_AUTO_SCHEDULE[] = "ota_auto_sched";
    constexpr char RESET_DIAG_PREF_NAMESPACE[] = "reset_diag";
    constexpr char RESET_DIAG_PREF_BROWNOUT_COUNT[] = "brownout_count";
    constexpr unsigned long SERIAL_IDLE_SUBMIT_MS = 1200;
    constexpr size_t SERIAL_MAX_COMMAND_LEN = 128;

    DeviceResetReason classifyEspResetReason(const esp_reset_reason_t reason)
    {
        switch (reason)
        {
        case ESP_RST_BROWNOUT: return DeviceResetReason::Brownout;
        case ESP_RST_POWERON: return DeviceResetReason::PowerOn;
        case ESP_RST_SW: return DeviceResetReason::Software;
        case ESP_RST_EXT: return DeviceResetReason::External;
        case ESP_RST_PANIC: return DeviceResetReason::Panic;
        case ESP_RST_DEEPSLEEP: return DeviceResetReason::DeepSleep;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT: return DeviceResetReason::Watchdog;
        default: return DeviceResetReason::Unknown;
        }
    }

    uint32_t recordBrownoutCount(const DeviceResetReason reason)
    {
        Preferences preferences;
        if (!preferences.begin(RESET_DIAG_PREF_NAMESPACE, false))
        {
            Serial.println("[NVS] Warning: reset diagnostics preferences unavailable.");
            return 0;
        }

        uint32_t brownoutCount = preferences.getULong(RESET_DIAG_PREF_BROWNOUT_COUNT, 0);
        if (reason == DeviceResetReason::Brownout && brownoutCount < UINT32_MAX)
        {
            ++brownoutCount;
            preferences.putULong(RESET_DIAG_PREF_BROWNOUT_COUNT, brownoutCount);
        }
        preferences.end();
        return brownoutCount;
    }

    const char *temperatureStorageSlotName(const char *slot)
    {
        if (slot != nullptr && strcmp(slot, "cal_a") == 0) return "cal_a";
        if (slot != nullptr && strcmp(slot, "cal_b") == 0) return "cal_b";
        if (slot != nullptr && strcmp(slot, "cal_v2") == 0) return "cal_v2";
        if (slot != nullptr && strcmp(slot, "legacy_keys") == 0) return "legacy_keys";
        return "none";
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
String getNvsHealth();
void disableBluetooth();
bool updateDeviceHostname(const String &requested, String &error);
void maintainMdns();
void loadDeviceHostname();
void saveDeviceHostname(const String &hostname);
String getDiscoveryDeviceName();
String getDiscoveryDeviceId();
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
bool getTemperatureMonitoringEnabled();
bool setTemperatureMonitoringEnabled(bool enabled, String &error);
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
const char *getResetReason();
uint32_t getBrownoutCount();
bool getTemperatureStorageLoadOk();
bool getTemperatureStorageWriteVerified();
const char *getTemperatureStorageSlot();
uint8_t getTemperatureStorageGeneration();
bool captureLowCalibration(float knownTempC, String &error);
bool captureHighCalibration(float knownTempC, String &error);
bool resetTemperatureCalibration(String &error);
bool setTemperatureTrimOffsetC(float offsetC, String &error);
bool captureTempLowFromSaved();
bool captureTempHighFromSaved();
String getUnifiedRegistrationJson();
String getUnifiedCalibrationJson();
String getUnifiedStateJson();
bool handleUnifiedCommand(bool desiredOn, String &error);
String getUnifiedSettingsJson();
bool handleUnifiedSettings(JsonObjectConst values, String &error);
bool handleUnifiedCalibration(JsonObjectConst values, String &error);
void scheduleUnifiedRestart();
bool configureUnifiedServer(const String &serverUrl, String &error);
void handleUnifiedServerDiscovered(const String &serverUrl, const String &websocketUrl);
bool getRelayIsOn();
bool getDiscoveryWifiConnected();
int getDiscoveryWifiRssi();
void handleEspNowPeerPayload(const String &payload);
bool handleEspNowOtaCheck(String &latestVersion, bool &updateAvailable, String &message);
bool handleEspNowOtaUpdate(String &message);
void handleEspNowOtaResult(const String &commandId, bool install, bool ok, bool updateAvailable,
                           const String &latestVersion, const String &message);
bool triggerEspNowOta(bool install, const String &targetDeviceId, String &commandId, String &error);
void requestRebootCommand();
void maybeRunBootOtaCheck();

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

    // GPIO4 is pulled high internally; a fitted jumper to GND enables debug.
    debugLogging = digitalRead(DEBUG_JUMPER_PIN) == LOW;

    Serial.print("Debug jumper GPIO");
    Serial.print(DEBUG_JUMPER_PIN);
    Serial.print(": ");
    Serial.println(debugLogging ? "FITTED - DEBUG ON" : "NOT FITTED - DEBUG OFF");
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
    udpDiscovery.advertiseNow();
    espNowDiscoveryBridge.advertiseNow();
    unifiedServerClient.publishState();
}

void disableBluetooth()
{
    const esp_err_t btMemRelease = esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    if (btMemRelease == ESP_OK)
    {
        Serial.println("[BT] Bluetooth memory released (BLE disabled).");
        return;
    }

    if (btMemRelease == ESP_ERR_INVALID_STATE)
    {
        Serial.println("[BT] Bluetooth stack already active; memory release skipped.");
        return;
    }

    Serial.print("[BT] Bluetooth disable request returned error: ");
    Serial.println(btMemRelease);
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

bool clearUserSettings()
{
    constexpr const char *USER_SETTING_NAMESPACES[] = {
        "device_cfg",
        "wifi_cfg",
        "mqtt_cfg",
        "relay_cfg",
        "time_cfg",
        "sched_cfg",
        "led_cfg",
        "temp_probe"};

    bool success = true;

    for (const char *settingsNamespace : USER_SETTING_NAMESPACES)
    {
        Preferences preferences;
        if (!preferences.begin(settingsNamespace, false))
        {
            Serial.print("[NVS] Failed to open settings namespace: ");
            Serial.println(settingsNamespace);
            success = false;
            continue;
        }

        if (!preferences.clear())
        {
            Serial.print("[NVS] Failed to clear settings namespace: ");
            Serial.println(settingsNamespace);
            success = false;
        }
        preferences.end();
    }

    return success;
}

void handleFactoryResetButtonHold()
{
    Serial.println("BOOT button hold confirmed. Resetting saved user settings...");
    const bool resetSucceeded = clearUserSettings();
    Serial.println(resetSucceeded ? "User settings reset. Restarting ESP32..." : "Some settings could not be reset. Restarting ESP32...");
    delay(100);
    ESP.restart();
}

void handleResetButtonPress()
{
    Serial.println("Reset button pressed. Restarting ESP32...");
    delay(100);
    ESP.restart();
}

void requestRebootCommand()
{
    rebootPending = true;
    rebootAtMs = millis() + 250UL;
    Serial.println("Reboot command accepted. Restarting...");
}

void maybeRunBootOtaCheck()
{
    if (!bootOtaCheckPending || bootOtaCheckAttempted)
    {
        return;
    }

    if (!wifiManager.isConnected())
    {
        return;
    }

    bootOtaCheckAttempted = true;
    const OtaCheckResult result = otaUpdateManager.checkForUpdate();

    Serial.print("[BOOT OTA CHECK] ");
    Serial.println(result.ok ? "ok" : "failed");
    if (result.latestVersion.length() > 0)
    {
        Serial.print("[BOOT OTA CHECK] latest=");
        Serial.println(result.latestVersion);
    }
    Serial.print("[BOOT OTA CHECK] update_available=");
    Serial.println(result.updateAvailable ? "yes" : "no");
    Serial.print("[BOOT OTA CHECK] message=");
    Serial.println(result.message);

    bootOtaCheckPending = false;
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

String getNvsHealth()
{
    String health = "device=";
    health += deviceHostnameNvsReady ? "ok" : "default";
    health += ", wifi=";
    health += wifiManager.nvsReady() ? "ok" : "default";
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
            kjunified::advertise(MDNS, 80);
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
    return getUnifiedStateJson();
}

String getDiscoveryCapabilitiesJson()
{
    return "[\"relay\",\"http\",\"websocket\",\"status\",\"toggle\",\"temperature\",\"calibration\",\"settings\",\"restart\"]";
}

String getUnifiedStateJson()
{
    String state = "{\"hostname\":\"" + deviceHostname + "\",\"relay\":\"";
    state += relayController.isOn() ? "on" : "off";
    state += "\",\"temperature_probe_present\":";
    state += temperatureProbeManager.isPresent() ? "true" : "false";
    state += ",\"temperature_monitoring_enabled\":";
    state += temperatureProbeManager.isEnabled() ? "true" : "false";
    state += ",\"temperature_probe_raw\":" + String(temperatureProbeManager.rawReading());
    state += ",\"current_temperature_raw\":" + String(temperatureProbeManager.currentTemperatureRaw());
    state += ",\"current_temperature_c\":";
    const float temperatureC = temperatureProbeManager.currentTemperatureC();
    state += isnan(temperatureC) ? "null" : String(temperatureC, 2);
    state += ",\"temperature_calibration_ready\":";
    state += temperatureProbeManager.calibrationReady() ? "true" : "false";
    state += ",\"temperature_calibration_low_valid\":";
    state += temperatureProbeManager.lowPointValid() ? "true" : "false";
    state += ",\"temperature_calibration_low_raw\":" + String(temperatureProbeManager.lowPointRaw());
    state += ",\"temperature_calibration_low_c\":";
    state += temperatureProbeManager.lowPointValid() ? String(temperatureProbeManager.lowPointTempC(), 2) : "null";
    state += ",\"temperature_calibration_high_valid\":";
    state += temperatureProbeManager.highPointValid() ? "true" : "false";
    state += ",\"temperature_calibration_high_raw\":" + String(temperatureProbeManager.highPointRaw());
    state += ",\"temperature_calibration_high_c\":";
    state += temperatureProbeManager.highPointValid() ? String(temperatureProbeManager.highPointTempC(), 2) : "null";
    state += ",\"temperature_calibration_record_version\":" + String(TEMPERATURE_CALIBRATION_VERSION);
    state += ",\"temperature_trim_offset_c\":" + String(temperatureProbeManager.trimOffsetC(), 2);
    state += ",\"reset_reason\":\"";
    state += deviceResetReasonName(bootResetReason);
    state += "\"";
    state += ",\"brownout_count\":" + String(brownoutCount);
    state += ",\"relay_boot_failsafe_applied\":";
    state += relayController.bootFailsafeApplied() ? "true" : "false";
    state += ",\"temperature_probe_stable\":";
    state += temperatureProbeManager.isPresent() ? "true" : "false";
    state += ",\"temperature_storage_load_ok\":";
    state += temperatureProbeManager.storageLoadOk() ? "true" : "false";
    state += ",\"temperature_storage_write_verified\":";
    state += temperatureProbeManager.lastStorageWriteVerified() ? "true" : "false";
    state += ",\"temperature_storage_slot\":\"";
    state += temperatureStorageSlotName(temperatureProbeManager.selectedStorageSlot());
    state += "\"";
    state += ",\"temperature_storage_generation\":" + String(temperatureProbeManager.calibrationGenerationValue());
    state += ",\"rssi\":" + String(WiFi.RSSI());
    state += ",\"uptime_ms\":" + String(millis()) + "}";
    return state;
}

String getUnifiedCalibrationJson()
{
    String json = "{\"record_version\":" + String(TEMPERATURE_CALIBRATION_VERSION);
    json += ",\"ready\":";
    json += temperatureProbeManager.calibrationReady() ? "true" : "false";
    json += ",\"low\":{\"valid\":";
    json += temperatureProbeManager.lowPointValid() ? "true" : "false";
    json += ",\"raw\":" + String(temperatureProbeManager.lowPointRaw()) + ",\"temp_c\":";
    json += temperatureProbeManager.lowPointValid() ? String(temperatureProbeManager.lowPointTempC(), 4) : "null";
    json += "},\"high\":{\"valid\":";
    json += temperatureProbeManager.highPointValid() ? "true" : "false";
    json += ",\"raw\":" + String(temperatureProbeManager.highPointRaw()) + ",\"temp_c\":";
    json += temperatureProbeManager.highPointValid() ? String(temperatureProbeManager.highPointTempC(), 4) : "null";
    json += "},\"trim_offset_c\":" + String(temperatureProbeManager.trimOffsetC(), 4);
    json += ",\"monitoring_enabled\":";
    json += temperatureProbeManager.isEnabled() ? "true" : "false";
    json += "}";
    return json;
}

// Snapshot existing managers only. Discovery must not enroll, write NVS or execute commands.
String getUnifiedManifestJson()
{
    RelayManifestSnapshot snapshot;
    snapshot.hardwareId = kjunified::factoryHardwareId();
    snapshot.name = deviceHostname.c_str();
    String legacyMac = WiFi.macAddress();
    legacyMac.toLowerCase();
    legacyMac.replace(":", "");
    snapshot.legacyDeviceId = ("esp32-" + deviceHostname + "-" + legacyMac).c_str();
    snapshot.firmwareVersion = FIRMWARE_VERSION;
    snapshot.firmwareReleaseDate = FIRMWARE_RELEASE_DATE;
    snapshot.relayOn = relayController.isOn();
    snapshot.autoOffMinutes = relayController.autoOffMinutes();
    snapshot.probePresent = temperatureProbeManager.isPresent();
    snapshot.monitoringEnabled = temperatureProbeManager.isEnabled();
    snapshot.calibrationReady = temperatureProbeManager.calibrationReady();
    snapshot.temperatureC = temperatureProbeManager.currentTemperatureC();
    snapshot.rawTemperature = temperatureProbeManager.rawReading();
    snapshot.lowValid = temperatureProbeManager.lowPointValid();
    snapshot.highValid = temperatureProbeManager.highPointValid();
    snapshot.lowRaw = temperatureProbeManager.lowPointRaw();
    snapshot.highRaw = temperatureProbeManager.highPointRaw();
    snapshot.lowC = temperatureProbeManager.lowPointTempC();
    snapshot.highC = temperatureProbeManager.highPointTempC();
    snapshot.trimC = temperatureProbeManager.trimOffsetC();
    snapshot.relayPin = RELAY_PIN;
    snapshot.temperaturePin = TEMP_PROBE_ADC_PIN;
    snapshot.wifiConnected = wifiManager.isConnected();
    snapshot.timeValid = timeSyncManager.isTimeValid();
    snapshot.uptimeSeconds = (millis() - bootTime) / 1000;
    auto manifest = buildRelayManifest(snapshot);
    kjunified::Text body, error;
    if (!manifest.serialize(body, error)) return String();
    return body;
}

String getUnifiedRegistrationJson()
{
    String mac = WiFi.macAddress();
    mac.toLowerCase();
    mac.replace(":", "");
    const String deviceId = "esp32-" + deviceHostname + "-" + mac;
    const String ip = WiFi.localIP().toString();
    String registration = "{\"device_id\":\"" + deviceId + "\",\"device_name\":\"" + deviceHostname;
    registration += "\",\"device_type\":\"relay\",\"firmware\":{\"name\":\"" + String(FIRMWARE_NAME);
    registration += "\",\"version\":\"" + String(FIRMWARE_VERSION) + "\",\"release_date\":\"" + String(FIRMWARE_RELEASE_DATE) + "\"}";
    registration += ",\"configuration_key\":\"relay:" + deviceHostname + "\",\"network\":{\"ip\":\"" + ip;
    registration += "\",\"base_url\":\"http://" + ip + "/\"},\"advanced_url\":\"http://" + ip + "/\"";
    registration += ",\"capabilities\":" + getDiscoveryCapabilitiesJson() + ",\"state\":" + getUnifiedStateJson() + "}";
    return registration;
}

bool handleUnifiedCommand(bool desiredOn, String &error)
{
    (void)error;
    if (desiredOn)
    {
        relayController.set(true);
    }
    else
    {
        relayController.set(false);
    }
    return relayController.isOn() == desiredOn;
}

String getUnifiedSettingsJson()
{
    String json = "{\"revision\":1,\"values\":{\"relay_auto_off_minutes\":";
    json += String(relayController.autoOffMinutes());
    json += ",\"temperature_monitoring_enabled\":";
    json += temperatureProbeManager.isEnabled() ? "true" : "false";
    json += ",\"temperature_trim_offset_c\":" + String(temperatureProbeManager.trimOffsetC(), 2);
    json += ",\"ota_auto_schedule_enabled\":";
    json += otaAutoScheduleEnabled ? "true" : "false";
    json += "},\"schema\":{";
    json += "\"relay_auto_off_minutes\":{\"type\":\"integer\",\"label\":\"Auto-off timer\",\"description\":\"Minutes after switching on before the relay turns itself off. Use 0 to disable.\",\"unit\":\"minutes\",\"min\":0,\"max\":10080,\"step\":1},";
    json += "\"temperature_monitoring_enabled\":{\"type\":\"boolean\",\"label\":\"Temperature monitoring\",\"description\":\"Keep the configured analogue temperature probe active.\"},";
    json += "\"temperature_trim_offset_c\":{\"type\":\"number\",\"label\":\"Temperature trim\",\"description\":\"Fine adjustment only; this does not replace two-reference calibration.\",\"unit\":\"°C\",\"min\":-20,\"max\":20,\"step\":0.1},";
    json += "\"ota_auto_schedule_enabled\":{\"type\":\"boolean\",\"label\":\"Weekly OTA check\",\"description\":\"Keep the existing Monday 10:30 firmware update check scheduled.\"}";
    json += "},\"calibration\":{\"ready\":";
    json += temperatureProbeManager.calibrationReady() ? "true" : "false";
    json += ",\"message\":\"Two-reference capture is available in the native console; complete calibrations are backed up by Unified Server.\",\"record\":";
    json += getUnifiedCalibrationJson();
    json += "}";
    json += "}";
    return json;
}

bool handleUnifiedCalibration(JsonObjectConst values, String &error)
{
    const JsonObjectConst low = values["low"].as<JsonObjectConst>();
    const JsonObjectConst high = values["high"].as<JsonObjectConst>();
    if (low.isNull() || high.isNull() || !low["valid"].is<bool>() || !high["valid"].is<bool>() ||
        !low["raw"].is<int>() || !high["raw"].is<int>() ||
        (!low["temp_c"].is<float>() && !low["temp_c"].is<int>()) ||
        (!high["temp_c"].is<float>() && !high["temp_c"].is<int>()))
    {
        error = "Calibration payload is incomplete";
        return false;
    }
    return temperatureProbeManager.restoreCalibration(
        low["valid"].as<bool>(), low["raw"].as<int>(), low["temp_c"].as<float>(),
        high["valid"].as<bool>(), high["raw"].as<int>(), high["temp_c"].as<float>(),
        values["trim_offset_c"] | 0.0f, values["monitoring_enabled"] | true, error);
}

bool handleUnifiedSettings(JsonObjectConst values, String &error)
{
    const JsonVariantConst autoOff = values["relay_auto_off_minutes"];
    const JsonVariantConst temperatureEnabled = values["temperature_monitoring_enabled"];
    const JsonVariantConst temperatureTrim = values["temperature_trim_offset_c"];
    const JsonVariantConst otaSchedule = values["ota_auto_schedule_enabled"];
    const bool hasAutoOff = !autoOff.isNull();
    const bool hasTemperatureEnabled = !temperatureEnabled.isNull();
    const bool hasTemperatureTrim = !temperatureTrim.isNull();
    const bool hasOtaSchedule = !otaSchedule.isNull();

    if (hasAutoOff)
    {
        if (!autoOff.is<int>() || autoOff.as<int>() < 0 || autoOff.as<int>() > 10080)
        {
            error = "relay_auto_off_minutes must be between 0 and 10080";
            return false;
        }
    }
    if (hasTemperatureEnabled && !temperatureEnabled.is<bool>())
    {
        error = "temperature_monitoring_enabled must be boolean";
        return false;
    }
    if (hasTemperatureTrim)
    {
        if ((!temperatureTrim.is<float>() && !temperatureTrim.is<int>()) ||
            temperatureTrim.as<float>() < -20.0f || temperatureTrim.as<float>() > 20.0f)
        {
            error = "temperature_trim_offset_c must be between -20 and 20";
            return false;
        }
    }
    if (hasOtaSchedule && !otaSchedule.is<bool>())
    {
        error = "ota_auto_schedule_enabled must be boolean";
        return false;
    }

    if (hasAutoOff && !setRelayAutoOffMinutes(autoOff.as<int>(), error)) return false;
    if (hasTemperatureEnabled && !setTemperatureMonitoringEnabled(temperatureEnabled.as<bool>(), error)) return false;
    if (hasTemperatureTrim && !setTemperatureTrimOffsetC(temperatureTrim.as<float>(), error)) return false;
    if (hasOtaSchedule && !setOtaAutoScheduleEnabled(otaSchedule.as<bool>(), error)) return false;
    return true;
}

bool unifiedRestartPending = false;
unsigned long unifiedRestartAtMs = 0;

void scheduleUnifiedRestart()
{
    unifiedRestartPending = true;
    unifiedRestartAtMs = millis() + 1000UL;
}

bool configureUnifiedServer(const String &serverUrl, String &error)
{
    return unifiedServerClient.configureFromServerUrl(serverUrl, error);
}

void handleUnifiedServerDiscovered(const String &serverUrl, const String &websocketUrl)
{
    (void)serverUrl;
    String error;
    if (!unifiedServerClient.configureFromWebSocketUrl(websocketUrl, error) && debugLogging)
    {
        Serial.println("[UNIFIED] Discovery rejected: " + error);
    }
}

String getDiscoveryModel()
{
    return "esp32-c3";
}

String getDiscoveryDeviceId()
{
    String mac = WiFi.macAddress();
    mac.toLowerCase();
    mac.replace(":", "");

    String host = getDeviceHostname();
    host.toLowerCase();

    String deviceId = "esp32-";
    deviceId += host;
    deviceId += "-";
    deviceId += mac;
    return deviceId;
}

bool getRelayIsOn()
{
    return relayController.isOn();
}

bool getDiscoveryWifiConnected()
{
    return wifiManager.isConnected();
}

int getDiscoveryWifiRssi()
{
    if (!wifiManager.isConnected())
    {
        return 0;
    }
    return WiFi.RSSI();
}

void handleEspNowPeerPayload(const String &payload)
{
    if (!wifiManager.isConnected())
    {
        return;
    }

    JsonDocument peerDoc;
    const DeserializationError err = deserializeJson(peerDoc, payload);
    if (err)
    {
        return;
    }

    JsonDocument proxiedDoc;
    proxiedDoc["protocol"] = "kj-esp-discovery";
    proxiedDoc["protocol_version"] = 1;
    proxiedDoc["transport"] = "udp-proxy";
    proxiedDoc["via"] = "espnow";
    proxiedDoc["proxied"] = true;
    proxiedDoc["gateway_hostname"] = getDeviceHostname();
    proxiedDoc["gateway_ip"] = WiFi.localIP().toString();
    proxiedDoc["peer"] = peerDoc.as<JsonVariantConst>();

    String proxiedPayload;
    serializeJson(proxiedDoc, proxiedPayload);
    udpDiscovery.advertisePeerPayload(proxiedPayload);
}

bool handleEspNowOtaCheck(String &latestVersion, bool &updateAvailable, String &message)
{
    const OtaCheckResult result = otaUpdateManager.checkForUpdate();
    latestVersion = result.latestVersion;
    updateAvailable = result.updateAvailable;
    message = result.message;
    return result.ok;
}

bool handleEspNowOtaUpdate(String &message)
{
    return otaUpdateManager.performUpdate(message);
}

void handleEspNowOtaResult(const String &commandId, const bool install, const bool ok,
                           const bool updateAvailable, const String &latestVersion, const String &message)
{
    Serial.print("[ESPNOW OTA RESULT] id=");
    Serial.print(commandId);
    Serial.print(" type=");
    Serial.print(install ? "ota_update" : "ota_check");
    Serial.print(" ok=");
    Serial.print(ok ? "true" : "false");
    Serial.print(" update_available=");
    Serial.print(updateAvailable ? "yes" : "no");
    if (latestVersion.length() > 0)
    {
        Serial.print(" latest=");
        Serial.print(latestVersion);
    }
    Serial.print(" message=");
    Serial.println(message);

    if (install && ok)
    {
        Serial.println("[ESPNOW OTA] Update installed; rebooting device.");
        delay(150);
        ESP.restart();
    }
}

bool triggerEspNowOta(const bool install, const String &targetDeviceId, String &commandId, String &error)
{
    if (!espNowDiscoveryBridge.ready())
    {
        error = "ESPNOW bridge is not ready";
        return false;
    }

    return espNowDiscoveryBridge.sendOtaCommand(install, targetDeviceId, commandId, error, 2);
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
                kjunified::advertise(MDNS, 80);
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

bool getTemperatureMonitoringEnabled()
{
    return temperatureProbeManager.isEnabled();
}

bool setTemperatureMonitoringEnabled(bool enabled, String &error)
{
    return temperatureProbeManager.setEnabled(enabled, error);
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

const char *getResetReason()
{
    return deviceResetReasonName(bootResetReason);
}

uint32_t getBrownoutCount()
{
    return brownoutCount;
}

bool getTemperatureStorageLoadOk()
{
    return temperatureProbeManager.storageLoadOk();
}

bool getTemperatureStorageWriteVerified()
{
    return temperatureProbeManager.lastStorageWriteVerified();
}

const char *getTemperatureStorageSlot()
{
    return temperatureStorageSlotName(temperatureProbeManager.selectedStorageSlot());
}

uint8_t getTemperatureStorageGeneration()
{
    return temperatureProbeManager.calibrationGenerationValue();
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
    Serial.println();
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
    Serial.println();
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
    bootResetReason = classifyEspResetReason(esp_reset_reason());
    Serial.begin(115200);
    delay(2500);

    brownoutCount = recordBrownoutCount(bootResetReason);
    Serial.printf("[RESET] reason=%s brownout_count=%lu\n", deviceResetReasonName(bootResetReason),
                  static_cast<unsigned long>(brownoutCount));

    const bool cpuReduced = setCpuFrequencyMhz(80);
    Serial.printf("[POWER] CPU frequency=%u MHz reduced=%s deep_sleep=disabled light_sleep=disabled\n",
                  static_cast<unsigned>(getCpuFrequencyMhz()), cpuReduced ? "true" : "false");

    setupDebugLogging();

    bootTime = millis();

    Serial.println();
    Serial.println("ESP32-C3 RELAY WIFI");
    Serial.print("Firmware version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("Debug logging: ");
    Serial.println(debugLogging ? "ON" : "OFF");
    Serial.println("Type 'help' for available commands.");

    loadDeviceHostname();
    disableBluetooth();

    buttonManager.begin();
    buttonManager.setRelayButtonCallback(handleRelayButtonPress);
    buttonManager.setResetButtonCallback(handleResetButtonPress);
    buttonManager.setFactoryResetCallback(handleFactoryResetButtonHold);

    relayController.begin(shouldForceRelayOff(bootResetReason));
    relayController.setStateChangedCallback(onRelayStateChanged);

    commandContext.relay = &relayController;
    commandContext.wifi = &wifiManager;
    commandContext.ota = &otaUpdateManager;
    commandContext.printStatus = printStatus;
    commandContext.requestReboot = requestRebootCommand;
    commandContext.captureTempLow = captureTempLowFromSaved;
    commandContext.captureTempHigh = captureTempHighFromSaved;

    DeviceCommands::begin(commandRouter, commandContext);

    WebControlContext webContext;
    webContext.router = &commandRouter;
    webContext.relay = &relayController;
    webContext.wifi = &wifiManager;
    webContext.timeSync = &timeSyncManager;
    webContext.schedule = &scheduleManager;
    webContext.ota = &otaUpdateManager;
    webContext.getHostname = getDeviceHostname;
    webContext.getManifest = getUnifiedManifestJson;
    webContext.setHostname = updateDeviceHostname;
    webContext.getNvsHealth = getNvsHealth;
    webContext.getOtaAutoScheduleEnabled = getOtaAutoScheduleEnabled;
    webContext.setOtaAutoScheduleEnabled = setOtaAutoScheduleEnabled;
    webContext.getRelayAutoOffMinutes = getRelayAutoOffMinutes;
    webContext.setRelayAutoOffMinutes = setRelayAutoOffMinutes;
    webContext.getRelayAutoOffArmed = getRelayAutoOffArmed;
    webContext.getRelayAutoOffRemainingSeconds = getRelayAutoOffRemainingSeconds;
    webContext.getTemperatureProbePresent = getTemperatureProbePresent;
    webContext.getTemperatureMonitoringEnabled = getTemperatureMonitoringEnabled;
    webContext.setTemperatureMonitoringEnabled = setTemperatureMonitoringEnabled;
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
    webContext.getResetReason = getResetReason;
    webContext.getBrownoutCount = getBrownoutCount;
    webContext.getTemperatureStorageLoadOk = getTemperatureStorageLoadOk;
    webContext.getTemperatureStorageWriteVerified = getTemperatureStorageWriteVerified;
    webContext.getTemperatureStorageSlot = getTemperatureStorageSlot;
    webContext.getTemperatureStorageGeneration = getTemperatureStorageGeneration;
    webContext.captureLowCalibration = captureLowCalibration;
    webContext.captureHighCalibration = captureHighCalibration;
    webContext.resetTemperatureCalibration = resetTemperatureCalibration;
    webContext.setTemperatureTrimOffsetC = setTemperatureTrimOffsetC;
    webContext.setUnifiedServer = configureUnifiedServer;
    webContext.triggerEspNowOta = triggerEspNowOta;
    webControlServer.configure(webContext);

    if (debugLogging)
    {
        wifiManager.scanWifi();
    }

    WiFi.setHostname(deviceHostname.c_str());
    wifiManager.begin();

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
    discoveryConfig.unifiedServerDiscovered = handleUnifiedServerDiscovered;
    udpDiscovery.begin(discoveryConfig);

    EspNowDiscoveryBridgeConfig espNowConfig;
    espNowConfig.advertiseIntervalMs = 7000;
    espNowConfig.deviceIdProvider = getDiscoveryDeviceId;
    espNowConfig.deviceNameProvider = getDiscoveryDeviceName;
    espNowConfig.hostnameProvider = getDeviceHostname;
    espNowConfig.firmwareVersionProvider = getDiscoveryFirmwareVersion;
    espNowConfig.relayOnProvider = getRelayIsOn;
    espNowConfig.wifiConnectedProvider = getDiscoveryWifiConnected;
    espNowConfig.wifiRssiProvider = getDiscoveryWifiRssi;
    espNowConfig.peerPayloadReceived = handleEspNowPeerPayload;
    espNowConfig.otaCheckHandler = handleEspNowOtaCheck;
    espNowConfig.otaUpdateHandler = handleEspNowOtaUpdate;
    espNowConfig.otaResultReceived = handleEspNowOtaResult;
    espNowDiscoveryBridge.begin(espNowConfig);

    unifiedServerClient.begin(getUnifiedRegistrationJson, getUnifiedStateJson, handleUnifiedCommand,
                              getUnifiedSettingsJson, handleUnifiedSettings,
                              getUnifiedCalibrationJson, handleUnifiedCalibration, scheduleUnifiedRestart);

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
    temperatureProbeManager.maintain(millis());
    udpDiscovery.loop(wifiManager.isConnected());
    espNowDiscoveryBridge.loop();
    unifiedServerClient.maintain(wifiManager.isConnected());
    maybeRunBootOtaCheck();
    if (unifiedRestartPending && static_cast<long>(millis() - unifiedRestartAtMs) >= 0)
    {
        ESP.restart();
    }
    if (rebootPending && static_cast<long>(millis() - rebootAtMs) >= 0)
    {
        ESP.restart();
    }

    if (wifiManager.isConnected() && !lastDiscoveryWifiConnected)
    {
        udpDiscovery.advertiseNow();
        espNowDiscoveryBridge.advertiseNow();
    }

    lastDiscoveryWifiConnected = wifiManager.isConnected();
}
