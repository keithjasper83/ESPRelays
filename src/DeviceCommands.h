#pragma once

#include <Arduino.h>

class CommandRouter;
class RelayController;
class WiFiManager;
class OtaUpdateManager;

struct DeviceCommandContext
{
    RelayController *relay = nullptr;
    WiFiManager *wifi = nullptr;
    OtaUpdateManager *ota = nullptr;
    void (*printStatus)() = nullptr;
};

namespace DeviceCommands
{
    void begin(CommandRouter &router, DeviceCommandContext &context);
}