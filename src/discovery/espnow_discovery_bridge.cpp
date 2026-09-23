/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "discovery/espnow_discovery_bridge.h"

#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

namespace
{
    constexpr uint8_t FRAME_MAGIC_0 = 'K';
    constexpr uint8_t FRAME_MAGIC_1 = 'J';
    constexpr uint8_t FRAME_VERSION = 1;
    constexpr uint8_t FRAME_TYPE_DISCOVERY = 1;
    constexpr size_t FRAME_HEADER_SIZE = 8;
    constexpr size_t FRAME_MAX_PAYLOAD = ESP_NOW_MAX_DATA_LEN - FRAME_HEADER_SIZE;

    struct EspNowFrame
    {
        uint8_t magic0;
        uint8_t magic1;
        uint8_t version;
        uint8_t type;
        uint16_t sequence;
        uint16_t payloadLength;
        uint8_t payload[FRAME_MAX_PAYLOAD];
    };

    uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
}

EspNowDiscoveryBridge *EspNowDiscoveryBridge::instance = nullptr;

void EspNowDiscoveryBridge::begin(const EspNowDiscoveryBridgeConfig &newConfig)
{
    config = newConfig;
    configured = true;
    pendingAdvertise = true;
    lastAdvertiseMs = 0;
    sequence = 0;

    if (instance == nullptr)
    {
        instance = this;
    }

    transportReady = initTransport();
    if (!transportReady)
    {
        Serial.println("[ESPNOW] init failed; bridge disabled");
    }
}

bool EspNowDiscoveryBridge::initTransport()
{
    if (esp_now_init() != ESP_OK)
    {
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(BROADCAST_MAC))
    {
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            return false;
        }
    }

    Serial.println("[ESPNOW] discovery bridge ready");
    return true;
}

void EspNowDiscoveryBridge::loop()
{
    if (!configured || !transportReady)
    {
        return;
    }

    const unsigned long now = millis();
    const bool intervalDue = (now - lastAdvertiseMs) >= config.advertiseIntervalMs;
    if (!pendingAdvertise && !intervalDue)
    {
        return;
    }

    const String payload = buildPayload();
    const bool sent = sendPayload(payload);
    if (sent)
    {
        pendingAdvertise = false;
    }

    // Backoff on failure too, otherwise we'd spam each loop iteration.
    lastAdvertiseMs = now;
}

void EspNowDiscoveryBridge::advertiseNow()
{
    pendingAdvertise = true;
}

bool EspNowDiscoveryBridge::ready() const
{
    return transportReady;
}

String EspNowDiscoveryBridge::buildPayload() const
{
    const String deviceId = config.deviceIdProvider != nullptr ? config.deviceIdProvider() : String("esp32-unknown");
    const String deviceName = config.deviceNameProvider != nullptr ? config.deviceNameProvider() : String("ESP Device");
    const String hostname = config.hostnameProvider != nullptr ? config.hostnameProvider() : String("esp32");
    const String firmwareVersion = config.firmwareVersionProvider != nullptr ? config.firmwareVersionProvider() : String("0.0.0");
    const bool relayOn = config.relayOnProvider != nullptr ? config.relayOnProvider() : false;
    const bool wifiConnected = config.wifiConnectedProvider != nullptr ? config.wifiConnectedProvider() : false;
    const int rssi = config.wifiRssiProvider != nullptr ? config.wifiRssiProvider() : 0;

    JsonDocument document;
    // Compact keys keep ESP-NOW frames under 250-byte limits.
    document["p"] = "kj-esp-discovery";
    document["v"] = 1;
    document["t"] = "espnow";
    document["id"] = deviceId;
    document["dn"] = deviceName;
    document["h"] = hostname;
    document["fv"] = firmwareVersion;
    document["ro"] = relayOn;
    document["wc"] = wifiConnected;
    document["r"] = rssi;
    document["u"] = millis();

    String payload;
    serializeJson(document, payload);
    return payload;
}

bool EspNowDiscoveryBridge::sendPayload(const String &payload)
{
    if (payload.length() > FRAME_MAX_PAYLOAD)
    {
        Serial.print("[ESPNOW] payload too large; skipped bytes=");
        Serial.println(payload.length());
        return false;
    }

    EspNowFrame frame = {};
    frame.magic0 = FRAME_MAGIC_0;
    frame.magic1 = FRAME_MAGIC_1;
    frame.version = FRAME_VERSION;
    frame.type = FRAME_TYPE_DISCOVERY;
    frame.sequence = sequence++;
    frame.payloadLength = static_cast<uint16_t>(payload.length());
    memcpy(frame.payload, payload.c_str(), payload.length());

    const size_t txSize = FRAME_HEADER_SIZE + frame.payloadLength;
    const esp_err_t err = esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&frame), txSize);
    if (err != ESP_OK)
    {
        Serial.println("[ESPNOW] send failed");
        return false;
    }

    return true;
}

void EspNowDiscoveryBridge::onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len)
{
    (void)macAddr;

    if (instance == nullptr || data == nullptr || len < static_cast<int>(FRAME_HEADER_SIZE))
    {
        return;
    }

    const EspNowFrame *frame = reinterpret_cast<const EspNowFrame *>(data);
    if (frame->magic0 != FRAME_MAGIC_0 || frame->magic1 != FRAME_MAGIC_1 ||
        frame->version != FRAME_VERSION || frame->type != FRAME_TYPE_DISCOVERY)
    {
        return;
    }

    if (frame->payloadLength == 0 || frame->payloadLength > FRAME_MAX_PAYLOAD)
    {
        return;
    }

    const size_t expectedLen = FRAME_HEADER_SIZE + frame->payloadLength;
    if (static_cast<size_t>(len) < expectedLen)
    {
        return;
    }

    String payload;
    payload.reserve(frame->payloadLength + 1);
    payload.concat(reinterpret_cast<const char *>(frame->payload), frame->payloadLength);

    if (instance->config.peerPayloadReceived != nullptr)
    {
        instance->config.peerPayloadReceived(payload);
    }
}

void EspNowDiscoveryBridge::onDataSent(const uint8_t *macAddr, esp_now_send_status_t status)
{
    (void)macAddr;
    (void)status;
}
