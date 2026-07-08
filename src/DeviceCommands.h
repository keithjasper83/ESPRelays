#pragma once

#include <Arduino.h>

class CommandRouter;
class RelayController;
class WiFiManager;

struct DeviceCommandContext
{
    RelayController *relay = nullptr;
    WiFiManager *wifi = nullptr;
    void (*printStatus)() = nullptr;
};

namespace DeviceCommands
{
    void begin(CommandRouter &router, DeviceCommandContext &context);
}