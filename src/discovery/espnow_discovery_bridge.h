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
    bool (*otaCheckHandler)(String &latestVersion, bool &updateAvailable, String &message) = nullptr;
    bool (*otaUpdateHandler)(String &message) = nullptr;
    void (*otaResultReceived)(const String &commandId, bool install, bool ok, bool updateAvailable,
                              const String &latestVersion, const String &message) = nullptr;
};

class EspNowDiscoveryBridge
{
public:
    void begin(const EspNowDiscoveryBridgeConfig &config);
    void loop();
    void advertiseNow();
    bool ready() const;
    bool sendOtaCommand(bool install, const String &targetDeviceId, String &commandId, String &error,
                        uint8_t maxHops = 2);

private:
    static void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len);
    static void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status);
    bool initTransport();
    String buildPayload() const;
    bool sendPayload(const String &payload);
    bool sendFrame(uint8_t type, const String &payload);
    void handleCommandFrame(const String &payload, uint8_t hopCount, uint8_t maxHops);
    void handleResultFrame(const String &payload);
    bool seenMessageId(const String &messageId);
    void recordMessageId(const String &messageId);

    static constexpr size_t SEEN_MESSAGE_CAPACITY = 8;
    String seenMessageIds[SEEN_MESSAGE_CAPACITY];
    uint8_t seenMessageWriteIndex = 0;

    EspNowDiscoveryBridgeConfig config = {};
    bool configured = false;
    bool transportReady = false;
    bool pendingAdvertise = false;
    unsigned long lastAdvertiseMs = 0;
    uint16_t sequence = 0;

    static EspNowDiscoveryBridge *instance;
};
