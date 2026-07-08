#pragma once

#include <Arduino.h>

struct DiscoveryEndpoint
{
    const char *name;
    const char *method;
    const char *path;
};

struct DiscoveryConfig
{
    const char *multicastGroup;
    uint16_t port;
    uint32_t advertiseIntervalMs;
    const char *protocol;
    int protocolVersion;
    uint16_t httpPort;

    String (*deviceNameProvider)();
    String (*deviceTypeProvider)();
    String (*firmwareNameProvider)();
    String (*firmwareVersionProvider)();
    String (*hostnameProvider)();
    String (*stateJsonProvider)();
    String (*capabilitiesJsonProvider)();
    String (*modelProvider)();

    const DiscoveryEndpoint *endpoints;
    size_t endpointCount;
};

class UdpDiscovery
{
public:
    void begin(const DiscoveryConfig &config);
    void loop(bool wifiConnected);
    void advertiseNow();

private:
    String buildPayload() const;
    String buildDeviceId() const;
    bool sendPayload(const String &payload);
    static String jsonEscape(const String &input);

    DiscoveryConfig config = {};
    bool configured = false;
    bool transportReady = false;
    bool lastWifiConnected = false;
    bool pendingAdvertise = false;
    unsigned long lastAdvertiseMs = 0;
};