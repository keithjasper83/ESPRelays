/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <esp_now.h>

struct EspNowDiscoveryBridgeConfig
{
    uint32_t advertiseIntervalMs = 7000;
    String (*deviceIdProvider)();
    String (*deviceNameProvider)();
    String (*hostnameProvider)();
    String (*firmwareVersionProvider)();
    bool (*relayOnProvider)();
    bool (*wifiConnectedProvider)();
    int (*wifiRssiProvider)();
    void (*peerPayloadReceived)(const String &payload) = nullptr;
};

class EspNowDiscoveryBridge
{
public:
    void begin(const EspNowDiscoveryBridgeConfig &config);
    void loop();
    void advertiseNow();
    bool ready() const;

private:
    static void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len);
    static void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status);
    bool initTransport();
    String buildPayload() const;
    bool sendPayload(const String &payload);

    EspNowDiscoveryBridgeConfig config = {};
    bool configured = false;
    bool transportReady = false;
    bool pendingAdvertise = false;
    unsigned long lastAdvertiseMs = 0;
    uint16_t sequence = 0;

    static EspNowDiscoveryBridge *instance;
};
