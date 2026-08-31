/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include <unity.h>

#include "ResetDiagnostics.h"
#include "ResetDiagnostics.cpp"

namespace
{
    struct ResetReasonExpectation
    {
        DeviceResetReason reason;
        const char *name;
        bool forcesOff;
    };

    constexpr ResetReasonExpectation resetReasons[] = {
        {DeviceResetReason::PowerOn, "power_on", false},
        {DeviceResetReason::Software, "software", false},
        {DeviceResetReason::External, "external", false},
        {DeviceResetReason::Watchdog, "watchdog", false},
        {DeviceResetReason::Panic, "panic", false},
        {DeviceResetReason::DeepSleep, "deep_sleep", false},
        {DeviceResetReason::Brownout, "brownout", true},
        {DeviceResetReason::Unknown, "unknown", false},
    };

    void assertBootDecision(const bool persistedOn, const ResetReasonExpectation &expectation)
    {
        const RelayBootDecision decision = relayBootDecision(persistedOn, expectation.reason);
        TEST_ASSERT_EQUAL(expectation.forcesOff ? false : persistedOn, decision.relayOn);
        TEST_ASSERT_EQUAL(expectation.forcesOff && persistedOn, decision.overrideApplied);
    }
}

void test_every_reset_reason_has_a_stable_server_name()
{
    for (const ResetReasonExpectation &expectation : resetReasons)
    {
        TEST_ASSERT_EQUAL_STRING(expectation.name, deviceResetReasonName(expectation.reason));
    }
}

void test_only_confirmed_brownout_forces_relay_off()
{
    TEST_ASSERT_TRUE(shouldForceRelayOff(DeviceResetReason::Brownout));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::PowerOn));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::Software));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::External));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::Watchdog));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::Panic));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::DeepSleep));
    TEST_ASSERT_FALSE(shouldForceRelayOff(DeviceResetReason::Unknown));
}

void test_boot_decision_preserves_persisted_off_for_every_reset_reason()
{
    for (const ResetReasonExpectation &expectation : resetReasons)
    {
        assertBootDecision(false, expectation);
    }
}

void test_boot_decision_only_overrides_persisted_on_after_confirmed_brownout()
{
    for (const ResetReasonExpectation &expectation : resetReasons)
    {
        assertBootDecision(true, expectation);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_every_reset_reason_has_a_stable_server_name);
    RUN_TEST(test_only_confirmed_brownout_forces_relay_off);
    RUN_TEST(test_boot_decision_preserves_persisted_off_for_every_reset_reason);
    RUN_TEST(test_boot_decision_only_overrides_persisted_on_after_confirmed_brownout);
    return UNITY_END();
}
