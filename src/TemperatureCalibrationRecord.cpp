#include "TemperatureCalibrationRecord.h"

#include <math.h>

uint32_t temperatureCalibrationCrc32(const TemperatureCalibrationRecord &record)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&record);
    const size_t length = offsetof(TemperatureCalibrationRecord, crc32);
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t index = 0; index < length; ++index)
    {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

bool temperatureCalibrationPairValid(
    const int32_t lowRaw, const float lowTempC,
    const int32_t highRaw, const float highTempC)
{
    return lowRaw >= 0 && lowRaw <= 4095 &&
           highRaw >= 0 && highRaw <= 4095 &&
           lowRaw != highRaw &&
           isfinite(lowTempC) && isfinite(highTempC) &&
           lowTempC >= -100.0f && highTempC <= 200.0f &&
           lowTempC < highTempC;
}

bool temperatureCalibrationRecordValid(const TemperatureCalibrationRecord &record)
{
    if (record.magic != TEMPERATURE_CALIBRATION_MAGIC ||
        record.version != TEMPERATURE_CALIBRATION_VERSION ||
        record.crc32 != temperatureCalibrationCrc32(record) ||
        isnan(record.trimOffsetC) || record.trimOffsetC < -20.0f || record.trimOffsetC > 20.0f)
    {
        return false;
    }

    const bool lowValid = (record.flags & TEMPERATURE_CALIBRATION_LOW_VALID) != 0;
    const bool highValid = (record.flags & TEMPERATURE_CALIBRATION_HIGH_VALID) != 0;
    if (lowValid && (record.lowRaw < 0 || record.lowRaw > 4095 ||
                     !isfinite(record.lowTempC) || record.lowTempC < -100.0f || record.lowTempC > 200.0f))
    {
        return false;
    }
    if (highValid && (record.highRaw < 0 || record.highRaw > 4095 ||
                      !isfinite(record.highTempC) || record.highTempC < -100.0f || record.highTempC > 200.0f))
    {
        return false;
    }
    return !lowValid || !highValid || temperatureCalibrationPairValid(
        record.lowRaw, record.lowTempC, record.highRaw, record.highTempC);
}

TemperatureCalibrationRecord makeTemperatureCalibrationRecord(
    const bool lowValid, const int32_t lowRaw, const float lowTempC,
    const bool highValid, const int32_t highRaw, const float highTempC,
    const float trimOffsetC, const bool enabled, const uint8_t generation)
{
    TemperatureCalibrationRecord record;
    record.reserved = generation;
    record.flags = 0;
    if (lowValid) record.flags |= TEMPERATURE_CALIBRATION_LOW_VALID;
    if (highValid) record.flags |= TEMPERATURE_CALIBRATION_HIGH_VALID;
    if (enabled) record.flags |= TEMPERATURE_MONITORING_ENABLED;
    record.lowRaw = lowRaw;
    record.lowTempC = lowTempC;
    record.highRaw = highRaw;
    record.highTempC = highTempC;
    record.trimOffsetC = trimOffsetC;
    record.crc32 = temperatureCalibrationCrc32(record);
    return record;
}
