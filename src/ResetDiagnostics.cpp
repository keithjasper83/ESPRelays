/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "ResetDiagnostics.h"

const char *deviceResetReasonName(const DeviceResetReason reason)
{
    switch (reason)
    {
    case DeviceResetReason::PowerOn: return "power_on";
    case DeviceResetReason::Software: return "software";
    case DeviceResetReason::External: return "external";
    case DeviceResetReason::Watchdog: return "watchdog";
    case DeviceResetReason::Panic: return "panic";
    case DeviceResetReason::DeepSleep: return "deep_sleep";
    case DeviceResetReason::Brownout: return "brownout";
    case DeviceResetReason::Unknown: return "unknown";
    }

    return "unknown";
}

bool shouldForceRelayOff(const DeviceResetReason reason)
{
    return reason == DeviceResetReason::Brownout;
}

RelayBootDecision relayBootDecision(const bool persistedOn, const DeviceResetReason reason)
{
    const bool forceOff = shouldForceRelayOff(reason);
    return {
        .relayOn = forceOff ? false : persistedOn,
        .overrideApplied = forceOff && persistedOn,
    };
}
