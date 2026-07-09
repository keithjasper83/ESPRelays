/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "discovery/udp_discovery.h"

#include <WiFi.h>
#include <WiFiUdp.h>

namespace
{
    WiFiUDP gDiscoveryUdp;
}

void UdpDiscovery::begin(const DiscoveryConfig &newConfig)
{
    config = newConfig;
    configured = true;
    pendingAdvertise = true;
    transportReady = false;
    lastAdvertiseMs = 0;
}

void UdpDiscovery::loop(bool wifiConnected)
{
    if (!configured)
    {
        return;
    }

    if (!wifiConnected)
    {
        if (transportReady)
        {
            gDiscoveryUdp.stop();
            transportReady = false;
        }

        lastWifiConnected = false;
        return;
    }

    if (!lastWifiConnected)
    {
        if (!transportReady)
        {
            IPAddress multicastAddress;
            if (!multicastAddress.fromString(config.multicastGroup))
            {
                multicastAddress = IPAddress(239, 255, 42, 99);
            }

            transportReady = gDiscoveryUdp.beginMulticast(multicastAddress, config.port) == 1;
        }

        pendingAdvertise = true;
    }

    lastWifiConnected = true;

    if (!transportReady)
    {
        return;
    }

    const unsigned long now = millis();
    const bool intervalDue = (now - lastAdvertiseMs) >= config.advertiseIntervalMs;

    if (!pendingAdvertise && !intervalDue)
    {
        return;
    }

    String payload = buildPayload();
    if (sendPayload(payload))
    {
        pendingAdvertise = false;
        lastAdvertiseMs = now;
    }
}

void UdpDiscovery::advertiseNow()
{
    pendingAdvertise = true;
}

String UdpDiscovery::jsonEscape(const String &input)
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

String UdpDiscovery::buildDeviceId() const
{
    String mac = WiFi.macAddress();
    mac.toLowerCase();
    mac.replace(":", "");

    String host = config.hostnameProvider != nullptr ? config.hostnameProvider() : String("esp32");
    host.toLowerCase();

    String deviceId = "esp32-";
    deviceId += host;
    deviceId += "-";
    deviceId += mac;
    return deviceId;
}

String UdpDiscovery::buildPayload() const
{
    const String deviceName = config.deviceNameProvider != nullptr ? config.deviceNameProvider() : String("ESP Device");
    const String deviceType = config.deviceTypeProvider != nullptr ? config.deviceTypeProvider() : String("device");
    const String firmwareName = config.firmwareNameProvider != nullptr ? config.firmwareNameProvider() : String("esp-firmware");
    const String firmwareVersion = config.firmwareVersionProvider != nullptr ? config.firmwareVersionProvider() : String("0.0.0");
    const String stateJson = config.stateJsonProvider != nullptr ? config.stateJsonProvider() : String("{}");
    const String capabilitiesJson = config.capabilitiesJsonProvider != nullptr ? config.capabilitiesJsonProvider() : String("[]");
    const String model = config.modelProvider != nullptr ? config.modelProvider() : String("");

    String endpointJson = "{";
    for (size_t i = 0; i < config.endpointCount; i++)
    {
        const DiscoveryEndpoint &ep = config.endpoints[i];
        if (i > 0)
        {
            endpointJson += ",";
        }

        endpointJson += "\"" + jsonEscape(String(ep.name)) + "\":{";
        endpointJson += "\"method\":\"" + jsonEscape(String(ep.method)) + "\",";
        endpointJson += "\"path\":\"" + jsonEscape(String(ep.path)) + "\"}";
    }
    endpointJson += "}";

    String ipText = WiFi.localIP().toString();
    String baseUrl = "http://" + ipText + "/";

    String payload = "{";
    payload += "\"protocol\":\"" + jsonEscape(String(config.protocol)) + "\",";
    payload += "\"protocol_version\":" + String(config.protocolVersion) + ",";
    payload += "\"device_id\":\"" + jsonEscape(buildDeviceId()) + "\",";
    payload += "\"device_name\":\"" + jsonEscape(deviceName) + "\",";
    payload += "\"device_type\":\"" + jsonEscape(deviceType) + "\",";
    payload += "\"firmware\":{";
    payload += "\"name\":\"" + jsonEscape(firmwareName) + "\",";
    payload += "\"version\":\"" + jsonEscape(firmwareVersion) + "\"},";
    payload += "\"network\":{";
    payload += "\"ip\":\"" + jsonEscape(ipText) + "\",";
    payload += "\"http_port\":" + String(config.httpPort) + ",";
    payload += "\"base_url\":\"" + jsonEscape(baseUrl) + "\"},";
    payload += "\"capabilities\":" + capabilitiesJson + ",";
    payload += "\"endpoints\":" + endpointJson + ",";
    payload += "\"state\":" + stateJson + ",";
    payload += "\"uptime_ms\":" + String(millis()) + ",";
    payload += "\"rssi\":" + String(WiFi.RSSI());

    if (model.length() > 0)
    {
        payload += ",\"model\":\"" + jsonEscape(model) + "\"";
    }

    payload += "}";
    return payload;
}

bool UdpDiscovery::sendPayload(const String &payload)
{
    IPAddress multicastAddress;
    if (!multicastAddress.fromString(config.multicastGroup))
    {
        return false;
    }

    if (!gDiscoveryUdp.beginPacket(multicastAddress, config.port))
    {
        return false;
    }

    const size_t written = gDiscoveryUdp.write(reinterpret_cast<const uint8_t *>(payload.c_str()), payload.length());
    const bool ok = written == payload.length() && gDiscoveryUdp.endPacket() == 1;

    if (ok)
    {
        float chipTempC = temperatureRead();

        Serial.print(chipTempC, 1);
        Serial.print(" - [DISCOVERY] Advertised to ");
        Serial.print(config.multicastGroup);
        Serial.print(":");
        Serial.print(config.port);
        Serial.print(" | bytes=");
        Serial.println(payload.length());
    }
    else
    {
        Serial.println("[DISCOVERY] Advertisement send failed");
    }

    return ok;
}