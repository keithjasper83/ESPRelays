#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <unity.h>

#include <Preferences.h>

#include "CalibrationRecordStorage.h"
#include "TemperatureCalibrationRecord.h"

#define private public
#include "TemperatureProbeManager.h"
#undef private

#include "../../src/TemperatureCalibrationRecord.cpp"
#include "../../src/CalibrationRecordStorage.cpp"
#include "../../src/ProbePresenceFilter.cpp"
#include "../../src/TemperatureProbeManager.cpp"

class FakeCalibrationStorage final : public CalibrationBlobStorage
{
public:
    size_t length(const char *key) override
    {
        const auto found = records.find(key);
        return found == records.end() ? 0 : found->second.size();
    }

    size_t read(const char *key, void *value, size_t valueLength) override
    {
        const auto found = records.find(key);
        if (found == records.end()) return 0;
        const size_t copied = valueLength < found->second.size() ? valueLength : found->second.size();
        memcpy(value, found->second.data(), copied);
        return copied;
    }

    size_t write(const char *key, const void *value, size_t valueLength) override
    {
        if (shortWrite) return valueLength - 1;
        const auto *bytes = static_cast<const uint8_t *>(value);
        records[key] = std::vector<uint8_t>(bytes, bytes + valueLength);
        if (corruptAfterWrite) records[key][0] ^= 0x01U;
        return valueLength;
    }

    bool corruptAfterWrite = false;
    bool shortWrite = false;

private:
    std::map<std::string, std::vector<uint8_t>> records;
};

namespace
{
    TemperatureCalibrationRecord validRecord(uint8_t generation)
    {
        return makeTemperatureCalibrationRecord(true, 555, -17.0f,
                                                true, 3054, 42.5f, 0.0f,
                                                true, generation);
    }

    TemperatureCalibrationRecord managerRecord()
    {
        return makeTemperatureCalibrationRecord(true, 500, -10.0f,
                                                true, 3000, 50.0f, 1.0f,
                                                true, 10);
    }

    TemperatureProbeManager managerWithPersistedRecord()
    {
        Preferences::reset();
        Preferences preferences;
        TEST_ASSERT_TRUE(preferences.begin("temp_probe", false));
        const TemperatureCalibrationRecord record = managerRecord();
        TEST_ASSERT_EQUAL_UINT(sizeof(record), preferences.putBytes("cal_a", &record, sizeof(record)));
        preferences.end();

        TemperatureProbeManager manager;
        manager.loadCalibration();
        return manager;
    }

    void rejectNextManagerWrite()
    {
        Preferences::corruptByteWrites = true;
    }

    void assertManagerStorageFailure(const TemperatureProbeManager &manager)
    {
        TEST_ASSERT_FALSE(manager.lastStorageWriteVerified());
        TEST_ASSERT_EQUAL_UINT8(10, manager.calibrationGenerationValue());
    }
}

void test_verified_write_accepts_valid_byte_for_byte_readback()
{
    FakeCalibrationStorage storage;
    TEST_ASSERT_TRUE(writeVerifiedCalibration(storage, "cal_a", validRecord(7)));
}

void test_verified_write_rejects_short_write()
{
    FakeCalibrationStorage storage;
    storage.shortWrite = true;
    TEST_ASSERT_FALSE(writeVerifiedCalibration(storage, "cal_a", validRecord(7)));
}

void test_verified_write_rejects_corrupt_readback()
{
    FakeCalibrationStorage storage;
    storage.corruptAfterWrite = true;
    TEST_ASSERT_FALSE(writeVerifiedCalibration(storage, "cal_a", validRecord(7)));
}

void test_select_newest_falls_back_from_corrupt_newer_slot()
{
    FakeCalibrationStorage storage;
    TEST_ASSERT_TRUE(writeVerifiedCalibration(storage, "cal_a", validRecord(7)));
    TEST_ASSERT_TRUE(writeVerifiedCalibration(storage, "cal_b", validRecord(8)));
    storage.corruptAfterWrite = true;
    TEST_ASSERT_FALSE(writeVerifiedCalibration(storage, "cal_b", validRecord(9)));

    const SelectedCalibrationRecord selected = selectNewestCalibration(storage, "cal_a", "cal_b");
    TEST_ASSERT_TRUE(selected.found);
    TEST_ASSERT_EQUAL_STRING("cal_a", selected.key);
    TEST_ASSERT_EQUAL_UINT8(7, selected.record.reserved);
}

void test_select_newest_handles_generation_wrap_from_255_to_zero()
{
    FakeCalibrationStorage storage;
    TEST_ASSERT_TRUE(writeVerifiedCalibration(storage, "cal_a", validRecord(255)));
    TEST_ASSERT_TRUE(writeVerifiedCalibration(storage, "cal_b", validRecord(0)));

    const SelectedCalibrationRecord selected = selectNewestCalibration(storage, "cal_a", "cal_b");
    TEST_ASSERT_TRUE(selected.found);
    TEST_ASSERT_EQUAL_STRING("cal_b", selected.key);
    TEST_ASSERT_EQUAL_UINT8(0, selected.record.reserved);
}

void test_set_enabled_restores_enabled_state_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    for (int sample = 0; sample < 5; ++sample)
        manager.probePresenceFilter.update(700, true);
    manager.probePresent = manager.probePresenceFilter.present();
    manager.lastRawReading = 700;
    manager.savedCurrentTemperatureRaw = manager.probePresenceFilter.stableRaw();
    manager.lastSampleAtMs = 123;
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.setEnabled(false, error));
    TEST_ASSERT_TRUE(manager.isEnabled());
    TEST_ASSERT_TRUE(manager.isPresent());
    TEST_ASSERT_EQUAL_INT(700, manager.rawReading());
    TEST_ASSERT_EQUAL_INT(700, manager.currentTemperatureRaw());
    TEST_ASSERT_EQUAL_UINT32(123, manager.lastSampleAtMs);
    assertManagerStorageFailure(manager);
}

void test_capture_low_restores_low_point_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    manager.probePresent = true;
    manager.savedCurrentTemperatureRaw = 600;
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.captureLow(5.0f, error));
    TEST_ASSERT_TRUE(manager.lowPointValid());
    TEST_ASSERT_EQUAL_INT(500, manager.lowPointRaw());
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, manager.lowPointTempC());
    assertManagerStorageFailure(manager);
}

void test_capture_high_restores_high_point_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    manager.probePresent = true;
    manager.savedCurrentTemperatureRaw = 3200;
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.captureHigh(60.0f, error));
    TEST_ASSERT_TRUE(manager.highPointValid());
    TEST_ASSERT_EQUAL_INT(3000, manager.highPointRaw());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, manager.highPointTempC());
    assertManagerStorageFailure(manager);
}

void test_reset_calibration_restores_both_points_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.resetCalibration(error));
    TEST_ASSERT_TRUE(manager.lowPointValid());
    TEST_ASSERT_TRUE(manager.highPointValid());
    TEST_ASSERT_EQUAL_INT(500, manager.lowPointRaw());
    TEST_ASSERT_EQUAL_INT(3000, manager.highPointRaw());
    assertManagerStorageFailure(manager);
}

void test_set_trim_offset_restores_trim_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.setTrimOffsetC(7.5f, error));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, manager.trimOffsetC());
    assertManagerStorageFailure(manager);
}

void test_restore_calibration_restores_all_values_after_failed_verification()
{
    TemperatureProbeManager manager = managerWithPersistedRecord();
    rejectNextManagerWrite();
    String error;

    TEST_ASSERT_FALSE(manager.restoreCalibration(true, 800, 0.0f,
                                                  true, 3500, 75.0f,
                                                  2.5f, false, error));
    TEST_ASSERT_TRUE(manager.isEnabled());
    TEST_ASSERT_TRUE(manager.lowPointValid());
    TEST_ASSERT_EQUAL_INT(500, manager.lowPointRaw());
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, manager.lowPointTempC());
    TEST_ASSERT_TRUE(manager.highPointValid());
    TEST_ASSERT_EQUAL_INT(3000, manager.highPointRaw());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, manager.highPointTempC());
    TEST_ASSERT_EQUAL_FLOAT(1.0f, manager.trimOffsetC());
    assertManagerStorageFailure(manager);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_verified_write_accepts_valid_byte_for_byte_readback);
    RUN_TEST(test_verified_write_rejects_short_write);
    RUN_TEST(test_verified_write_rejects_corrupt_readback);
    RUN_TEST(test_select_newest_falls_back_from_corrupt_newer_slot);
    RUN_TEST(test_select_newest_handles_generation_wrap_from_255_to_zero);
    RUN_TEST(test_set_enabled_restores_enabled_state_after_failed_verification);
    RUN_TEST(test_capture_low_restores_low_point_after_failed_verification);
    RUN_TEST(test_capture_high_restores_high_point_after_failed_verification);
    RUN_TEST(test_reset_calibration_restores_both_points_after_failed_verification);
    RUN_TEST(test_set_trim_offset_restores_trim_after_failed_verification);
    RUN_TEST(test_restore_calibration_restores_all_values_after_failed_verification);
    return UNITY_END();
}
