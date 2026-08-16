#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint32_t TEMPERATURE_CALIBRATION_MAGIC = 0x54434C32UL;
constexpr uint16_t TEMPERATURE_CALIBRATION_VERSION = 2;

enum TemperatureCalibrationFlags : uint8_t
{
    TEMPERATURE_CALIBRATION_LOW_VALID = 1U << 0,
    TEMPERATURE_CALIBRATION_HIGH_VALID = 1U << 1,
    TEMPERATURE_MONITORING_ENABLED = 1U << 2,
};

struct TemperatureCalibrationRecord
{
    uint32_t magic = TEMPERATURE_CALIBRATION_MAGIC;
    uint16_t version = TEMPERATURE_CALIBRATION_VERSION;
    uint8_t flags = TEMPERATURE_MONITORING_ENABLED;
    uint8_t reserved = 0;
    int32_t lowRaw = -1;
    float lowTempC = 0.0f;
    int32_t highRaw = -1;
    float highTempC = 0.0f;
    float trimOffsetC = 0.0f;
    uint32_t crc32 = 0;
};

uint32_t temperatureCalibrationCrc32(const TemperatureCalibrationRecord &record);
bool temperatureCalibrationPairValid(
    int32_t lowRaw, float lowTempC, int32_t highRaw, float highTempC);
bool temperatureCalibrationRecordValid(const TemperatureCalibrationRecord &record);
TemperatureCalibrationRecord makeTemperatureCalibrationRecord(
    bool lowValid, int32_t lowRaw, float lowTempC,
    bool highValid, int32_t highRaw, float highTempC,
    float trimOffsetC, bool enabled, uint8_t generation = 0);
