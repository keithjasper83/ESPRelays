/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

enum class DeviceResetReason
{
    PowerOn,
    Software,
    External,
    Watchdog,
    Panic,
    DeepSleep,
    Brownout,
    Unknown,
};

struct RelayBootDecision
{
    bool relayOn;
    bool overrideApplied;
};

const char *deviceResetReasonName(DeviceResetReason reason);
bool shouldForceRelayOff(DeviceResetReason reason);
RelayBootDecision relayBootDecision(bool persistedOn, DeviceResetReason reason);
