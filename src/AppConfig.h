#pragma once

#include <Arduino.h>

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

constexpr char WIFI_SSID[] = "SKYIL37L";
constexpr char WIFI_PASS[] = "K6B5IwYvREypf3";

constexpr char DEVICE_HOSTNAME_DEFAULT[] = "homerelay";
constexpr char DEVICE_NAME_DEFAULT[] = "Home Relay";
constexpr char DEVICE_TYPE_DEFAULT[] = "relay";
constexpr char FIRMWARE_NAME[] = "esp-relay-controller";
constexpr char FIRMWARE_VERSION[] = "1.0.0";

constexpr char MQTT_HOST[] = "192.168.0.10";
constexpr int MQTT_PORT = 1883;

constexpr char TOPIC_CMD[] = "home/relay/command";
constexpr char TOPIC_STATE[] = "home/relay/state";
constexpr char TOPIC_AVAIL[] = "home/relay/availability";