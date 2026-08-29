#include <unity.h>

#include "ProbePresenceFilter.h"
#include "ProbePresenceFilter.cpp"

void test_disabled_monitoring_clears_presence()
{
    ProbePresenceFilter filter(5, 5, 10, 4000);
    for (int index = 0; index < 5; ++index) filter.update(1800, true);
    TEST_ASSERT_TRUE(filter.present());
    TEST_ASSERT_FALSE(filter.update(1800, false));
    TEST_ASSERT_EQUAL_INT(-1, filter.stableRaw());
}

void test_floating_in_range_spike_does_not_become_present()
{
    ProbePresenceFilter filter(5, 5, 10, 4000);
    TEST_ASSERT_FALSE(filter.update(1800, true));
    TEST_ASSERT_FALSE(filter.update(4095, true));
    TEST_ASSERT_FALSE(filter.update(1800, true));
    TEST_ASSERT_EQUAL_INT(-1, filter.stableRaw());
}

void test_requires_five_consecutive_samples_to_become_present()
{
    ProbePresenceFilter filter(5, 5, 10, 4000);
    for (int index = 0; index < 4; ++index)
        TEST_ASSERT_FALSE(filter.update(1800, true));
    TEST_ASSERT_TRUE(filter.update(1800, true));
    TEST_ASSERT_EQUAL_INT(1800, filter.stableRaw());
}

void test_isolated_out_of_range_sample_does_not_become_absent()
{
    ProbePresenceFilter filter(5, 5, 10, 4000);
    for (int index = 0; index < 5; ++index) filter.update(1800, true);
    TEST_ASSERT_TRUE(filter.update(4095, true));
    TEST_ASSERT_EQUAL_INT(1800, filter.stableRaw());
}

void test_requires_five_consecutive_samples_to_become_absent()
{
    ProbePresenceFilter filter(5, 5, 10, 4000);
    for (int index = 0; index < 5; ++index) filter.update(1800, true);
    for (int index = 0; index < 4; ++index)
        TEST_ASSERT_TRUE(filter.update(4095, true));
    TEST_ASSERT_FALSE(filter.update(4095, true));
    TEST_ASSERT_EQUAL_INT(-1, filter.stableRaw());
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_disabled_monitoring_clears_presence);
    RUN_TEST(test_floating_in_range_spike_does_not_become_present);
    RUN_TEST(test_requires_five_consecutive_samples_to_become_present);
    RUN_TEST(test_isolated_out_of_range_sample_does_not_become_absent);
    RUN_TEST(test_requires_five_consecutive_samples_to_become_absent);
    return UNITY_END();
}
