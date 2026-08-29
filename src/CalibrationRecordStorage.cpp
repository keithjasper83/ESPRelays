/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "CalibrationRecordStorage.h"

#include <string.h>

namespace
{
    bool readValidCalibration(CalibrationBlobStorage &storage, const char *key,
                              TemperatureCalibrationRecord &record)
    {
        return storage.length(key) == sizeof(record) &&
               storage.read(key, &record, sizeof(record)) == sizeof(record) &&
               temperatureCalibrationRecordValid(record);
    }

    bool generationNewer(uint8_t candidate, uint8_t current)
    {
        return candidate != current && static_cast<uint8_t>(candidate - current) < 128U;
    }
}

bool writeVerifiedCalibration(CalibrationBlobStorage &storage, const char *key,
                              const TemperatureCalibrationRecord &record)
{
    if (storage.write(key, &record, sizeof(record)) != sizeof(record)) return false;

    TemperatureCalibrationRecord readback{};
    return storage.length(key) == sizeof(readback) &&
           storage.read(key, &readback, sizeof(readback)) == sizeof(readback) &&
           memcmp(&readback, &record, sizeof(record)) == 0 &&
           temperatureCalibrationRecordValid(readback);
}

SelectedCalibrationRecord selectNewestCalibration(CalibrationBlobStorage &storage,
                                                   const char *keyA, const char *keyB)
{
    TemperatureCalibrationRecord recordA;
    TemperatureCalibrationRecord recordB;
    const bool recordAValid = readValidCalibration(storage, keyA, recordA);
    const bool recordBValid = readValidCalibration(storage, keyB, recordB);

    if (recordBValid && (!recordAValid || generationNewer(recordB.reserved, recordA.reserved)))
    {
        SelectedCalibrationRecord selected;
        selected.record = recordB;
        selected.key = keyB;
        selected.found = true;
        return selected;
    }
    if (recordAValid)
    {
        SelectedCalibrationRecord selected;
        selected.record = recordA;
        selected.key = keyA;
        selected.found = true;
        return selected;
    }
    return {};
}
