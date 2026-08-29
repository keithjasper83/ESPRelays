#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <unity.h>

#include "CalibrationRecordStorage.h"
#include "TemperatureCalibrationRecord.h"

#include "../../src/TemperatureCalibrationRecord.cpp"
#include "../../src/CalibrationRecordStorage.cpp"

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
    return UNITY_END();
}
