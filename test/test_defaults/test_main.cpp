#include <cstring>
#include <unity.h>

#include "AppConfig.h"

void setUp()
{
}

void tearDown()
{
}

void test_defaults_strings_are_present()
{
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(DEVICE_HOSTNAME_DEFAULT));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(DEVICE_NAME_DEFAULT));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(DEVICE_TYPE_DEFAULT));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(FIRMWARE_NAME));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(FIRMWARE_VERSION));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(MQTT_HOST));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(TIME_SERVER_DEFAULT));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(TIME_SERVER_FALLBACK_1));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(TIME_SERVER_FALLBACK_2));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(TIME_TZ_DEFAULT));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(OTA_RELEASE_INFO_URL));
    TEST_ASSERT_GREATER_THAN_UINT32(0, std::strlen(OTA_FIRMWARE_ASSET_NAME));
}

void test_defaults_numeric_ranges_are_valid()
{
    TEST_ASSERT_GREATER_THAN(0, MQTT_PORT);
    TEST_ASSERT_LESS_OR_EQUAL(65535, MQTT_PORT);

    TEST_ASSERT_GREATER_THAN(0u, TIME_SYNC_INTERVAL_MS);
    TEST_ASSERT_GREATER_THAN(0u, TIME_SYNC_RETRY_MS);

    TEST_ASSERT_GREATER_THAN(0u, SCHEDULE_MAX_EVENTS);
    TEST_ASSERT_LESS_OR_EQUAL(10u, SCHEDULE_MAX_EVENTS);

    TEST_ASSERT_NOT_EQUAL(RELAY_PIN, RELAY_BUTTON_PIN);
    TEST_ASSERT_NOT_EQUAL(RELAY_PIN, RESET_BUTTON_PIN);
}

void test_mqtt_default_ip_is_expected()
{
    TEST_ASSERT_EQUAL_STRING("192.168.0.50", MQTT_HOST);
    TEST_ASSERT_EQUAL(1883, MQTT_PORT);
}

void test_ntp_default_and_fallback_chain_is_expected()
{
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", TIME_SERVER_DEFAULT);
    TEST_ASSERT_EQUAL_STRING("time.google.com", TIME_SERVER_FALLBACK_1);
    TEST_ASSERT_EQUAL_STRING("time.cloudflare.com", TIME_SERVER_FALLBACK_2);
}

void test_ota_asset_name_is_bin()
{
    const size_t length = std::strlen(OTA_FIRMWARE_ASSET_NAME);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(5, length);
    TEST_ASSERT_EQUAL_STRING(".bin", OTA_FIRMWARE_ASSET_NAME + (length - 4));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_defaults_strings_are_present);
    RUN_TEST(test_defaults_numeric_ranges_are_valid);
    RUN_TEST(test_mqtt_default_ip_is_expected);
    RUN_TEST(test_ntp_default_and_fallback_chain_is_expected);
    RUN_TEST(test_ota_asset_name_is_bin);
    return UNITY_END();
}
