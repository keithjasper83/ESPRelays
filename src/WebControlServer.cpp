/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "WebControlServer.h"

#include <Arduino.h>
#include <WebServer.h>
#include <time.h>

#include "AppConfig.h"
#include "CommandRouter.h"
#include "MqttManager.h"
#include "OtaUpdateManager.h"
#include "RelayController.h"
#include "ScheduleManager.h"
#include "TimeSyncManager.h"
#include "WiFiManager.h"

namespace
{
  WebServer gServer(80);

  String formatLocalNow()
  {
    const time_t now = time(nullptr);
    if (now <= 0)
    {
      return String();
    }

    struct tm localTime;
    if (!localtime_r(&now, &localTime))
    {
      return String();
    }

    char buffer[48];
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &localTime) == 0)
    {
      return String();
    }

    return String(buffer);
  }

  String jsonEscape(const String &input)
  {
    String escaped;
    escaped.reserve(input.length() + 8);

    for (size_t i = 0; i < input.length(); i++)
    {
      const char c = input[i];
      if (c == '\\')
      {
        escaped += "\\\\";
      }
      else if (c == '"')
      {
        escaped += "\\\"";
      }
      else if (c == '\n')
      {
        escaped += "\\n";
      }
      else if (c == '\r')
      {
        escaped += "\\r";
      }
      else
      {
        escaped += c;
      }
    }

    return escaped;
  }

  String buildConfigJson(const WebControlContext &context)
  {
    String json = "{";
    json += "\"ok\":true,";
    json += "\"device_name\":\"";
    json += jsonEscape(DEVICE_NAME_DEFAULT);
    json += "\",";
    json += "\"device_type\":\"";
    json += jsonEscape(DEVICE_TYPE_DEFAULT);
    json += "\",";
    json += "\"firmware\":{\"name\":\"";
    json += jsonEscape(FIRMWARE_NAME);
    json += "\",\"version\":\"";
    json += jsonEscape(FIRMWARE_VERSION);
    json += "\"},";
    json += "\"hostname_default\":\"";
    json += jsonEscape(DEVICE_HOSTNAME_DEFAULT);
    json += "\",";
    json += "\"hostname\":\"";
    json += jsonEscape(context.getHostname != nullptr ? context.getHostname() : String(DEVICE_HOSTNAME_DEFAULT));
    json += "\",";
    json += "\"mdns_host\":\"";
    json += jsonEscape((context.getHostname != nullptr ? context.getHostname() : String(DEVICE_HOSTNAME_DEFAULT)) + String(".local"));
    json += "\",";
    json += "\"mqtt_host\":\"";
    json += jsonEscape(context.mqtt != nullptr ? context.mqtt->serverHost() : String(MQTT_HOST));
    json += "\",";
    json += "\"mqtt_port\":";
    json += context.mqtt != nullptr ? context.mqtt->serverPort() : MQTT_PORT;
    json += ",";
    json += "\"mqtt_enabled\":";
    json += context.mqtt != nullptr ? (context.mqtt->isEnabled() ? "true" : "false") : "true";
    json += ",";
    json += "\"mqtt_username\":\"";
    json += jsonEscape(context.mqtt != nullptr ? context.mqtt->username() : String(MQTT_USER));
    json += "\",";
    json += "\"mqtt_password_set\":";
    json += context.mqtt != nullptr ? (context.mqtt->passwordSet() ? "true" : "false") : (String(MQTT_PASS).length() > 0 ? "true" : "false");
    json += ",";
    json += "\"time_enabled\":";
    json += context.timeSync != nullptr ? (context.timeSync->isEnabled() ? "true" : "false") : "true";
    json += ",";
    json += "\"time_server\":\"";
    json += jsonEscape(context.timeSync != nullptr ? context.timeSync->server() : String(TIME_SERVER_DEFAULT));
    json += "\",";
    json += "\"time_timezone\":\"";
    json += jsonEscape(context.timeSync != nullptr ? context.timeSync->timezone() : String(TIME_TZ_DEFAULT));
    json += "\",";
    json += "\"time_valid\":";
    json += context.timeSync != nullptr ? (context.timeSync->isTimeValid() ? "true" : "false") : "false";
    json += ",";
    json += "\"schedule_count\":";
    json += context.schedule != nullptr ? context.schedule->count() : 0;
    json += ",";
    json += "\"schedule_capacity\":";
    json += context.schedule != nullptr ? context.schedule->capacity() : SCHEDULE_MAX_EVENTS;
    json += ",";
    json += "\"ota_configured\":";
    json += context.ota != nullptr ? (context.ota->isConfigured() ? "true" : "false") : "false";
    json += ",";
    json += "\"ota_auto_schedule_enabled\":";
    json += context.getOtaAutoScheduleEnabled != nullptr ? (context.getOtaAutoScheduleEnabled() ? "true" : "false") : "true";
    json += ",";
    json += "\"relay_auto_off_minutes\":";
    json += context.getRelayAutoOffMinutes != nullptr ? context.getRelayAutoOffMinutes() : 0;
    json += ",";
    json += "\"relay_auto_off_armed\":";
    json += context.getRelayAutoOffArmed != nullptr ? (context.getRelayAutoOffArmed() ? "true" : "false") : "false";
    json += ",";
    json += "\"relay_auto_off_remaining_s\":";
    json += context.getRelayAutoOffRemainingSeconds != nullptr ? context.getRelayAutoOffRemainingSeconds() : 0;
    json += ",";
    json += "\"ota_release_info_url\":\"";
    json += jsonEscape(OTA_RELEASE_INFO_URL);
    json += "\",";
    json += "\"ota_asset_name\":\"";
    json += jsonEscape(OTA_FIRMWARE_ASSET_NAME);
    json += "\",";
    json += "\"wifi_ssid\":\"";
    json += jsonEscape(context.wifi != nullptr ? context.wifi->ssid() : String(WIFI_SSID));
    json += "\",";
    json += "\"wifi_password_set\":";
    json += context.wifi != nullptr ? (context.wifi->ssid().length() > 0 ? "true" : "false") : "false";
    json += ",";
    json += "\"relay_pin\":";
    json += RELAY_PIN;
    json += ",";
    json += "\"relay_led_pin\":";
    json += RELAY_LED_PIN;
    json += ",";
    json += "\"wifi_led_pin\":";
    json += WIFI_LED_PIN;
    json += ",";
    json += "\"relay_button_pin\":";
    json += RELAY_BUTTON_PIN;
    json += ",";
    json += "\"reset_button_pin\":";
    json += RESET_BUTTON_PIN;
    json += ",";
    json += "\"temp_probe_adc_pin\":";
    json += TEMP_PROBE_ADC_PIN;
    json += ",";
    json += "\"temp_probe_present_min_raw\":";
    json += TEMP_PROBE_PRESENT_MIN_RAW;
    json += ",";
    json += "\"temp_probe_present_max_raw\":";
    json += TEMP_PROBE_PRESENT_MAX_RAW;
    json += ",";
    json += "\"temperature_probe_present\":";
    json += context.getTemperatureProbePresent != nullptr ? (context.getTemperatureProbePresent() ? "true" : "false") : "false";
    json += ",";
    json += "\"temperature_probe_raw\":";
    if (context.getTemperatureProbeRaw != nullptr)
    {
      json += context.getTemperatureProbeRaw();
    }
    else
    {
      json += -1;
    }
    json += ",";
    json += "\"current_temperature_raw\":";
    if (context.getCurrentTemperatureRaw != nullptr)
    {
      json += context.getCurrentTemperatureRaw();
    }
    else
    {
      json += -1;
    }
    json += ",";
    json += "\"current_temperature_c\":";
    if (context.getCurrentTemperatureC != nullptr)
    {
      const float currentC = context.getCurrentTemperatureC();
      json += isnan(currentC) ? "null" : String(currentC, 2);
    }
    else
    {
      json += "null";
    }
    json += ",";
    json += "\"temperature_calibration_ready\":";
    json += context.getTemperatureCalibrationReady != nullptr ? (context.getTemperatureCalibrationReady() ? "true" : "false") : "false";
    json += ",";
    json += "\"temperature_trim_offset_c\":";
    if (context.getTemperatureTrimOffsetC != nullptr)
    {
      json += String(context.getTemperatureTrimOffsetC(), 2);
    }
    else
    {
      json += "0.00";
    }
    json += ",";
    json += "\"nvs_health\":\"";
    json += jsonEscape(context.getNvsHealth != nullptr ? context.getNvsHealth() : String("nvs health unavailable"));
    json += "\"";
    json += "}";
    return json;
  }

  String buildTemperatureJson(const WebControlContext &context)
  {
    const bool present = context.getTemperatureProbePresent != nullptr ? context.getTemperatureProbePresent() : false;
    const int raw = context.getTemperatureProbeRaw != nullptr ? context.getTemperatureProbeRaw() : -1;
    const int currentRaw = context.getCurrentTemperatureRaw != nullptr ? context.getCurrentTemperatureRaw() : -1;
    const float currentC = context.getCurrentTemperatureC != nullptr ? context.getCurrentTemperatureC() : NAN;
    const bool ready = context.getTemperatureCalibrationReady != nullptr ? context.getTemperatureCalibrationReady() : false;
    const bool lowValid = context.getLowCalibrationValid != nullptr ? context.getLowCalibrationValid() : false;
    const bool highValid = context.getHighCalibrationValid != nullptr ? context.getHighCalibrationValid() : false;
    const int lowRaw = context.getLowCalibrationRaw != nullptr ? context.getLowCalibrationRaw() : -1;
    const int highRaw = context.getHighCalibrationRaw != nullptr ? context.getHighCalibrationRaw() : -1;
    const float lowTempC = context.getLowCalibrationTempC != nullptr ? context.getLowCalibrationTempC() : NAN;
    const float highTempC = context.getHighCalibrationTempC != nullptr ? context.getHighCalibrationTempC() : NAN;
    const float trimOffsetC = context.getTemperatureTrimOffsetC != nullptr ? context.getTemperatureTrimOffsetC() : 0.0f;

    String json = "{";
    json += "\"ok\":true,";
    json += "\"probe_present\":";
    json += present ? "true" : "false";
    json += ",\"probe_raw\":";
    json += raw;
    json += ",\"current_temperature_raw\":";
    json += currentRaw;
    json += ",\"current_temperature_c\":";
    json += isnan(currentC) ? "null" : String(currentC, 2);
    json += ",\"calibration_ready\":";
    json += ready ? "true" : "false";
    json += ",\"trim_offset_c\":";
    json += String(trimOffsetC, 2);
    json += ",\"low_point\":{";
    json += "\"valid\":";
    json += lowValid ? "true" : "false";
    json += ",\"raw\":";
    json += lowRaw;
    json += ",\"temp_c\":";
    json += isnan(lowTempC) ? "null" : String(lowTempC, 2);
    json += "},\"high_point\":{";
    json += "\"valid\":";
    json += highValid ? "true" : "false";
    json += ",\"raw\":";
    json += highRaw;
    json += ",\"temp_c\":";
    json += isnan(highTempC) ? "null" : String(highTempC, 2);
    json += "}}";
    return json;
  }

  String buildLedTestJson(const WebControlContext &context, const char *command, const char *message)
  {
    String json = "{\"ok\":true,\"command\":\"";
    json += command;
    json += "\",\"message\":\"";
    json += message;
    json += "\",\"relay_led_test_active\":";
    json += context.getRelayLedTestActive != nullptr ? (context.getRelayLedTestActive() ? "true" : "false") : "false";
    json += ",\"wifi_led_test_active\":";
    json += context.getWifiLedTestActive != nullptr ? (context.getWifiLedTestActive() ? "true" : "false") : "false";
    json += "}";
    return json;
  }

  bool parseBoolArg(const String &value)
  {
    String normalized = value;
    normalized.trim();
    normalized.toLowerCase();
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
  }

  bool parseHourMinute(const String &value, uint8_t &hour, uint8_t &minute)
  {
    const int sep = value.indexOf(':');
    if (sep <= 0 || sep >= value.length() - 1)
    {
      return false;
    }

    const int parsedHour = value.substring(0, sep).toInt();
    const int parsedMinute = value.substring(sep + 1).toInt();
    if (parsedHour < 0 || parsedHour > 23 || parsedMinute < 0 || parsedMinute > 59)
    {
      return false;
    }

    hour = static_cast<uint8_t>(parsedHour);
    minute = static_cast<uint8_t>(parsedMinute);
    return true;
  }

  bool parseDateYmd(const String &value, uint16_t &year, uint8_t &month, uint8_t &day)
  {
    const int first = value.indexOf('-');
    if (first <= 0)
    {
      return false;
    }

    const int second = value.indexOf('-', first + 1);
    if (second <= first || second >= value.length() - 1)
    {
      return false;
    }

    const int parsedYear = value.substring(0, first).toInt();
    const int parsedMonth = value.substring(first + 1, second).toInt();
    const int parsedDay = value.substring(second + 1).toInt();

    if (parsedYear < 2020 || parsedYear > 2099 || parsedMonth < 1 || parsedMonth > 12 || parsedDay < 1 || parsedDay > 31)
    {
      return false;
    }

    year = static_cast<uint16_t>(parsedYear);
    month = static_cast<uint8_t>(parsedMonth);
    day = static_cast<uint8_t>(parsedDay);
    return true;
  }

  bool parseCalibrationTemperatureC(const String &valueText, const String &unitText, float &tempC, String &error)
  {
    const float parsed = valueText.toFloat();
    if (isnan(parsed))
    {
      error = "Invalid temperature value";
      return false;
    }

    String unit = unitText;
    unit.trim();
    unit.toLowerCase();
    if (unit.length() == 0)
    {
      unit = "c";
    }

    if (unit == "c")
    {
      tempC = parsed;
      return true;
    }

    if (unit == "f")
    {
      tempC = (parsed - 32.0f) * (5.0f / 9.0f);
      return true;
    }

    error = "temp_unit must be C or F";
    return false;
  }

  const char WEB_UI[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover" />
  <title>Relay Control</title>
  <style>
    :root {
      color-scheme: dark;
      --bg-0: #020617;
      --bg-1: #0f172a;
      --bg-2: #111827;
      --card: rgba(15, 23, 42, 0.88);
      --card-border: rgba(148, 163, 184, 0.18);
      --text: #e2e8f0;
      --muted: #94a3b8;
      --line: rgba(148, 163, 184, 0.12);
      --accent: #22d3ee;
      --accent-2: #38bdf8;
      --shadow: 0 18px 40px rgba(2, 6, 23, 0.55);
    }
    html, body {
      width: 100%;
      min-width: 100%;
    }
    body {
      margin: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at 15% 15%, rgba(34, 211, 238, 0.10), transparent 30%),
        radial-gradient(circle at 85% 15%, rgba(56, 189, 248, 0.10), transparent 28%),
        linear-gradient(180deg, var(--bg-1) 0%, var(--bg-0) 100%);
      color: var(--text);
      min-height: 100vh;
      overflow-x: hidden;
    }
    .wrap {
      width: 100%;
      max-width: none;
      margin: 0;
      padding: calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-top))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-right))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-bottom))
               calc(clamp(12px, 2vw, 24px) + env(safe-area-inset-left));
      box-sizing: border-box;
    }
    .card {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 18px;
      margin-bottom: 16px;
      box-shadow: var(--shadow);
      width: 100%;
      box-sizing: border-box;
      backdrop-filter: blur(14px);
    }
    h1 {
      margin: 0 0 8px;
      font-size: clamp(1.5rem, 2vw, 2rem);
      letter-spacing: -0.03em;
    }
    .muted { color: var(--muted); margin: 0 0 10px; }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
      gap: 14px;
      margin-top: 6px;
    }
    .statusStrip {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
      margin: 8px 0 14px;
    }
    .statusBadge {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 10px 14px;
      border-radius: 999px;
      background: rgba(2, 6, 23, 0.68);
      border: 1px solid rgba(148, 163, 184, 0.18);
      color: var(--text);
      font-size: 0.92rem;
      font-weight: 700;
    }
    .statusDot {
      width: 10px;
      height: 10px;
      border-radius: 999px;
      background: #64748b;
      box-shadow: 0 0 0 4px rgba(100, 116, 139, 0.14);
    }
    .statusBadge[data-state="on"] .statusDot {
      background: #34d399;
      box-shadow: 0 0 0 4px rgba(52, 211, 153, 0.16);
    }
    .statusBadge[data-state="off"] .statusDot {
      background: #fb7185;
      box-shadow: 0 0 0 4px rgba(251, 113, 133, 0.16);
    }
    .statusNote {
      color: var(--muted);
      font-size: 0.9rem;
    }
    button {
      border: none;
      border-radius: 16px;
      padding: 16px 14px;
      font-weight: 700;
      color: #e2e8f0;
      cursor: pointer;
      transition: transform .08s ease, opacity .2s ease, box-shadow .2s ease;
      min-height: 54px;
      touch-action: manipulation;
    }
    button:active { transform: scale(0.985); }
    button:disabled { opacity: 0.55; cursor: not-allowed; }
    .on { background: linear-gradient(135deg, #115e59, #0f766e); box-shadow: 0 10px 24px rgba(15, 118, 110, 0.24); }
    .off { background: linear-gradient(135deg, #7f1d1d, #b91c1c); box-shadow: 0 10px 24px rgba(185, 28, 28, 0.24); }
    .toggle { background: linear-gradient(135deg, #7c2d12, #c2410c); box-shadow: 0 10px 24px rgba(194, 65, 12, 0.24); }
    .refresh { background: linear-gradient(135deg, #1e40af, #2563eb); box-shadow: 0 10px 24px rgba(37, 99, 235, 0.24); }
    .save { background: linear-gradient(135deg, #6d28d9, #7c3aed); color: #f8fafc; box-shadow: 0 10px 24px rgba(124, 58, 237, 0.24); }
    .is-active {
      position: relative;
      box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.12), 0 0 0 6px rgba(34, 211, 238, 0.10), 0 14px 30px rgba(2, 6, 23, 0.35);
      transform: translateY(-1px);
    }
    .is-active::after {
      content: "";
      position: absolute;
      inset: -6px;
      border-radius: 18px;
      border: 1px solid rgba(34, 211, 238, 0.22);
      animation: activePulse 1.6s ease-in-out infinite;
      pointer-events: none;
    }
    @keyframes activePulse {
      0% {
        opacity: 0.18;
        transform: scale(0.98);
      }
      60% {
        opacity: 0.55;
        transform: scale(1.02);
      }
      100% {
        opacity: 0.18;
        transform: scale(1.04);
      }
    }
    .is-inactive {
      opacity: 0.72;
      box-shadow: 0 8px 18px rgba(2, 6, 23, 0.2);
    }
    .kv { margin: 6px 0; font-size: 0.95rem; line-height: 1.45; }
    .sectionTitle { margin: 0 0 10px; font-size: 1rem; letter-spacing: 0.03em; text-transform: uppercase; color: var(--accent); }
    .row { display: flex; gap: 8px; flex-wrap: wrap; }
    .settingsGroup {
      margin-top: 12px;
      border-radius: 20px;
      overflow: hidden;
      background: rgba(2, 6, 23, 0.22);
      border: 1px solid rgba(148, 163, 184, 0.14);
    }
    .settingsRow {
      display: grid;
      grid-template-columns: minmax(138px, 220px) minmax(0, 1fr);
      gap: 12px;
      align-items: center;
      padding: 14px 16px;
      border-bottom: 1px solid rgba(148, 163, 184, 0.12);
      background: rgba(15, 23, 42, 0.35);
    }
    .settingsRow:last-child {
      border-bottom: none;
    }
    .settingsLabel {
      display: flex;
      flex-direction: column;
      gap: 2px;
      min-width: 0;
    }
    .settingsLabel span {
      font-size: 0.95rem;
      font-weight: 700;
      color: var(--text);
    }
    .settingsLabel small {
      font-size: 0.8rem;
      color: var(--muted);
      line-height: 1.35;
    }
    .settingsValue {
      width: 100%;
    }
    .settingsValue input[type=text] {
      width: 100%;
      min-width: 0;
      box-sizing: border-box;
    }
    .settingsRow--action {
      grid-template-columns: 1fr;
    }
    .settingsRow--action button {
      width: 100%;
      min-height: 52px;
    }
    .stackSection {
      margin-top: 14px;
    }
    .stateCard {
      margin-top: 12px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 14px 16px;
      border-radius: 18px;
      background: linear-gradient(135deg, rgba(15, 23, 42, 0.78), rgba(2, 6, 23, 0.68));
      border: 1px solid rgba(148, 163, 184, 0.18);
    }
    .stateCard strong {
      display: block;
      font-size: 0.92rem;
      color: var(--muted);
      letter-spacing: 0.02em;
      font-weight: 700;
      margin-bottom: 4px;
    }
    .stateCard span {
      font-size: 1.15rem;
      font-weight: 800;
      letter-spacing: -0.02em;
    }
    .list { margin: 10px 0 0; padding: 0; list-style: none; }
    .list li {
      border-top: 1px solid var(--line);
      padding: 10px 0;
    }
    .list small { color: var(--muted); display: block; margin-top: 4px; }
    input[type=text] {
      flex: 1;
      min-width: 0;
      border-radius: 14px;
      border: 1px solid rgba(148, 163, 184, 0.28);
      padding: 14px 16px;
      font-size: 1rem;
      background: rgba(2, 6, 23, 0.72);
      color: var(--text);
      box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.02);
    }
    input[type=text]::placeholder {
      color: #64748b;
    }
    pre {
      margin: 0;
      white-space: pre-wrap;
      background: rgba(2, 6, 23, 0.92);
      border-radius: 12px;
      border: 1px solid rgba(148, 163, 184, 0.18);
      padding: 12px;
      min-height: 92px;
      overflow-wrap: anywhere;
      color: #c7d2fe;
    }
    @media (max-width: 520px) {
      .grid { grid-template-columns: 1fr; }
      .settingsRow { grid-template-columns: 1fr; gap: 8px; padding: 14px; }
      .card { padding: 16px; border-radius: 14px; }
      .row { gap: 10px; }
      button { min-height: 52px; }
      .stateCard {
        align-items: flex-start;
        flex-direction: column;
      }
    }
    @media (pointer: coarse) {
      .grid { gap: 12px; }
      .settingsGroup { border-radius: 18px; }
      .settingsRow { padding: 14px 15px; }
      input[type=text] {
        padding: 14px;
        font-size: 1.05rem;
      }
      .muted {
        line-height: 1.5;
      }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>ESP32 Relay Console</h1>
      <p class="muted">Dark mode control panel with live relay, network, and configuration status.</p>
      <div class="stateCard">
        <div>
          <strong>Current Relay State</strong>
          <span id="relayStateLabel">unknown</span>
        </div>
        <div class="statusBadge" id="relayStateBadge" data-state="unknown">
          <span class="statusDot"></span>
          <span id="relayStateBadgeText">Waiting for status</span>
        </div>
      </div>
      <div class="grid stackSection">
        <button id="btnOn" class="on">ON</button>
        <button id="btnOff" class="off">OFF</button>
        <button id="btnToggle" class="toggle">TOGGLE</button>
        <button id="btnRefresh" class="refresh">REFRESH STATUS</button>
      </div>
      <div class="stackSection">
        <div class="sectionTitle">LED Test Controls</div>
        <p class="muted">Runs the selected LED test for 5 seconds, then returns control to normal firmware behavior.</p>
        <div class="grid">
          <button id="btnRelayLedTest" class="refresh">TEST RELAY LED</button>
          <button id="btnWifiLedTest" class="refresh">TEST WI-FI LED</button>
          <button id="btnAllLedTests" class="toggle">TEST BOTH LEDS</button>
        </div>
        <div class="kv" style="margin-top:10px"><strong>Relay LED test active:</strong> <span id="relayLedTestActive">no</span></div>
        <div class="kv"><strong>Wi-Fi LED test active:</strong> <span id="wifiLedTestActive">no</span></div>
      </div>
      <div class="settingsGroup stackSection">
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Hostname</span>
            <small>Used for mDNS and device identity.</small>
          </div>
          <div class="settingsValue">
            <input id="hostnameInput" type="text" placeholder="homerelay" maxlength="32" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Host</span>
            <small>Broker address or hostname.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttHostInput" type="text" placeholder="192.168.0.50" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Port</span>
            <small>Defaults to 1883 if empty.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttPortInput" type="text" placeholder="1883" inputmode="numeric" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Username</span>
            <small>Leave empty for anonymous broker access.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttUsernameInput" type="text" placeholder="mqtt-user" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>MQTT Password</span>
            <small>Leave empty to keep current password.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttPasswordInput" type="password" placeholder="stored password" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Disable MQTT</span>
            <small>Stops all MQTT connect, subscribe, and publish activity.</small>
          </div>
          <div class="settingsValue">
            <input id="mqttDisabledInput" type="checkbox" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Disable Weekly OTA Auto-Update</span>
            <small>Default is enabled (Monday 10:30). Disable to stop automatic OTA schedule.</small>
          </div>
          <div class="settingsValue">
            <input id="otaAutoScheduleDisabledInput" type="checkbox" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Relay Auto-Off (minutes)</span>
            <small>Set to 0 to disable. Any ON trigger resets this countdown.</small>
          </div>
          <div class="settingsValue">
            <input id="relayAutoOffMinutesInput" type="text" placeholder="60" inputmode="numeric" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Wi-Fi SSID</span>
            <small>Tap a network below to fill this in.</small>
          </div>
          <div class="settingsValue">
            <input id="wifiSsidInput" type="text" placeholder="Wi-Fi network name" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Wi-Fi Password</span>
            <small>Leave empty to keep the saved password.</small>
          </div>
          <div class="settingsValue">
            <input id="wifiPassInput" type="text" placeholder="stored password or new password" />
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <button id="btnSaveConfig" class="save">SAVE CONFIG</button>
        </div>
      </div>
      <div class="muted" style="margin-top:10px">mDNS hostnames are saved without the .local suffix, but the UI shows the full name as <span id="mdnsHostInline">n/a</span>.</div>
    </div>

    <div class="card">
      <div class="sectionTitle">Configuration</div>
      <div class="kv"><strong>Device:</strong> <span id="deviceName">n/a</span></div>
      <div class="kv"><strong>Type:</strong> <span id="deviceType">n/a</span></div>
      <div class="kv"><strong>Firmware:</strong> <span id="firmware">n/a</span></div>
      <div class="kv"><strong>mDNS host:</strong> <span id="mdnsHost">n/a</span></div>
      <div class="kv"><strong>MQTT Host:</strong> <span id="mqttHost">n/a</span></div>
      <div class="kv"><strong>MQTT Port:</strong> <span id="mqttPort">n/a</span></div>
      <div class="kv"><strong>MQTT Enabled:</strong> <span id="mqttEnabled">n/a</span></div>
      <div class="kv"><strong>MQTT Username:</strong> <span id="mqttUsername">n/a</span></div>
      <div class="kv"><strong>MQTT Password Set:</strong> <span id="mqttPasswordSet">n/a</span></div>
      <div class="kv"><strong>Relay Auto-Off Minutes:</strong> <span id="relayAutoOffMinutes">n/a</span></div>
      <div class="kv"><strong>Relay Auto-Off Armed:</strong> <span id="relayAutoOffArmed">n/a</span></div>
      <div class="kv"><strong>Relay Auto-Off Remaining:</strong> <span id="relayAutoOffRemaining">n/a</span></div>
      <div class="kv"><strong>OTA Configured:</strong> <span id="otaConfigured">n/a</span></div>
      <div class="kv"><strong>Relay pin:</strong> <span id="relayPin">n/a</span></div>
      <div class="kv"><strong>Relay LED pin:</strong> <span id="relayLedPin">n/a</span></div>
      <div class="kv"><strong>Wi-Fi LED pin:</strong> <span id="wifiLedPin">n/a</span></div>
      <div class="kv"><strong>Relay button pin:</strong> <span id="relayButtonPin">n/a</span></div>
      <div class="kv"><strong>Reset button pin:</strong> <span id="resetButtonPin">n/a</span></div>
      <div class="kv"><strong>Temp probe ADC pin:</strong> <span id="tempProbeAdcPin">n/a</span></div>
      <div class="kv"><strong>Probe detect range:</strong> <span id="tempProbeRange">n/a</span></div>
      <div class="kv"><strong>Probe present:</strong> <span id="tempProbePresent">n/a</span></div>
      <div class="kv"><strong>Probe raw value:</strong> <span id="tempProbeRaw">n/a</span></div>
      <div class="kv"><strong>Current temperature raw:</strong> <span id="currentTemperatureRaw">n/a</span></div>
      <div class="kv"><strong>Hostname default:</strong> <span id="hostnameDefault">n/a</span></div>
      <div class="kv"><strong>NVS:</strong> <span id="nvsHealth">n/a</span></div>
    </div>

    <div class="card">
      <div class="sectionTitle">Firmware Update (OTA)</div>
      <p class="muted">Manual update only. Check for a release first, then install when ready.</p>
      <div class="row">
        <button id="btnOtaCheck" class="refresh">CHECK FOR UPDATE</button>
        <button id="btnOtaUpdate" class="save">INSTALL UPDATE</button>
      </div>
      <div class="kv" style="margin-top:10px"><strong>Current Version:</strong> <span id="otaCurrentVersion">n/a</span></div>
      <div class="kv"><strong>Latest Version:</strong> <span id="otaLatestVersion">n/a</span></div>
      <div class="kv"><strong>Update Available:</strong> <span id="otaUpdateAvailable">n/a</span></div>
      <div class="kv"><strong>Status:</strong> <span id="otaStatus">No OTA check yet</span></div>
    </div>

    <div class="card">
      <div class="sectionTitle">Temperature Calibration</div>
      <p class="muted">Capture low/high calibration points from live ADC probe readings and compute calibrated temperature.</p>
      <div class="kv"><strong>Probe detected:</strong> <span id="tempCalProbePresent">n/a</span></div>
      <div class="kv"><strong>Live raw ADC:</strong> <span id="tempCalLiveRaw">n/a</span></div>
      <div class="kv"><strong>Computed temperature:</strong> <span id="tempCalComputedC">n/a</span></div>
      <div class="kv"><strong>Calibration ready:</strong> <span id="tempCalReady">n/a</span></div>
      <div class="kv"><strong>Trim offset:</strong> <span id="tempCalTrimOffset">0.00 C</span></div>
      <div class="settingsGroup stackSection">
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Calibration Input Unit</span>
            <small>Choose the unit used by your reference thermometer.</small>
          </div>
          <div class="settingsValue">
            <select id="tempCalUnitInput">
              <option value="c" selected>Celsius (C)</option>
              <option value="f">Fahrenheit (F)</option>
            </select>
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Low Reference Temperature (C)</span>
            <small>Known real temperature when capturing low point (converted to C internally).</small>
          </div>
          <div class="settingsValue">
            <input id="tempLowInput" type="text" placeholder="30.0" inputmode="decimal" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>High Reference Temperature (C)</span>
            <small>Known real temperature when capturing high point (converted to C internally).</small>
          </div>
          <div class="settingsValue">
            <input id="tempHighInput" type="text" placeholder="40.0" inputmode="decimal" />
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <div class="row">
            <button id="btnTempCaptureLow" class="save">CAPTURE LOW</button>
            <button id="btnTempCaptureHigh" class="save">CAPTURE HIGH</button>
            <button id="btnTempResetCal" class="off">RESET CALIBRATION</button>
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <div class="row">
            <button id="btnTempTrimMinus1" class="off">TRIM -1.0 C</button>
            <button id="btnTempTrimMinus02" class="off">TRIM -0.2 C</button>
            <button id="btnTempTrimPlus02" class="save">TRIM +0.2 C</button>
            <button id="btnTempTrimPlus1" class="save">TRIM +1.0 C</button>
            <button id="btnTempTrimReset" class="refresh">TRIM RESET</button>
          </div>
        </div>
      </div>
      <div class="kv" style="margin-top:10px"><strong>Low point:</strong> <span id="tempCalLowPoint">n/a</span></div>
      <div class="kv"><strong>High point:</strong> <span id="tempCalHighPoint">n/a</span></div>
    </div>

    <div class="card">
      <div class="sectionTitle">Wi-Fi</div>
      <div class="row">
        <button id="btnScanWifi" class="refresh">SCAN WI-FI</button>
      </div>
      <ul id="wifiList" class="list"></ul>
    </div>

    <div class="card">
      <div class="sectionTitle">Time & Schedules</div>
      <div class="settingsGroup">
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Enable Time Sync</span>
            <small>Uses public or local NTP servers for clock sync.</small>
          </div>
          <div class="settingsValue">
            <input id="timeEnabledInput" type="checkbox" checked />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Time Server</span>
            <small>Local NTP server hostname or IP.</small>
          </div>
          <div class="settingsValue">
            <input id="timeServerInput" type="text" placeholder="pool.ntp.org" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Timezone Rule</span>
            <small>POSIX TZ rule used for local schedule evaluation.</small>
          </div>
          <div class="settingsValue">
            <input id="timeTimezoneInput" type="text" placeholder="GMT0BST,M3.5.0/1,M10.5.0/2" />
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <div class="row">
            <button id="btnSaveTimeConfig" class="save">SAVE TIME SETTINGS</button>
            <button id="btnTimeSyncNow" class="refresh">SYNC NOW</button>
          </div>
        </div>
      </div>

      <div class="kv" style="margin-top:10px"><strong>Time Valid:</strong> <span id="timeValid">n/a</span></div>
      <div class="kv"><strong>System Time:</strong> <span id="systemTime">n/a</span></div>
      <div class="kv"><strong>System Epoch:</strong> <span id="systemEpoch">n/a</span></div>
      <div class="kv"><strong>Last Sync Status:</strong> <span id="timeSyncStatus">n/a</span></div>
      <div class="kv"><strong>Last Sync Epoch:</strong> <span id="timeSyncEpoch">n/a</span></div>
      <div class="kv"><strong>Schedule Slots:</strong> <span id="scheduleSlots">0/10</span></div>

      <div class="settingsGroup stackSection">
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Command</span>
            <small>Existing command router keyword (on/off/toggle).</small>
          </div>
          <div class="settingsValue">
            <input id="eventCommandInput" type="text" placeholder="on" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Time</span>
            <small>24h time HH:MM.</small>
          </div>
          <div class="settingsValue">
            <input id="eventTimeInput" type="text" placeholder="06:30" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Recurring</span>
            <small>Checked = recurring, unchecked = one-time date event.</small>
          </div>
          <div class="settingsValue">
            <input id="eventRecurringInput" type="checkbox" checked />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Day Mask</span>
            <small>Recurring bitmask 0-127 (Mon=1, Tue=2 ... Sun=64).</small>
          </div>
          <div class="settingsValue">
            <input id="eventDowMaskInput" type="text" placeholder="127" inputmode="numeric" pattern="^(0|[1-9][0-9]?)$|^1[01][0-9]$|^12[0-7]$" />
          </div>
        </div>
        <div class="settingsRow">
          <div class="settingsLabel">
            <span>Date</span>
            <small>One-time only date YYYY-MM-DD.</small>
          </div>
          <div class="settingsValue">
            <input id="eventDateInput" type="text" placeholder="2026-12-31" />
          </div>
        </div>
        <div class="settingsRow settingsRow--action">
          <input id="eventIdInput" type="hidden" />
          <div class="row">
            <button id="btnScheduleSave" class="save">ADD / UPDATE EVENT</button>
            <button id="btnScheduleReset" class="refresh">RESET FORM</button>
          </div>
        </div>
      </div>

      <ul id="scheduleList" class="list"></ul>
    </div>

    <div class="card">
      <div class="kv"><strong>Relay:</strong> <span id="relay">unknown</span></div>
      <div class="kv"><strong>Firmware Version:</strong> <span id="firmwareVersion">n/a</span></div>
      <div class="kv"><strong>Last command:</strong> <span id="last">none</span></div>
      <div class="kv"><strong>Uptime:</strong> <span id="uptime">n/a</span></div>
      <div class="kv"><strong>Hostname:</strong> <span id="hostname">n/a</span></div>
      <div class="kv"><strong>MQTT Client ID:</strong> <span id="mqttClientId">n/a</span></div>
      <div class="kv"><strong>Auto-Off Remaining (s):</strong> <span id="statusRelayAutoOffRemaining">n/a</span></div>
      <div class="kv"><strong>Probe present:</strong> <span id="statusTempProbePresent">n/a</span></div>
      <div class="kv"><strong>Probe raw:</strong> <span id="statusTempProbeRaw">n/a</span></div>
      <div class="kv"><strong>Current temperature raw:</strong> <span id="statusCurrentTemperatureRaw">n/a</span></div>
      <div class="kv"><strong>Error:</strong> <span id="error">none</span></div>
    </div>

    <div class="card">
      <strong>Raw JSON</strong>
      <pre id="raw">(no response yet)</pre>
    </div>
  </div>

  <script>
    const ids = {
      relay: document.getElementById('relay'),
      firmwareVersion: document.getElementById('firmwareVersion'),
      last: document.getElementById('last'),
      uptime: document.getElementById('uptime'),
      hostname: document.getElementById('hostname'),
      mqttClientId: document.getElementById('mqttClientId'),
      error: document.getElementById('error'),
      relayStateLabel: document.getElementById('relayStateLabel'),
      relayStateBadge: document.getElementById('relayStateBadge'),
      relayStateBadgeText: document.getElementById('relayStateBadgeText'),
      mqttHostInput: document.getElementById('mqttHostInput'),
      mqttPortInput: document.getElementById('mqttPortInput'),
      mqttUsernameInput: document.getElementById('mqttUsernameInput'),
      mqttPasswordInput: document.getElementById('mqttPasswordInput'),
      mqttDisabledInput: document.getElementById('mqttDisabledInput'),
      otaAutoScheduleDisabledInput: document.getElementById('otaAutoScheduleDisabledInput'),
      relayAutoOffMinutesInput: document.getElementById('relayAutoOffMinutesInput'),
      relayLedTestActive: document.getElementById('relayLedTestActive'),
      wifiLedTestActive: document.getElementById('wifiLedTestActive'),
      timeEnabledInput: document.getElementById('timeEnabledInput'),
      timeServerInput: document.getElementById('timeServerInput'),
      timeTimezoneInput: document.getElementById('timeTimezoneInput'),
      timeValid: document.getElementById('timeValid'),
      systemTime: document.getElementById('systemTime'),
      systemEpoch: document.getElementById('systemEpoch'),
      timeSyncStatus: document.getElementById('timeSyncStatus'),
      timeSyncEpoch: document.getElementById('timeSyncEpoch'),
      scheduleSlots: document.getElementById('scheduleSlots'),
      eventIdInput: document.getElementById('eventIdInput'),
      eventCommandInput: document.getElementById('eventCommandInput'),
      eventTimeInput: document.getElementById('eventTimeInput'),
      eventRecurringInput: document.getElementById('eventRecurringInput'),
      eventDowMaskInput: document.getElementById('eventDowMaskInput'),
      eventDateInput: document.getElementById('eventDateInput'),
      scheduleList: document.getElementById('scheduleList'),
      wifiSsidInput: document.getElementById('wifiSsidInput'),
      wifiPassInput: document.getElementById('wifiPassInput'),
      deviceName: document.getElementById('deviceName'),
      deviceType: document.getElementById('deviceType'),
      firmware: document.getElementById('firmware'),
      mdnsHost: document.getElementById('mdnsHost'),
      mdnsHostInline: document.getElementById('mdnsHostInline'),
      mqttHost: document.getElementById('mqttHost'),
      mqttPort: document.getElementById('mqttPort'),
      mqttEnabled: document.getElementById('mqttEnabled'),
      mqttUsername: document.getElementById('mqttUsername'),
      mqttPasswordSet: document.getElementById('mqttPasswordSet'),
      relayAutoOffMinutes: document.getElementById('relayAutoOffMinutes'),
      relayAutoOffArmed: document.getElementById('relayAutoOffArmed'),
      relayAutoOffRemaining: document.getElementById('relayAutoOffRemaining'),
      statusRelayAutoOffRemaining: document.getElementById('statusRelayAutoOffRemaining'),
      otaConfigured: document.getElementById('otaConfigured'),
      otaCurrentVersion: document.getElementById('otaCurrentVersion'),
      otaLatestVersion: document.getElementById('otaLatestVersion'),
      otaUpdateAvailable: document.getElementById('otaUpdateAvailable'),
      otaStatus: document.getElementById('otaStatus'),
      tempCalProbePresent: document.getElementById('tempCalProbePresent'),
      tempCalLiveRaw: document.getElementById('tempCalLiveRaw'),
      tempCalComputedC: document.getElementById('tempCalComputedC'),
      tempCalReady: document.getElementById('tempCalReady'),
      tempCalTrimOffset: document.getElementById('tempCalTrimOffset'),
      tempCalUnitInput: document.getElementById('tempCalUnitInput'),
      tempLowInput: document.getElementById('tempLowInput'),
      tempHighInput: document.getElementById('tempHighInput'),
      tempCalLowPoint: document.getElementById('tempCalLowPoint'),
      tempCalHighPoint: document.getElementById('tempCalHighPoint'),
      relayPin: document.getElementById('relayPin'),
      relayLedPin: document.getElementById('relayLedPin'),
      wifiLedPin: document.getElementById('wifiLedPin'),
      relayButtonPin: document.getElementById('relayButtonPin'),
      resetButtonPin: document.getElementById('resetButtonPin'),
      tempProbeAdcPin: document.getElementById('tempProbeAdcPin'),
      tempProbeRange: document.getElementById('tempProbeRange'),
      tempProbePresent: document.getElementById('tempProbePresent'),
      tempProbeRaw: document.getElementById('tempProbeRaw'),
      currentTemperatureRaw: document.getElementById('currentTemperatureRaw'),
      statusTempProbePresent: document.getElementById('statusTempProbePresent'),
      statusTempProbeRaw: document.getElementById('statusTempProbeRaw'),
      statusCurrentTemperatureRaw: document.getElementById('statusCurrentTemperatureRaw'),
      hostnameDefault: document.getElementById('hostnameDefault'),
      nvsHealth: document.getElementById('nvsHealth'),
      wifiList: document.getElementById('wifiList'),
      raw: document.getElementById('raw'),
      hostnameInput: document.getElementById('hostnameInput'),
      buttons: [
        document.getElementById('btnOn'),
        document.getElementById('btnOff'),
        document.getElementById('btnToggle'),
        document.getElementById('btnRefresh'),
        document.getElementById('btnRelayLedTest'),
        document.getElementById('btnWifiLedTest'),
        document.getElementById('btnAllLedTests'),
        document.getElementById('btnSaveConfig'),
        document.getElementById('btnScanWifi'),
        document.getElementById('btnSaveTimeConfig'),
        document.getElementById('btnTimeSyncNow'),
        document.getElementById('btnScheduleSave'),
        document.getElementById('btnScheduleReset'),
        document.getElementById('btnOtaCheck'),
        document.getElementById('btnOtaUpdate'),
        document.getElementById('btnTempCaptureLow'),
        document.getElementById('btnTempCaptureHigh'),
        document.getElementById('btnTempResetCal'),
        document.getElementById('btnTempTrimMinus1'),
        document.getElementById('btnTempTrimMinus02'),
        document.getElementById('btnTempTrimPlus02'),
        document.getElementById('btnTempTrimPlus1'),
        document.getElementById('btnTempTrimReset')
      ]
    };

    let currentMqttEnabled = true;
    let currentMqttUsername = '';
    let currentOtaAutoScheduleEnabled = true;
    let currentRelayAutoOffMinutes = 0;
    let timeState = { enabled: true, events: [], count: 0, capacity: 10 };

    function setBusy(busy) {
      ids.buttons.forEach(b => b.disabled = busy);
    }

    function setRelayState(stateText) {
      const normalized = (stateText || 'unknown').toString().toLowerCase();
      const isOn = normalized === 'on';
      const isOff = normalized === 'off';

      ids.relayStateLabel.textContent = normalized;
      ids.relayStateBadge.dataset.state = normalized;
      ids.relayStateBadgeText.textContent = isOn ? 'Relay energized' : (isOff ? 'Relay idle' : 'State unknown');

      ids.buttons.forEach(button => {
        button.classList.remove('is-active', 'is-inactive');
      });

      if (isOn) {
        document.getElementById('btnOn').classList.add('is-active');
        document.getElementById('btnOff').classList.add('is-inactive');
      } else if (isOff) {
        document.getElementById('btnOff').classList.add('is-active');
        document.getElementById('btnOn').classList.add('is-inactive');
      }
    }

    function showLedTestStatus(obj) {
      if (typeof obj.relay_led_test_active !== 'undefined') {
        ids.relayLedTestActive.textContent = obj.relay_led_test_active ? 'yes' : 'no';
      }
      if (typeof obj.wifi_led_test_active !== 'undefined') {
        ids.wifiLedTestActive.textContent = obj.wifi_led_test_active ? 'yes' : 'no';
      }
    }

    function showJson(obj) {
      ids.raw.textContent = JSON.stringify(obj, null, 2);
      if (obj.relay) {
        ids.relay.textContent = obj.relay;
        setRelayState(obj.relay);
      }
      if (typeof obj.uptime_ms !== 'undefined') ids.uptime.textContent = obj.uptime_ms + ' ms';
      if (obj.hostname) {
        ids.hostname.textContent = obj.hostname;
      }
      if (obj.firmware_version) ids.firmwareVersion.textContent = obj.firmware_version;
      if (obj.mqtt_client_id) ids.mqttClientId.textContent = obj.mqtt_client_id;
      if (typeof obj.relay_auto_off_remaining_s !== 'undefined') {
        const remaining = Number(obj.relay_auto_off_remaining_s);
        ids.statusRelayAutoOffRemaining.textContent = Number.isFinite(remaining) ? String(Math.max(0, Math.floor(remaining))) : 'n/a';
        ids.relayAutoOffRemaining.textContent = Number.isFinite(remaining) ? `${Math.max(0, Math.floor(remaining))} s` : 'n/a';
      }
      if (typeof obj.relay_auto_off_armed !== 'undefined') {
        ids.relayAutoOffArmed.textContent = obj.relay_auto_off_armed ? 'yes' : 'no';
      }
      if (typeof obj.temperature_probe_present !== 'undefined') {
        const present = !!obj.temperature_probe_present;
        ids.statusTempProbePresent.textContent = present ? 'yes' : 'no';
        ids.tempProbePresent.textContent = present ? 'yes' : 'no';
      }
      if (typeof obj.temperature_probe_raw !== 'undefined') {
        const raw = Number(obj.temperature_probe_raw);
        const rawText = Number.isFinite(raw) && raw >= 0 ? String(raw) : 'n/a';
        ids.statusTempProbeRaw.textContent = rawText;
        ids.tempProbeRaw.textContent = rawText;
      }
      if (typeof obj.current_temperature_raw !== 'undefined') {
        const currentRaw = Number(obj.current_temperature_raw);
        const currentText = Number.isFinite(currentRaw) && currentRaw >= 0 ? String(currentRaw) : 'n/a';
        ids.statusCurrentTemperatureRaw.textContent = currentText;
        ids.currentTemperatureRaw.textContent = currentText;
        ids.tempCalLiveRaw.textContent = currentText;
      }
      if (typeof obj.current_temperature_c !== 'undefined') {
        const currentTempC = Number(obj.current_temperature_c);
        ids.tempCalComputedC.textContent = Number.isFinite(currentTempC) ? `${currentTempC.toFixed(2)} C` : 'n/a';
      }
      if (typeof obj.temperature_calibration_ready !== 'undefined') {
        ids.tempCalReady.textContent = obj.temperature_calibration_ready ? 'yes' : 'no';
      }
      if (typeof obj.temperature_trim_offset_c !== 'undefined') {
        const trim = Number(obj.temperature_trim_offset_c);
        ids.tempCalTrimOffset.textContent = Number.isFinite(trim) ? `${trim.toFixed(2)} C` : '0.00 C';
      }
      showLedTestStatus(obj);
      if (obj.command) ids.last.textContent = obj.command;
      ids.error.textContent = obj.ok ? 'none' : (obj.error || 'request failed');
    }

    function showConfig(obj) {
      if (obj.hostname) {
        ids.hostnameInput.value = obj.hostname;
      }
      if (obj.device_name) ids.deviceName.textContent = obj.device_name;
      if (obj.device_type) ids.deviceType.textContent = obj.device_type;
      if (obj.firmware) ids.firmware.textContent = obj.firmware.name + ' ' + obj.firmware.version;
      if (obj.mdns_host) ids.mdnsHost.textContent = obj.mdns_host;
      if (obj.mdns_host) ids.mdnsHostInline.textContent = obj.mdns_host;
      if (obj.mqtt_host) ids.mqttHost.textContent = obj.mqtt_host;
      if (typeof obj.mqtt_port !== 'undefined') ids.mqttPort.textContent = obj.mqtt_port;
      if (obj.mqtt_host) ids.mqttHostInput.value = obj.mqtt_host;
      if (typeof obj.mqtt_port !== 'undefined') ids.mqttPortInput.value = obj.mqtt_port;
      if (typeof obj.mqtt_username !== 'undefined') {
        currentMqttUsername = (obj.mqtt_username || '').toString();
        ids.mqttUsernameInput.value = currentMqttUsername;
        ids.mqttUsername.textContent = currentMqttUsername.length > 0 ? currentMqttUsername : 'none';
      }
      if (typeof obj.mqtt_password_set !== 'undefined') {
        ids.mqttPasswordSet.textContent = obj.mqtt_password_set ? 'yes' : 'no';
        ids.mqttPasswordInput.placeholder = obj.mqtt_password_set ? 'stored password' : 'new password';
        ids.mqttPasswordInput.value = '';
      }
      if (typeof obj.mqtt_enabled !== 'undefined') {
        currentMqttEnabled = !!obj.mqtt_enabled;
        ids.mqttDisabledInput.checked = !currentMqttEnabled;
        ids.mqttEnabled.textContent = currentMqttEnabled ? 'yes' : 'no';
      }
      if (typeof obj.relay_auto_off_minutes !== 'undefined') {
        const minutes = Number(obj.relay_auto_off_minutes);
        currentRelayAutoOffMinutes = Number.isFinite(minutes) ? Math.max(0, Math.floor(minutes)) : 0;
        ids.relayAutoOffMinutesInput.value = String(currentRelayAutoOffMinutes);
        ids.relayAutoOffMinutes.textContent = String(currentRelayAutoOffMinutes);
      }
      if (typeof obj.relay_auto_off_armed !== 'undefined') {
        ids.relayAutoOffArmed.textContent = obj.relay_auto_off_armed ? 'yes' : 'no';
      }
      if (typeof obj.relay_auto_off_remaining_s !== 'undefined') {
        const remaining = Number(obj.relay_auto_off_remaining_s);
        ids.relayAutoOffRemaining.textContent = Number.isFinite(remaining) ? `${Math.max(0, Math.floor(remaining))} s` : 'n/a';
      }
      showLedTestStatus(obj);
      if (typeof obj.ota_configured !== 'undefined') {
        ids.otaConfigured.textContent = obj.ota_configured ? 'yes' : 'no';
      }
      if (typeof obj.ota_auto_schedule_enabled !== 'undefined') {
        currentOtaAutoScheduleEnabled = !!obj.ota_auto_schedule_enabled;
        ids.otaAutoScheduleDisabledInput.checked = !currentOtaAutoScheduleEnabled;
      }
      if (obj.firmware && obj.firmware.version) {
        ids.otaCurrentVersion.textContent = obj.firmware.version;
      }
      if (obj.wifi_ssid) ids.wifiSsidInput.value = obj.wifi_ssid;
      ids.wifiPassInput.placeholder = obj.wifi_password_set ? 'stored password' : 'new password';
      if (typeof obj.relay_pin !== 'undefined') ids.relayPin.textContent = obj.relay_pin;
      if (typeof obj.relay_led_pin !== 'undefined') ids.relayLedPin.textContent = obj.relay_led_pin;
      if (typeof obj.wifi_led_pin !== 'undefined') ids.wifiLedPin.textContent = obj.wifi_led_pin;
      if (typeof obj.relay_button_pin !== 'undefined') ids.relayButtonPin.textContent = obj.relay_button_pin;
      if (typeof obj.reset_button_pin !== 'undefined') ids.resetButtonPin.textContent = obj.reset_button_pin;
      if (typeof obj.temp_probe_adc_pin !== 'undefined') ids.tempProbeAdcPin.textContent = obj.temp_probe_adc_pin;
      if (typeof obj.temp_probe_present_min_raw !== 'undefined' && typeof obj.temp_probe_present_max_raw !== 'undefined') {
        ids.tempProbeRange.textContent = `${obj.temp_probe_present_min_raw + 1}-${obj.temp_probe_present_max_raw - 1}`;
      }
      if (typeof obj.temperature_probe_present !== 'undefined') ids.tempProbePresent.textContent = obj.temperature_probe_present ? 'yes' : 'no';
      if (typeof obj.temperature_probe_present !== 'undefined') ids.tempCalProbePresent.textContent = obj.temperature_probe_present ? 'yes' : 'no';
      if (typeof obj.temperature_probe_raw !== 'undefined') {
        const raw = Number(obj.temperature_probe_raw);
        ids.tempProbeRaw.textContent = Number.isFinite(raw) && raw >= 0 ? String(raw) : 'n/a';
      }
      if (typeof obj.current_temperature_raw !== 'undefined') {
        const currentRaw = Number(obj.current_temperature_raw);
        ids.currentTemperatureRaw.textContent = Number.isFinite(currentRaw) && currentRaw >= 0 ? String(currentRaw) : 'n/a';
        ids.tempCalLiveRaw.textContent = Number.isFinite(currentRaw) && currentRaw >= 0 ? String(currentRaw) : 'n/a';
      }
      if (typeof obj.current_temperature_c !== 'undefined') {
        const currentTempC = Number(obj.current_temperature_c);
        ids.tempCalComputedC.textContent = Number.isFinite(currentTempC) ? `${currentTempC.toFixed(2)} C` : 'n/a';
      }
      if (typeof obj.temperature_calibration_ready !== 'undefined') {
        ids.tempCalReady.textContent = obj.temperature_calibration_ready ? 'yes' : 'no';
      }
      if (typeof obj.temperature_trim_offset_c !== 'undefined') {
        const trim = Number(obj.temperature_trim_offset_c);
        ids.tempCalTrimOffset.textContent = Number.isFinite(trim) ? `${trim.toFixed(2)} C` : '0.00 C';
      }
      if (obj.hostname_default) ids.hostnameDefault.textContent = obj.hostname_default;
      if (obj.nvs_health) ids.nvsHealth.textContent = obj.nvs_health;
    }

    function renderWifiList(obj) {
      const networks = Array.isArray(obj.networks) ? obj.networks : [];
      if (networks.length === 0) {
        ids.wifiList.innerHTML = '<li>No networks found.</li>';
        return;
      }

      ids.wifiList.innerHTML = networks.map(net => {
        const ssid = net.ssid && net.ssid.length ? net.ssid : '<hidden>';
        return `<li data-ssid="${ssid}"><strong>${ssid}</strong><small>BSSID ${net.bssid || 'n/a'} | RSSI ${net.rssi} dBm | Channel ${net.channel} | Enc ${net.encryption}</small></li>`;
      }).join('');

      ids.wifiList.querySelectorAll('li[data-ssid]').forEach(item => {
        item.style.cursor = 'pointer';
        item.addEventListener('click', () => {
          const ssid = item.getAttribute('data-ssid');
          if (ssid && ssid !== '<hidden>') {
            ids.wifiSsidInput.value = ssid;
          }
        });
      });
    }

    function resetScheduleForm() {
      ids.eventIdInput.value = '';
      ids.eventCommandInput.value = '';
      ids.eventTimeInput.value = '';
      ids.eventRecurringInput.checked = true;
      ids.eventDowMaskInput.value = '127';
      ids.eventDateInput.value = '';
    }

    function validateTimeHm(value) {
      const match = /^(\d{2}):(\d{2})$/.exec(value);
      if (!match) {
        return { ok: false, error: 'Time must be in HH:MM 24-hour format' };
      }

      const hh = Number(match[1]);
      const mm = Number(match[2]);
      if (!Number.isInteger(hh) || !Number.isInteger(mm) || hh < 0 || hh > 23 || mm < 0 || mm > 59) {
        return { ok: false, error: 'Time must be between 00:00 and 23:59' };
      }

      return { ok: true };
    }

    function validateDowMask(value) {
      if (!/^[0-9]{1,3}$/.test(value)) {
        return { ok: false, error: 'Day mask must be a number from 0 to 127' };
      }

      const mask = Number(value);
      if (!Number.isInteger(mask) || mask < 0 || mask > 127) {
        return { ok: false, error: 'Day mask must be in range 0..127' };
      }

      return { ok: true };
    }

    function validateDateYmd(value) {
      const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(value);
      if (!match) {
        return { ok: false, error: 'Date must be in YYYY-MM-DD format' };
      }

      const year = Number(match[1]);
      const month = Number(match[2]);
      const day = Number(match[3]);

      if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) {
        return { ok: false, error: 'Date is out of supported range (2020-2099)' };
      }

      const date = new Date(Date.UTC(year, month - 1, day));
      const valid = date.getUTCFullYear() === year && (date.getUTCMonth() + 1) === month && date.getUTCDate() === day;
      if (!valid) {
        return { ok: false, error: 'Date is not a valid calendar day' };
      }

      return { ok: true };
    }

    function renderScheduleList(events) {
      if (!Array.isArray(events) || events.length === 0) {
        ids.scheduleList.innerHTML = '<li>No scheduled events yet.</li>';
        return;
      }

      ids.scheduleList.innerHTML = events.map(event => {
        const hh = String(event.hour).padStart(2, '0');
        const mm = String(event.minute).padStart(2, '0');
        const scheduleText = event.recurring
          ? `Recurring mask ${event.dow_mask}`
          : `One-time ${event.year}-${String(event.month).padStart(2, '0')}-${String(event.day).padStart(2, '0')}`;
        return `<li data-id="${event.id}"><strong>#${event.id} ${event.command} @ ${hh}:${mm}</strong><small>${scheduleText} | ${event.enabled ? 'enabled' : 'disabled'}</small><div class="row" style="margin-top:6px"><button class="refresh" data-edit="${event.id}">EDIT</button><button class="off" data-delete="${event.id}">DELETE</button></div></li>`;
      }).join('');

      ids.scheduleList.querySelectorAll('button[data-edit]').forEach(button => {
        button.addEventListener('click', () => {
          const id = Number(button.getAttribute('data-edit'));
          const event = events.find(e => Number(e.id) === id);
          if (!event) return;
          ids.eventIdInput.value = String(event.id);
          ids.eventCommandInput.value = event.command || '';
          ids.eventTimeInput.value = `${String(event.hour).padStart(2, '0')}:${String(event.minute).padStart(2, '0')}`;
          ids.eventRecurringInput.checked = !!event.recurring;
          ids.eventDowMaskInput.value = String(event.dow_mask || 127);
          if (!event.recurring) {
            ids.eventDateInput.value = `${event.year}-${String(event.month).padStart(2, '0')}-${String(event.day).padStart(2, '0')}`;
          } else {
            ids.eventDateInput.value = '';
          }
        });
      });

      ids.scheduleList.querySelectorAll('button[data-delete]').forEach(button => {
        button.addEventListener('click', async () => {
          const id = Number(button.getAttribute('data-delete'));
          setBusy(true);
          try {
            const body = new URLSearchParams();
            body.set('id', String(id));
            const data = await parseResponse(await fetch('/schedule/delete', {
              method: 'POST',
              headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
              body: body.toString()
            }));
            showTimeStatus(data);
          } catch (err) {
            showJson({ ok: false, error: err.error || 'Delete schedule failed' });
          } finally {
            setBusy(false);
          }
        });
      });
    }

    function showTimeStatus(obj) {
      if (!obj || !obj.ok) {
        return;
      }

      timeState = obj;
      ids.timeEnabledInput.checked = !!obj.enabled;
      if (obj.server) ids.timeServerInput.value = obj.server;
      if (obj.timezone) ids.timeTimezoneInput.value = obj.timezone;
      ids.timeValid.textContent = obj.time_valid ? 'yes' : 'no';
      ids.systemTime.textContent = obj.now_local || 'n/a';
      ids.systemEpoch.textContent = typeof obj.now_epoch !== 'undefined' ? String(obj.now_epoch) : 'n/a';
      ids.timeSyncStatus.textContent = obj.last_sync_status || 'unknown';
      ids.timeSyncEpoch.textContent = typeof obj.last_sync_epoch !== 'undefined' ? String(obj.last_sync_epoch) : 'n/a';
      ids.scheduleSlots.textContent = `${obj.count || 0}/${obj.capacity || 10}`;
      renderScheduleList(obj.events || []);
    }

    function showOtaStatus(obj) {
      if (!obj || !obj.ok) {
        ids.otaStatus.textContent = (obj && obj.error) ? obj.error : 'OTA status unavailable';
        return;
      }

      if (obj.current_version) ids.otaCurrentVersion.textContent = obj.current_version;
      ids.otaLatestVersion.textContent = obj.latest_version || 'n/a';
      ids.otaUpdateAvailable.textContent = obj.update_available ? 'yes' : 'no';
      ids.otaStatus.textContent = obj.message || 'ready';
    }

    function showTemperatureStatus(obj) {
      if (!obj || !obj.ok) {
        return;
      }

      ids.tempCalProbePresent.textContent = obj.probe_present ? 'yes' : 'no';
      ids.tempProbePresent.textContent = obj.probe_present ? 'yes' : 'no';
      ids.statusTempProbePresent.textContent = obj.probe_present ? 'yes' : 'no';
      const probeRaw = Number(obj.probe_raw);
      const probeRawText = Number.isFinite(probeRaw) && probeRaw >= 0 ? String(probeRaw) : 'n/a';
      ids.tempProbeRaw.textContent = probeRawText;
      ids.statusTempProbeRaw.textContent = probeRawText;
      const liveRaw = Number(obj.current_temperature_raw);
      const liveRawText = Number.isFinite(liveRaw) && liveRaw >= 0 ? String(liveRaw) : 'n/a';
      ids.tempCalLiveRaw.textContent = liveRawText;
      ids.currentTemperatureRaw.textContent = liveRawText;
      ids.statusCurrentTemperatureRaw.textContent = liveRawText;

      const computed = Number(obj.current_temperature_c);
      ids.tempCalComputedC.textContent = Number.isFinite(computed) ? `${computed.toFixed(2)} C` : 'n/a';
      ids.tempCalReady.textContent = obj.calibration_ready ? 'yes' : 'no';
      const trim = Number(obj.trim_offset_c);
      ids.tempCalTrimOffset.textContent = Number.isFinite(trim) ? `${trim.toFixed(2)} C` : '0.00 C';

      const low = obj.low_point || {};
      const high = obj.high_point || {};
      ids.tempCalLowPoint.textContent = low.valid ? `raw ${low.raw}, ${Number(low.temp_c).toFixed(2)} C` : 'not set';
      ids.tempCalHighPoint.textContent = high.valid ? `raw ${high.raw}, ${Number(high.temp_c).toFixed(2)} C` : 'not set';
    }

    async function parseResponse(res) {
      let data;
      try {
        data = await res.json();
      } catch {
        data = { ok: false, error: 'Invalid JSON response' };
      }

      if (!res.ok) {
        throw data;
      }

      return data;
    }

    async function refreshStatus() {
      setBusy(true);

      try {
        // Update relay and core status first so the UI is responsive even if OTA check is slow.
        const status = await parseResponse(await fetch('/status'));
        showJson(status);

        const [config, timeStatus, temperatureStatus] = await Promise.all([
          parseResponse(await fetch('/config')),
          parseResponse(await fetch('/time')),
          parseResponse(await fetch('/temperature'))
        ]);
        showConfig(config);
        showTimeStatus(timeStatus);
        showTemperatureStatus(temperatureStatus);

        try {
          const otaStatus = await parseResponse(await fetch('/ota/check'));
          showOtaStatus(otaStatus);
        } catch (otaErr) {
          showOtaStatus(otaErr || { ok: false, error: 'OTA status unavailable' });
        }
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Status request failed' });
      } finally {
        setBusy(false);
      }
    }

    async function pollLiveTemperature() {
      try {
        const temperature = await parseResponse(await fetch('/temperature'));
        showTemperatureStatus(temperature);
      } catch {
        // Silent polling failures keep the page usable during reconnects.
      }
    }

    async function checkOtaUpdate() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/ota/check', { method: 'POST' }));
        showOtaStatus(data);
        showJson({ ok: true, command: 'ota-check' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'OTA check failed' });
      } finally {
        setBusy(false);
      }
    }

    async function installOtaUpdate() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/ota/update', { method: 'POST' }));
        showOtaStatus(data);
        showJson({ ok: true, command: 'ota-update' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'OTA update failed' });
      } finally {
        setBusy(false);
      }
    }

    async function sendCommand(path, commandName) {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch(path, { method: 'POST' }));
        showJson(data);

        const status = await parseResponse(await fetch('/status'));
        showJson(status);
        ids.last.textContent = commandName;
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Command request failed' });
      } finally {
        setBusy(false);
      }
    }

    async function runLedTest(path, commandName) {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch(path, { method: 'POST' }));
        showJson(data);
        ids.last.textContent = commandName;
      } catch (err) {
        showJson({ ok: false, error: err.error || 'LED test request failed' });
      } finally {
        setBusy(false);
      }
    }

    async function scanWifi() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/wifi/scan'));
        showJson(data);
        renderWifiList(data);
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Wi-Fi scan failed' });
      } finally {
        setBusy(false);
      }
    }

    async function applyConfig() {
      const hostname = ids.hostnameInput.value.trim();
      const mqttHost = ids.mqttHostInput.value.trim();
      const mqttPort = ids.mqttPortInput.value.trim();
      const mqttUsername = ids.mqttUsernameInput.value.trim();
      const mqttPassword = ids.mqttPasswordInput.value;
      const relayAutoOffText = ids.relayAutoOffMinutesInput.value.trim();
      const relayAutoOffParsed = relayAutoOffText.length > 0 ? Number(relayAutoOffText) : currentRelayAutoOffMinutes;
      const relayAutoOffMinutes = Number.isFinite(relayAutoOffParsed) ? Math.max(0, Math.floor(relayAutoOffParsed)) : currentRelayAutoOffMinutes;
      const mqttEnabled = !ids.mqttDisabledInput.checked;
      const otaAutoScheduleEnabled = !ids.otaAutoScheduleDisabledInput.checked;
      const wifiSsid = ids.wifiSsidInput.value.trim();
      const wifiPass = ids.wifiPassInput.value;
      const mqttEnabledChanged = mqttEnabled !== currentMqttEnabled;
      const mqttAuthChanged = mqttUsername !== currentMqttUsername || mqttPassword.trim().length > 0;
      const otaAutoScheduleChanged = otaAutoScheduleEnabled !== currentOtaAutoScheduleEnabled;
      const relayAutoOffChanged = relayAutoOffMinutes !== currentRelayAutoOffMinutes;

      if (!hostname && !mqttHost && !mqttPort && !wifiSsid && !wifiPass.trim() && !mqttEnabledChanged && !mqttAuthChanged && !otaAutoScheduleChanged && !relayAutoOffChanged) {
        showJson({ ok: false, error: 'enter at least one setting to save' });
        return;
      }

      setBusy(true);
      try {
        const body = new URLSearchParams();
        if (hostname) body.set('hostname', hostname);
        if (mqttHost) body.set('mqtt_host', mqttHost);
        if (mqttPort) body.set('mqtt_port', mqttPort);
        if (mqttAuthChanged) {
          body.set('mqtt_user', mqttUsername);
          if (mqttPassword.trim().length > 0 || mqttUsername.length === 0) {
            body.set('mqtt_pass', mqttPassword);
          }
        }
        if (mqttEnabledChanged) body.set('mqtt_enabled', mqttEnabled ? '1' : '0');
        if (otaAutoScheduleChanged) body.set('ota_auto_schedule_enabled', otaAutoScheduleEnabled ? '1' : '0');
        if (relayAutoOffChanged) body.set('relay_auto_off_minutes', String(relayAutoOffMinutes));
        if (wifiSsid) body.set('wifi_ssid', wifiSsid);
        if (wifiPass.trim()) body.set('wifi_pass', wifiPass);

        const response = await fetch('/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });

        const data = await parseResponse(response);
        showJson(data);

        const status = await parseResponse(await fetch('/status'));
        const config = await parseResponse(await fetch('/config'));
        showJson(status);
        showConfig(config);
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Configuration save failed' });
      } finally {
        setBusy(false);
      }
    }

    async function captureTemperaturePoint(endpoint, inputElement) {
      const tempText = inputElement.value.trim();
      const temp = Number(tempText);
      if (!Number.isFinite(temp)) {
        showJson({ ok: false, error: 'Enter a valid reference temperature' });
        return;
      }

      setBusy(true);
      try {
        const body = new URLSearchParams();
        body.set('temp_value', String(temp));
        body.set('temp_unit', ids.tempCalUnitInput.value || 'c');
        const data = await parseResponse(await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        }));
        showTemperatureStatus(data);
        showJson({ ok: true, command: endpoint.includes('low') ? 'temp-capture-low' : 'temp-capture-high' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Calibration capture failed' });
      } finally {
        setBusy(false);
      }
    }

    async function resetTemperatureCalibration() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/temperature/calibration/reset', { method: 'POST' }));
        showTemperatureStatus(data);
        showJson({ ok: true, command: 'temp-calibration-reset' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Calibration reset failed' });
      } finally {
        setBusy(false);
      }
    }

    async function setTemperatureTrimOffset(offsetC) {
      setBusy(true);
      try {
        const body = new URLSearchParams();
        body.set('offset_c', String(offsetC));
        const data = await parseResponse(await fetch('/temperature/trim', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        }));
        showTemperatureStatus(data);
        showJson({ ok: true, command: 'temp-trim-set', trim_offset_c: data.trim_offset_c });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Temperature trim update failed' });
      } finally {
        setBusy(false);
      }
    }

    async function adjustTemperatureTrim(deltaC) {
      const currentText = (ids.tempCalTrimOffset.textContent || '0').replace(' C', '').trim();
      const current = Number(currentText);
      const base = Number.isFinite(current) ? current : 0;
      await setTemperatureTrimOffset(base + deltaC);
    }

    async function saveTimeConfig() {
      setBusy(true);
      try {
        const body = new URLSearchParams();
        body.set('time_enabled', ids.timeEnabledInput.checked ? '1' : '0');
        body.set('time_server', ids.timeServerInput.value.trim());
        body.set('time_timezone', ids.timeTimezoneInput.value.trim());

        const data = await parseResponse(await fetch('/time/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        }));

        showTimeStatus(data);
        showJson({ ok: true, command: 'save-time-config' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Saving time config failed' });
      } finally {
        setBusy(false);
      }
    }

    async function syncTimeNow() {
      setBusy(true);
      try {
        const data = await parseResponse(await fetch('/time/sync', { method: 'POST' }));
        showTimeStatus(data);
        showJson({ ok: true, command: 'sync-time' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Time sync failed' });
      } finally {
        setBusy(false);
      }
    }

    async function saveScheduleEvent() {
      const command = ids.eventCommandInput.value.trim();
      const timeHm = ids.eventTimeInput.value.trim();
      const recurring = ids.eventRecurringInput.checked;
      const dowMask = ids.eventDowMaskInput.value.trim() || '127';
      const dateYmd = ids.eventDateInput.value.trim();
      const id = ids.eventIdInput.value.trim();

      if (!command || !timeHm) {
        showJson({ ok: false, error: 'Schedule command and time are required' });
        return;
      }

      const timeCheck = validateTimeHm(timeHm);
      if (!timeCheck.ok) {
        showJson({ ok: false, error: timeCheck.error });
        return;
      }

      if (!recurring && !dateYmd) {
        showJson({ ok: false, error: 'One-time events require date YYYY-MM-DD' });
        return;
      }

      if (recurring) {
        const maskCheck = validateDowMask(dowMask);
        if (!maskCheck.ok) {
          showJson({ ok: false, error: maskCheck.error });
          return;
        }
      }
      else {
        const dateCheck = validateDateYmd(dateYmd);
        if (!dateCheck.ok) {
          showJson({ ok: false, error: dateCheck.error });
          return;
        }
      }

      setBusy(true);
      try {
        const body = new URLSearchParams();
        body.set('enabled', '1');
        body.set('recurring', recurring ? '1' : '0');
        body.set('command', command);
        body.set('time_hm', timeHm);
        if (recurring) {
          body.set('dow_mask', dowMask);
        } else {
          body.set('date_ymd', dateYmd);
        }

        const endpoint = id ? '/schedule/update' : '/schedule/add';
        if (id) {
          body.set('id', id);
        }

        const data = await parseResponse(await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        }));

        showTimeStatus(data);
        resetScheduleForm();
        showJson({ ok: true, command: id ? 'update-schedule' : 'add-schedule' });
      } catch (err) {
        showJson({ ok: false, error: err.error || 'Saving schedule failed' });
      } finally {
        setBusy(false);
      }
    }

    document.getElementById('btnOn').addEventListener('click', () => sendCommand('/on', 'on'));
    document.getElementById('btnOff').addEventListener('click', () => sendCommand('/off', 'off'));
    document.getElementById('btnToggle').addEventListener('click', () => sendCommand('/toggle', 'toggle'));
    document.getElementById('btnRefresh').addEventListener('click', refreshStatus);
    document.getElementById('btnRelayLedTest').addEventListener('click', () => runLedTest('/test/relay-led', 'test-relay-led'));
    document.getElementById('btnWifiLedTest').addEventListener('click', () => runLedTest('/test/wifi-led', 'test-wifi-led'));
    document.getElementById('btnAllLedTests').addEventListener('click', () => runLedTest('/test/leds', 'test-leds'));
    document.getElementById('btnSaveConfig').addEventListener('click', applyConfig);
    document.getElementById('btnScanWifi').addEventListener('click', scanWifi);
    document.getElementById('btnSaveTimeConfig').addEventListener('click', saveTimeConfig);
    document.getElementById('btnTimeSyncNow').addEventListener('click', syncTimeNow);
    document.getElementById('btnScheduleSave').addEventListener('click', saveScheduleEvent);
    document.getElementById('btnScheduleReset').addEventListener('click', resetScheduleForm);
    document.getElementById('btnOtaCheck').addEventListener('click', checkOtaUpdate);
    document.getElementById('btnOtaUpdate').addEventListener('click', installOtaUpdate);
    document.getElementById('btnTempCaptureLow').addEventListener('click', () => captureTemperaturePoint('/temperature/capture-low', ids.tempLowInput));
    document.getElementById('btnTempCaptureHigh').addEventListener('click', () => captureTemperaturePoint('/temperature/capture-high', ids.tempHighInput));
    document.getElementById('btnTempResetCal').addEventListener('click', resetTemperatureCalibration);
    document.getElementById('btnTempTrimMinus1').addEventListener('click', () => adjustTemperatureTrim(-1.0));
    document.getElementById('btnTempTrimMinus02').addEventListener('click', () => adjustTemperatureTrim(-0.2));
    document.getElementById('btnTempTrimPlus02').addEventListener('click', () => adjustTemperatureTrim(0.2));
    document.getElementById('btnTempTrimPlus1').addEventListener('click', () => adjustTemperatureTrim(1.0));
    document.getElementById('btnTempTrimReset').addEventListener('click', () => setTemperatureTrimOffset(0.0));

    resetScheduleForm();
    refreshStatus();
    setInterval(pollLiveTemperature, 1000);
  </script>
</body>
</html>
)HTML";
} // namespace

void WebControlServer::configure(const WebControlContext &newContext)
{
  context = newContext;
}

void WebControlServer::beginIfNeeded(bool wifiConnected)
{
  if (started || !wifiConnected)
  {
    return;
  }

  registerRoutes();
  gServer.begin();
  started = true;

  Serial.println("[HTTP] Web control server started on port 80");
}

void WebControlServer::handleClient()
{
  if (!started)
  {
    return;
  }

  gServer.handleClient();
}

bool WebControlServer::isStarted() const
{
  return started;
}

void WebControlServer::registerRoutes()
{
  gServer.on("/", HTTP_GET, [this]()
             { handleRoot(); });
  gServer.on("/on", HTTP_POST, [this]()
             { handleOn(); });
  gServer.on("/off", HTTP_POST, [this]()
             { handleOff(); });
  gServer.on("/toggle", HTTP_POST, [this]()
             { handleToggle(); });
  gServer.on("/test/relay-led", HTTP_POST, [this]()
             { handleRelayLedTest(); });
  gServer.on("/test/wifi-led", HTTP_POST, [this]()
             { handleWifiLedTest(); });
  gServer.on("/test/leds", HTTP_POST, [this]()
             { handleAllLedTests(); });
  gServer.on("/status", HTTP_GET, [this]()
             { handleStatus(); });
  gServer.on("/config", HTTP_GET, [this]()
             { handleConfig(); });
  gServer.on("/config", HTTP_POST, [this]()
             { handleConfigSave(); });
  gServer.on("/time", HTTP_GET, [this]()
             { handleTimeStatus(); });
  gServer.on("/time/config", HTTP_POST, [this]()
             { handleTimeConfigSave(); });
  gServer.on("/time/sync", HTTP_POST, [this]()
             { handleTimeSyncNow(); });
  gServer.on("/schedule/add", HTTP_POST, [this]()
             { handleScheduleAdd(); });
  gServer.on("/schedule/update", HTTP_POST, [this]()
             { handleScheduleUpdate(); });
  gServer.on("/schedule/delete", HTTP_POST, [this]()
             { handleScheduleDelete(); });
  gServer.on("/ota/check", HTTP_GET, [this]()
             { handleOtaCheck(); });
  gServer.on("/ota/check", HTTP_POST, [this]()
             { handleOtaCheck(); });
  gServer.on("/ota/update", HTTP_POST, [this]()
             { handleOtaUpdate(); });
  gServer.on("/temperature", HTTP_GET, [this]()
             { handleTemperatureStatus(); });
  gServer.on("/temperature/capture-low", HTTP_POST, [this]()
             { handleTemperatureCaptureLow(); });
  gServer.on("/temperature/capture-high", HTTP_POST, [this]()
             { handleTemperatureCaptureHigh(); });
  gServer.on("/temperature/trim", HTTP_POST, [this]()
             { handleTemperatureTrimOffset(); });
  gServer.on("/temperature/calibration/reset", HTTP_POST, [this]()
             { handleTemperatureCalibrationReset(); });
  gServer.on("/favicon.ico", HTTP_GET, []()
             { gServer.send(204); });
  gServer.on("/wifi/scan", HTTP_GET, [this]()
             { handleWifiScan(); });
  gServer.on("/hostname", HTTP_POST, [this]()
             { handleSetHostname(); });
  gServer.onNotFound([this]()
                     { handleNotFound(); });
}

bool WebControlServer::dispatchCommand(const char *command) const
{
  if (context.router == nullptr)
  {
    return false;
  }

  return context.router->dispatch(String(command));
}

const char *WebControlServer::relayStateText() const
{
  if (context.relay == nullptr)
  {
    return "unknown";
  }

  return context.relay->isOn() ? "on" : "off";
}

void WebControlServer::sendError(int statusCode, const char *error)
{
  String json = "{\"ok\":false,\"error\":\"";
  json += error;
  json += "\"}";
  gServer.send(statusCode, "application/json", json);
}

void WebControlServer::handleRoot()
{
  Serial.println("[HTTP] GET /");
  gServer.send_P(200, "text/html", WEB_UI);
}

void WebControlServer::handleOn()
{
  Serial.println("[HTTP] POST /on");

  if (!dispatchCommand("on"))
  {
    sendError(500, "Failed to dispatch on command");
    return;
  }

  String json = "{\"ok\":true,\"command\":\"on\",\"relay\":\"";
  json += relayStateText();
  json += "\"}";
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleOff()
{
  Serial.println("[HTTP] POST /off");

  if (!dispatchCommand("off"))
  {
    sendError(500, "Failed to dispatch off command");
    return;
  }

  String json = "{\"ok\":true,\"command\":\"off\",\"relay\":\"";
  json += relayStateText();
  json += "\"}";
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleToggle()
{
  Serial.println("[HTTP] POST /toggle");

  const char *previous = relayStateText();

  if (!dispatchCommand("toggle"))
  {
    const bool wasOn = context.relay != nullptr ? context.relay->isOn() : false;
    if (!dispatchCommand(wasOn ? "off" : "on"))
    {
      sendError(500, "Failed to dispatch toggle command");
      return;
    }
  }

  const char *current = relayStateText();

  String json = "{\"ok\":true,\"command\":\"toggle\",\"previous_relay\":\"";
  json += previous;
  json += "\",\"relay\":\"";
  json += current;
  json += "\"}";
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleStatus()
{
  Serial.println("[HTTP] GET /status");

  String json = "{\"ok\":true,\"relay\":\"";
  json += relayStateText();
  json += "\",\"hostname\":\"";
  json += context.getHostname != nullptr ? context.getHostname() : String("unknown");
  json += "\",\"mqtt_client_id\":\"";
  json += context.getMqttClientId != nullptr ? context.getMqttClientId() : String("unknown");
  json += "\",\"firmware_version\":\"";
  json += FIRMWARE_VERSION;
  json += "\",\"uptime_ms\":";
  json += millis();
  json += ",\"relay_auto_off_minutes\":";
  json += context.getRelayAutoOffMinutes != nullptr ? context.getRelayAutoOffMinutes() : 0;
  json += ",\"relay_auto_off_armed\":";
  json += context.getRelayAutoOffArmed != nullptr ? (context.getRelayAutoOffArmed() ? "true" : "false") : "false";
  json += ",\"relay_auto_off_remaining_s\":";
  json += context.getRelayAutoOffRemainingSeconds != nullptr ? context.getRelayAutoOffRemainingSeconds() : 0;
  json += ",\"temperature_probe_present\":";
  json += context.getTemperatureProbePresent != nullptr ? (context.getTemperatureProbePresent() ? "true" : "false") : "false";
  json += ",\"temperature_probe_raw\":";
  if (context.getTemperatureProbeRaw != nullptr)
  {
    json += context.getTemperatureProbeRaw();
  }
  else
  {
    json += -1;
  }
  json += ",\"current_temperature_raw\":";
  if (context.getCurrentTemperatureRaw != nullptr)
  {
    json += context.getCurrentTemperatureRaw();
  }
  else
  {
    json += -1;
  }
  json += ",\"current_temperature_c\":";
  if (context.getCurrentTemperatureC != nullptr)
  {
    const float currentTempC = context.getCurrentTemperatureC();
    json += isnan(currentTempC) ? "null" : String(currentTempC, 2);
  }
  else
  {
    json += "null";
  }
  json += ",\"temperature_calibration_ready\":";
  json += context.getTemperatureCalibrationReady != nullptr ? (context.getTemperatureCalibrationReady() ? "true" : "false") : "false";
  json += ",\"temperature_trim_offset_c\":";
  if (context.getTemperatureTrimOffsetC != nullptr)
  {
    json += String(context.getTemperatureTrimOffsetC(), 2);
  }
  else
  {
    json += "0.00";
  }
  json += "}";
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleConfig()
{
  Serial.println("[HTTP] GET /config");
  gServer.send(200, "application/json", buildConfigJson(context));
}

void WebControlServer::handleConfigSave()
{
  Serial.println("[HTTP] POST /config");

  bool wifiChanged = false;
  bool settingsSaved = false;

  if (context.wifi != nullptr)
  {
    const String wifiSsid = gServer.arg("wifi_ssid");
    const String wifiPass = gServer.arg("wifi_pass");
    if (wifiSsid.length() > 0)
    {
      context.wifi->setCredentials(wifiSsid, wifiPass);
      wifiChanged = true;
      settingsSaved = true;
    }
  }

  if (context.mqtt != nullptr)
  {
    const String mqttHost = gServer.arg("mqtt_host");
    const String mqttPortText = gServer.arg("mqtt_port");
    const bool hasMqttUserArg = gServer.hasArg("mqtt_user");
    const bool hasMqttPassArg = gServer.hasArg("mqtt_pass");
    const String mqttUser = gServer.arg("mqtt_user");
    const String mqttPass = gServer.arg("mqtt_pass");
    const String mqttEnabledText = gServer.arg("mqtt_enabled");
    if (mqttHost.length() > 0 || mqttPortText.length() > 0)
    {
      const String currentHost = context.mqtt->serverHost();
      const int currentPort = context.mqtt->serverPort();
      const String nextHost = mqttHost.length() > 0 ? mqttHost : currentHost;
      int nextPort = currentPort;
      if (mqttPortText.length() > 0)
      {
        nextPort = mqttPortText.toInt();
      }
      if (nextPort <= 0)
      {
        nextPort = MQTT_PORT;
      }
      context.mqtt->setServer(nextHost, nextPort);
      settingsSaved = true;
    }

    if (mqttEnabledText.length() > 0)
    {
      context.mqtt->setEnabled(parseBoolArg(mqttEnabledText));
      settingsSaved = true;
    }

    if (hasMqttUserArg || hasMqttPassArg)
    {
      const String nextUser = hasMqttUserArg ? mqttUser : context.mqtt->username();
      String nextPass = hasMqttPassArg ? mqttPass : context.mqtt->password();
      if (hasMqttUserArg && mqttUser.length() == 0 && !hasMqttPassArg)
      {
        nextPass = String();
      }

      context.mqtt->setCredentials(nextUser, nextPass);
      settingsSaved = true;
    }
  }

  const String otaAutoScheduleEnabledText = gServer.arg("ota_auto_schedule_enabled");
  if (otaAutoScheduleEnabledText.length() > 0)
  {
    if (context.setOtaAutoScheduleEnabled == nullptr)
    {
      sendError(500, "OTA auto schedule setter is not available");
      return;
    }

    String error;
    if (!context.setOtaAutoScheduleEnabled(parseBoolArg(otaAutoScheduleEnabledText), error))
    {
      sendError(400, error.length() > 0 ? error.c_str() : "Failed to update OTA auto schedule setting");
      return;
    }

    settingsSaved = true;
  }

  const String relayAutoOffMinutesText = gServer.arg("relay_auto_off_minutes");
  if (relayAutoOffMinutesText.length() > 0)
  {
    if (context.setRelayAutoOffMinutes == nullptr)
    {
      sendError(500, "Relay auto-off setter is not available");
      return;
    }

    const int relayAutoOffMinutes = relayAutoOffMinutesText.toInt();
    String error;
    if (!context.setRelayAutoOffMinutes(relayAutoOffMinutes, error))
    {
      sendError(400, error.length() > 0 ? error.c_str() : "Failed to update relay auto-off setting");
      return;
    }

    settingsSaved = true;
  }

  const String requestedHostname = gServer.arg("hostname");
  if (requestedHostname.length() > 0)
  {
    if (context.setHostname == nullptr)
    {
      sendError(500, "Hostname setter is not available");
      return;
    }

    String error;
    if (!context.setHostname(requestedHostname, error))
    {
      sendError(400, error.length() > 0 ? error.c_str() : "Invalid hostname");
      return;
    }

    settingsSaved = true;
  }

  if (wifiChanged && requestedHostname.length() == 0 && context.wifi != nullptr)
  {
    context.wifi->forceReconnect();
  }

  if (!settingsSaved)
  {
    gServer.send(200, "application/json", buildConfigJson(context));
    return;
  }

  String json = buildConfigJson(context);
  if (json.endsWith("}"))
  {
    json.remove(json.length() - 1);
    json += ",\"restarting\":true}";
  }

  gServer.send(200, "application/json", json);
  delay(150);
  ESP.restart();
}

void WebControlServer::handleTimeStatus()
{
  Serial.println("[HTTP] GET /time");

  if (context.timeSync == nullptr || context.schedule == nullptr)
  {
    sendError(500, "Time manager or schedule manager unavailable");
    return;
  }

  const time_t now = time(nullptr);
  const String nowLocal = formatLocalNow();

  String json = "{";
  json += "\"ok\":true,";
  json += "\"enabled\":";
  json += context.timeSync->isEnabled() ? "true" : "false";
  json += ",";
  json += "\"server\":\"";
  json += jsonEscape(context.timeSync->server());
  json += "\",";
  json += "\"timezone\":\"";
  json += jsonEscape(context.timeSync->timezone());
  json += "\",";
  json += "\"time_valid\":";
  json += context.timeSync->isTimeValid() ? "true" : "false";
  json += ",";
  json += "\"now_epoch\":";
  json += static_cast<unsigned long>(now);
  json += ",";
  json += "\"now_local\":\"";
  json += jsonEscape(nowLocal);
  json += "\",";
  json += "\"last_sync_epoch\":";
  json += static_cast<unsigned long>(context.timeSync->lastSyncEpoch());
  json += ",";
  json += "\"last_sync_status\":\"";
  json += jsonEscape(context.timeSync->lastSyncStatus());
  json += "\",";
  json += "\"count\":";
  json += context.schedule->count();
  json += ",";
  json += "\"capacity\":";
  json += context.schedule->capacity();
  json += ",";
  json += "\"events\":";
  json += context.schedule->eventsJson();
  json += "}";

  gServer.send(200, "application/json", json);
}

void WebControlServer::handleTimeConfigSave()
{
  Serial.println("[HTTP] POST /time/config");

  if (context.timeSync == nullptr)
  {
    sendError(500, "Time manager unavailable");
    return;
  }

  const String enabledText = gServer.arg("time_enabled");
  const String serverText = gServer.arg("time_server");
  const String timezoneText = gServer.arg("time_timezone");

  if (enabledText.length() > 0)
  {
    context.timeSync->setEnabled(parseBoolArg(enabledText));
  }

  if (serverText.length() > 0)
  {
    context.timeSync->setServer(serverText);
  }

  if (timezoneText.length() > 0)
  {
    context.timeSync->setTimezone(timezoneText);
  }

  handleTimeStatus();
}

void WebControlServer::handleTimeSyncNow()
{
  Serial.println("[HTTP] POST /time/sync");

  if (context.timeSync == nullptr)
  {
    sendError(500, "Time manager unavailable");
    return;
  }

  context.timeSync->forceSync();
  handleTimeStatus();
}

void WebControlServer::handleScheduleAdd()
{
  Serial.println("[HTTP] POST /schedule/add");

  if (context.schedule == nullptr)
  {
    sendError(500, "Schedule manager unavailable");
    return;
  }

  ScheduleManager::EventData data;
  const String enabledText = gServer.arg("enabled");
  const String recurringText = gServer.arg("recurring");
  data.enabled = enabledText.length() > 0 ? parseBoolArg(enabledText) : true;
  data.recurring = recurringText.length() > 0 ? parseBoolArg(recurringText) : true;
  data.command = gServer.arg("command");
  data.command.trim();

  const String hm = gServer.arg("time_hm");
  if (!parseHourMinute(hm, data.hour, data.minute))
  {
    sendError(400, "Invalid time_hm. Use HH:MM");
    return;
  }

  if (data.recurring)
  {
    const String maskText = gServer.arg("dow_mask");
    if (maskText.length() == 0)
    {
      data.dowMask = 0x7F;
    }
    else
    {
      const int parsedMask = maskText.toInt();
      if (parsedMask < 0 || parsedMask > 127)
      {
        sendError(400, "Invalid dow_mask. Use 0-127 with Monday as bit 0");
        return;
      }
      data.dowMask = static_cast<uint8_t>(parsedMask);
    }
    data.year = 0;
    data.month = 0;
    data.day = 0;
  }
  else
  {
    const String date = gServer.arg("date_ymd");
    if (!parseDateYmd(date, data.year, data.month, data.day))
    {
      sendError(400, "Invalid date_ymd. Use YYYY-MM-DD");
      return;
    }
    data.dowMask = 0;
  }

  String error;
  uint8_t createdId = 0;
  if (!context.schedule->addEvent(data, createdId, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Failed to add schedule");
    return;
  }

  handleTimeStatus();
}

void WebControlServer::handleScheduleUpdate()
{
  Serial.println("[HTTP] POST /schedule/update");

  if (context.schedule == nullptr)
  {
    sendError(500, "Schedule manager unavailable");
    return;
  }

  const int id = gServer.arg("id").toInt();
  if (id <= 0)
  {
    sendError(400, "Missing valid id");
    return;
  }

  ScheduleManager::EventData data;
  const String enabledText = gServer.arg("enabled");
  const String recurringText = gServer.arg("recurring");
  data.enabled = enabledText.length() > 0 ? parseBoolArg(enabledText) : true;
  data.recurring = recurringText.length() > 0 ? parseBoolArg(recurringText) : true;
  data.command = gServer.arg("command");
  data.command.trim();

  const String hm = gServer.arg("time_hm");
  if (!parseHourMinute(hm, data.hour, data.minute))
  {
    sendError(400, "Invalid time_hm. Use HH:MM");
    return;
  }

  if (data.recurring)
  {
    const String maskText = gServer.arg("dow_mask");
    if (maskText.length() == 0)
    {
      data.dowMask = 0x7F;
    }
    else
    {
      const int parsedMask = maskText.toInt();
      if (parsedMask < 0 || parsedMask > 127)
      {
        sendError(400, "Invalid dow_mask. Use 0-127 with Monday as bit 0");
        return;
      }
      data.dowMask = static_cast<uint8_t>(parsedMask);
    }
    data.year = 0;
    data.month = 0;
    data.day = 0;
  }
  else
  {
    const String date = gServer.arg("date_ymd");
    if (!parseDateYmd(date, data.year, data.month, data.day))
    {
      sendError(400, "Invalid date_ymd. Use YYYY-MM-DD");
      return;
    }
    data.dowMask = 0;
  }

  String error;
  if (!context.schedule->updateEvent(static_cast<uint8_t>(id), data, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Failed to update schedule");
    return;
  }

  handleTimeStatus();
}

void WebControlServer::handleScheduleDelete()
{
  Serial.println("[HTTP] POST /schedule/delete");

  if (context.schedule == nullptr)
  {
    sendError(500, "Schedule manager unavailable");
    return;
  }

  const int id = gServer.arg("id").toInt();
  if (id <= 0)
  {
    sendError(400, "Missing valid id");
    return;
  }

  if (!context.schedule->removeEvent(static_cast<uint8_t>(id)))
  {
    sendError(404, "Schedule event not found");
    return;
  }

  handleTimeStatus();
}

void WebControlServer::handleOtaCheck()
{
  Serial.println("[HTTP] OTA check");

  if (context.ota == nullptr)
  {
    sendError(500, "OTA manager unavailable");
    return;
  }

  const OtaCheckResult result = context.ota->checkForUpdate();

  String json = "{";
  json += "\"ok\":";
  json += result.ok ? "true" : "false";
  json += ",";
  json += "\"configured\":";
  json += result.configured ? "true" : "false";
  json += ",";
  json += "\"current_version\":\"";
  json += jsonEscape(result.currentVersion);
  json += "\",";
  json += "\"latest_version\":\"";
  json += jsonEscape(result.latestVersion);
  json += "\",";
  json += "\"update_available\":";
  json += result.updateAvailable ? "true" : "false";
  json += ",";
  json += "\"asset_url\":\"";
  json += jsonEscape(result.assetUrl);
  json += "\",";
  json += "\"message\":\"";
  json += jsonEscape(result.message);
  json += "\"}";

  gServer.send(result.ok ? 200 : 400, "application/json", json);
}

void WebControlServer::handleOtaUpdate()
{
  Serial.println("[HTTP] OTA update");

  if (context.ota == nullptr)
  {
    sendError(500, "OTA manager unavailable");
    return;
  }

  String message;
  if (!context.ota->performUpdate(message))
  {
    String json = "{\"ok\":false,\"message\":\"";
    json += jsonEscape(message);
    json += "\"}";
    gServer.send(400, "application/json", json);
    return;
  }

  String json = "{\"ok\":true,\"update_available\":false,\"message\":\"Update installed. Rebooting device.\"}";
  gServer.send(200, "application/json", json);
  delay(150);
  ESP.restart();
}

void WebControlServer::handleTemperatureStatus()
{
  Serial.println("[HTTP] GET /temperature");
  gServer.send(200, "application/json", buildTemperatureJson(context));
}

void WebControlServer::handleRelayLedTest()
{
  Serial.println("[HTTP] POST /test/relay-led");

  if (context.startRelayLedTest == nullptr || !context.startRelayLedTest())
  {
    sendError(500, "Relay LED test is unavailable");
    return;
  }

  gServer.send(200, "application/json", buildLedTestJson(context, "test-relay-led", "Relay LED test started for 5 seconds"));
}

void WebControlServer::handleWifiLedTest()
{
  Serial.println("[HTTP] POST /test/wifi-led");

  if (context.startWifiLedTest == nullptr || !context.startWifiLedTest())
  {
    sendError(500, "Wi-Fi LED test is unavailable");
    return;
  }

  gServer.send(200, "application/json", buildLedTestJson(context, "test-wifi-led", "Wi-Fi LED test started for 5 seconds"));
}

void WebControlServer::handleAllLedTests()
{
  Serial.println("[HTTP] POST /test/leds");

  if (context.startAllLedTests == nullptr || !context.startAllLedTests())
  {
    sendError(500, "LED tests are unavailable");
    return;
  }

  gServer.send(200, "application/json", buildLedTestJson(context, "test-leds", "Both LED tests started for 5 seconds"));
}

void WebControlServer::handleTemperatureCaptureLow()
{
  Serial.println("[HTTP] POST /temperature/capture-low");

  if (context.captureLowCalibration == nullptr)
  {
    sendError(500, "Temperature low capture is unavailable");
    return;
  }

  String tempText = gServer.arg("temp_value");
  if (tempText.length() == 0)
  {
    tempText = gServer.arg("temp_c");
  }
  if (tempText.length() == 0)
  {
    sendError(400, "Missing temp_value");
    return;
  }

  const String unitText = gServer.arg("temp_unit");
  float knownTempC = NAN;
  String error;
  if (!parseCalibrationTemperatureC(tempText, unitText, knownTempC, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Invalid calibration temperature");
    return;
  }

  if (!context.captureLowCalibration(knownTempC, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Failed to capture low calibration");
    return;
  }

  String json = buildTemperatureJson(context);
  if (json.endsWith("}"))
  {
    json.remove(json.length() - 1);
    json += ",\"message\":\"Low calibration captured\"}";
  }
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleTemperatureCaptureHigh()
{
  Serial.println("[HTTP] POST /temperature/capture-high");

  if (context.captureHighCalibration == nullptr)
  {
    sendError(500, "Temperature high capture is unavailable");
    return;
  }

  String tempText = gServer.arg("temp_value");
  if (tempText.length() == 0)
  {
    tempText = gServer.arg("temp_c");
  }
  if (tempText.length() == 0)
  {
    sendError(400, "Missing temp_value");
    return;
  }

  const String unitText = gServer.arg("temp_unit");
  float knownTempC = NAN;
  String error;
  if (!parseCalibrationTemperatureC(tempText, unitText, knownTempC, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Invalid calibration temperature");
    return;
  }

  if (!context.captureHighCalibration(knownTempC, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Failed to capture high calibration");
    return;
  }

  String json = buildTemperatureJson(context);
  if (json.endsWith("}"))
  {
    json.remove(json.length() - 1);
    json += ",\"message\":\"High calibration captured\"}";
  }
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleTemperatureTrimOffset()
{
  Serial.println("[HTTP] POST /temperature/trim");

  if (context.setTemperatureTrimOffsetC == nullptr)
  {
    sendError(500, "Temperature trim adjustment is unavailable");
    return;
  }

  const String offsetText = gServer.arg("offset_c");
  if (offsetText.length() == 0)
  {
    sendError(400, "Missing offset_c");
    return;
  }

  String error;
  if (!context.setTemperatureTrimOffsetC(offsetText.toFloat(), error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Failed to set temperature trim offset");
    return;
  }

  String json = buildTemperatureJson(context);
  if (json.endsWith("}"))
  {
    json.remove(json.length() - 1);
    json += ",\"message\":\"Temperature trim offset updated\"}";
  }
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleTemperatureCalibrationReset()
{
  Serial.println("[HTTP] POST /temperature/calibration/reset");

  if (context.resetTemperatureCalibration == nullptr)
  {
    sendError(500, "Temperature calibration reset is unavailable");
    return;
  }

  String error;
  if (!context.resetTemperatureCalibration(error))
  {
    sendError(500, error.length() > 0 ? error.c_str() : "Failed to reset calibration");
    return;
  }

  String json = buildTemperatureJson(context);
  if (json.endsWith("}"))
  {
    json.remove(json.length() - 1);
    json += ",\"message\":\"Calibration reset\"}";
  }
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleWifiScan()
{
  Serial.println("[HTTP] GET /wifi/scan");

  if (context.wifi == nullptr)
  {
    sendError(500, "Wi-Fi manager is not available");
    return;
  }

  gServer.send(200, "application/json", context.wifi->scanWifiJson());
}

void WebControlServer::handleSetHostname()
{
  Serial.println("[HTTP] POST /hostname");

  if (context.setHostname == nullptr)
  {
    sendError(500, "Hostname setter is not available");
    return;
  }

  const String requested = gServer.arg("name");
  if (requested.length() == 0)
  {
    sendError(400, "Missing name parameter");
    return;
  }

  String error;
  if (!context.setHostname(requested, error))
  {
    sendError(400, error.length() > 0 ? error.c_str() : "Invalid hostname");
    return;
  }

  String json = "{\"ok\":true,\"command\":\"hostname\",\"hostname\":\"";
  json += context.getHostname != nullptr ? context.getHostname() : String("unknown");
  json += "\",\"mqtt_client_id\":\"";
  json += context.getMqttClientId != nullptr ? context.getMqttClientId() : String("unknown");
  json += "\"}";
  gServer.send(200, "application/json", json);
}

void WebControlServer::handleNotFound()
{
  Serial.print("[HTTP] ");
  Serial.print(gServer.method() == HTTP_POST ? "POST " : "GET ");
  Serial.println(gServer.uri());
  sendError(404, "Not found");
}