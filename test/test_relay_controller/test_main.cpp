#include <algorithm>
#include <string>
#include <vector>

#include <unity.h>

#include <Preferences.h>

#include "AppConfig.h"
#include "RelayController.h"

#include "../../src/RelayController.cpp"

namespace
{
    constexpr char RELAY_STATE[] = "state";
    constexpr char AUTO_OFF_MINUTES[] = "auto_off_min";
    constexpr char AUTO_OFF_ARMED[] = "auto_off_armed";
    constexpr char AUTO_OFF_EPOCH[] = "auto_off_epoch";
    constexpr char AUTO_OFF_REMAINING[] = "auto_off_rem";

    void seedPersistedOnWithArmedTimer()
    {
        Preferences preferences;
        TEST_ASSERT_TRUE(preferences.begin("relay_cfg", false));
        TEST_ASSERT_EQUAL_UINT(sizeof(bool), preferences.putBool(RELAY_STATE, true));
        TEST_ASSERT_EQUAL_UINT(sizeof(uint16_t), preferences.putUShort(AUTO_OFF_MINUTES, 10));
        TEST_ASSERT_EQUAL_UINT(sizeof(bool), preferences.putBool(AUTO_OFF_ARMED, true));
        TEST_ASSERT_EQUAL_UINT(sizeof(uint32_t), preferences.putULong(AUTO_OFF_EPOCH, 0));
        TEST_ASSERT_EQUAL_UINT(sizeof(uint32_t), preferences.putULong(AUTO_OFF_REMAINING, 600000));
        preferences.end();
        FakeRuntime::resetObservations();
    }

    size_t eventIndex(const std::string &event)
    {
        const auto found = std::find(FakeRuntime::events.begin(), FakeRuntime::events.end(), event);
        TEST_ASSERT_NOT_EQUAL(FakeRuntime::events.end(), found);
        return static_cast<size_t>(found - FakeRuntime::events.begin());
    }

    void assertWriteBeforeOutput(const std::string &event, size_t outputIndex)
    {
        TEST_ASSERT_LESS_THAN_UINT(outputIndex, eventIndex(event));
    }
}

void setUp()
{
    Preferences::reset();
    FakeRuntime::resetObservations();
}

void tearDown()
{
}

void test_brownout_begin_persists_relay_and_timer_off_before_gpio_apply()
{
    seedPersistedOnWithArmedTimer();
    RelayController relay;

    relay.begin(true);

    TEST_ASSERT_FALSE(relay.isOn());
    TEST_ASSERT_TRUE(relay.bootFailsafeApplied());
    TEST_ASSERT_FALSE(relay.autoOffArmed());
    TEST_ASSERT_EQUAL_INT(0, relay.autoOffRemainingSeconds());
    TEST_ASSERT_FALSE(Preferences::storedBool(RELAY_STATE, true));
    TEST_ASSERT_EQUAL_UINT16(10, Preferences::storedUShort(AUTO_OFF_MINUTES, 0));
    TEST_ASSERT_FALSE(Preferences::storedBool(AUTO_OFF_ARMED, true));
    TEST_ASSERT_EQUAL_UINT32(0, Preferences::storedULong(AUTO_OFF_EPOCH, 1));
    TEST_ASSERT_EQUAL_UINT32(0, Preferences::storedULong(AUTO_OFF_REMAINING, 1));
    TEST_ASSERT_EQUAL_INT(OUTPUT, FakeRuntime::pinModes.at(RELAY_PIN));
    TEST_ASSERT_EQUAL_INT(LOW, FakeRuntime::pinLevels.at(RELAY_PIN));

    const size_t outputIndex = eventIndex(
        "gpio.digitalWrite:" + std::to_string(RELAY_PIN) + ":" + std::to_string(LOW));
    assertWriteBeforeOutput("preferences.putBool:state", outputIndex);
    assertWriteBeforeOutput("preferences.putUShort:auto_off_min", outputIndex);
    assertWriteBeforeOutput("preferences.putBool:auto_off_armed", outputIndex);
    assertWriteBeforeOutput("preferences.putULong:auto_off_epoch", outputIndex);
    assertWriteBeforeOutput("preferences.putULong:auto_off_rem", outputIndex);
}

void test_normal_begin_preserves_persisted_on_and_armed_timer_restoration()
{
    seedPersistedOnWithArmedTimer();
    RelayController relay;

    relay.begin(false);

    TEST_ASSERT_TRUE(relay.isOn());
    TEST_ASSERT_FALSE(relay.bootFailsafeApplied());
    TEST_ASSERT_TRUE(relay.autoOffArmed());
    TEST_ASSERT_GREATER_THAN_INT(0, relay.autoOffRemainingSeconds());
    TEST_ASSERT_TRUE(Preferences::storedBool(RELAY_STATE, false));
    TEST_ASSERT_EQUAL_UINT16(10, Preferences::storedUShort(AUTO_OFF_MINUTES, 0));
    TEST_ASSERT_TRUE(Preferences::storedBool(AUTO_OFF_ARMED, false));
    TEST_ASSERT_EQUAL_UINT32(0, Preferences::storedULong(AUTO_OFF_EPOCH, 1));
    TEST_ASSERT_EQUAL_UINT32(600000, Preferences::storedULong(AUTO_OFF_REMAINING, 0));
    TEST_ASSERT_EQUAL_INT(OUTPUT, FakeRuntime::pinModes.at(RELAY_PIN));
    TEST_ASSERT_EQUAL_INT(HIGH, FakeRuntime::pinLevels.at(RELAY_PIN));
    TEST_ASSERT_EQUAL_UINT(0, FakeRuntime::preferenceWriteCount());
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_brownout_begin_persists_relay_and_timer_off_before_gpio_apply);
    RUN_TEST(test_normal_begin_preserves_persisted_on_and_armed_timer_restoration);
    return UNITY_END();
}
