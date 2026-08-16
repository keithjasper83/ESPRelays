#include <unity.h>

#include "TemperatureCalibrationRecord.h"
#include "TemperatureCalibrationRecord.cpp"

void test_valid_record_passes_crc_and_validation()
{
    TemperatureCalibrationRecord record;
    record.flags = TEMPERATURE_CALIBRATION_LOW_VALID |
                   TEMPERATURE_CALIBRATION_HIGH_VALID |
                   TEMPERATURE_MONITORING_ENABLED;
    record.lowRaw = 1000;
    record.lowTempC = 20.0f;
    record.highRaw = 3000;
    record.highTempC = 60.0f;
    record.trimOffsetC = 1.25f;
    record.crc32 = temperatureCalibrationCrc32(record);
    TEST_ASSERT_TRUE(temperatureCalibrationRecordValid(record));
}

void test_corrupt_record_is_rejected()
{
    TemperatureCalibrationRecord record;
    record.crc32 = temperatureCalibrationCrc32(record);
    record.lowRaw = 42;
    TEST_ASSERT_FALSE(temperatureCalibrationRecordValid(record));
}

void test_legacy_values_migrate_exactly_to_checksummed_record()
{
    const TemperatureCalibrationRecord record = makeTemperatureCalibrationRecord(
        true, 2872, 21.375f, true, 1130, 61.625f, -0.75f, false);
    TEST_ASSERT_TRUE(temperatureCalibrationRecordValid(record));
    TEST_ASSERT_EQUAL_INT32(2872, record.lowRaw);
    TEST_ASSERT_EQUAL_FLOAT(21.375f, record.lowTempC);
    TEST_ASSERT_EQUAL_INT32(1130, record.highRaw);
    TEST_ASSERT_EQUAL_FLOAT(61.625f, record.highTempC);
    TEST_ASSERT_EQUAL_FLOAT(-0.75f, record.trimOffsetC);
    TEST_ASSERT_FALSE(record.flags & TEMPERATURE_MONITORING_ENABLED);
}

void test_interrupted_blob_write_is_rejected_for_legacy_fallback()
{
    TemperatureCalibrationRecord record = makeTemperatureCalibrationRecord(
        true, 1000, 20.0f, true, 3000, 60.0f, 0.0f, true);
    reinterpret_cast<uint8_t *>(&record)[sizeof(record) / 2] ^= 0x40;
    TEST_ASSERT_FALSE(temperatureCalibrationRecordValid(record));
}

void test_inactive_legacy_fields_are_still_migrated_byte_for_value()
{
    const TemperatureCalibrationRecord record = makeTemperatureCalibrationRecord(
        false, 2872, 12.375f, false, 1999, 58.625f, 0.0f, true);
    TEST_ASSERT_TRUE(temperatureCalibrationRecordValid(record));
    TEST_ASSERT_EQUAL_INT32(2872, record.lowRaw);
    TEST_ASSERT_EQUAL_FLOAT(12.375f, record.lowTempC);
    TEST_ASSERT_EQUAL_INT32(1999, record.highRaw);
    TEST_ASSERT_EQUAL_FLOAT(58.625f, record.highTempC);
}

void test_generation_is_covered_by_checksum_for_dual_slot_selection()
{
    const TemperatureCalibrationRecord first = makeTemperatureCalibrationRecord(
        true, 2800, 20.0f, true, 1200, 60.0f, 0.0f, true, 41);
    TemperatureCalibrationRecord second = makeTemperatureCalibrationRecord(
        true, 2800, 20.0f, true, 1200, 60.0f, 0.0f, true, 42);
    TEST_ASSERT_EQUAL_UINT8(41, first.reserved);
    TEST_ASSERT_EQUAL_UINT8(42, second.reserved);
    TEST_ASSERT_TRUE(temperatureCalibrationRecordValid(first));
    second.reserved = 43;
    TEST_ASSERT_FALSE(temperatureCalibrationRecordValid(second));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_valid_record_passes_crc_and_validation);
    RUN_TEST(test_corrupt_record_is_rejected);
    RUN_TEST(test_legacy_values_migrate_exactly_to_checksummed_record);
    RUN_TEST(test_interrupted_blob_write_is_rejected_for_legacy_fallback);
    RUN_TEST(test_inactive_legacy_fields_are_still_migrated_byte_for_value);
    RUN_TEST(test_generation_is_covered_by_checksum_for_dual_slot_selection);
    return UNITY_END();
}
