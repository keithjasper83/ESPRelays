/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>

// Optional untracked local overrides for machine-specific defaults.
#if __has_include("AppConfig.local.h")
#include "AppConfig.local.h"
#endif

#ifndef WIFI_SSID_DEFAULT
#define WIFI_SSID_DEFAULT "CHANGE_ME_WIFI_SSID"
#endif

#ifndef WIFI_PASS_DEFAULT
#define WIFI_PASS_DEFAULT "CHANGE_ME_WIFI_PASS"
#endif

constexpr uint8_t DEBUG_JUMPER_PIN = 4;

constexpr uint8_t RELAY_PIN = 5;
constexpr bool RELAY_ACTIVE_LOW = false;

constexpr uint8_t RELAY_LED_PIN = 6;
constexpr uint8_t WIFI_LED_PIN = 7;
constexpr bool LED_ACTIVE_HIGH = true;
constexpr uint32_t WIFI_LED_BLINK_INTERVAL_MS = 5000;
constexpr uint32_t WIFI_LED_PULSE_MS = 150;

constexpr uint8_t RELAY_BUTTON_PIN = 3;
constexpr uint8_t RESET_BUTTON_PIN = 10;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;

constexpr char WIFI_SSID[] = WIFI_SSID_DEFAULT;
constexpr char WIFI_PASS[] = WIFI_PASS_DEFAULT;

constexpr char DEVICE_HOSTNAME_DEFAULT[] = "homerelay";
constexpr char DEVICE_NAME_DEFAULT[] = "Home Relay";
constexpr char DEVICE_TYPE_DEFAULT[] = "relay";
constexpr char FIRMWARE_NAME[] = "esp-relay-controller";
constexpr char FIRMWARE_VERSION[] = "2.0.1";

constexpr char MQTT_HOST[] = "192.168.0.50";
constexpr int MQTT_PORT = 1883;

constexpr char TIME_SERVER_DEFAULT[] = "pool.ntp.org";
constexpr char TIME_SERVER_FALLBACK_1[] = "time.google.com";
constexpr char TIME_SERVER_FALLBACK_2[] = "time.cloudflare.com";
constexpr char TIME_TZ_DEFAULT[] = "GMT0BST,M3.5.0/1,M10.5.0/2";
constexpr uint32_t TIME_SYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t TIME_SYNC_RETRY_MS = 60UL * 1000UL;

constexpr uint8_t SCHEDULE_MAX_EVENTS = 10;

constexpr char OTA_RELEASE_INFO_URL[] = "https://api.github.com/repos/keithjasper83/ESPRelays/releases/latest";
constexpr char OTA_FIRMWARE_ASSET_NAME[] = "firmware-esp32-c3-devkitm-1.bin";

constexpr char TOPIC_CMD[] = "home/relay/command";
constexpr char TOPIC_STATE[] = "home/relay/state";
constexpr char TOPIC_AVAIL[] = "home/relay/availability";