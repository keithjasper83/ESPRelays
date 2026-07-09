/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

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