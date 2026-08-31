/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <stddef.h>

#include "TemperatureCalibrationRecord.h"

class CalibrationBlobStorage
{
public:
    virtual ~CalibrationBlobStorage() = default;

    virtual size_t length(const char *key) = 0;
    virtual size_t read(const char *key, void *value, size_t length) = 0;
    virtual size_t write(const char *key, const void *value, size_t length) = 0;
};

struct SelectedCalibrationRecord
{
    TemperatureCalibrationRecord record{};
    const char *key = nullptr;
    bool found = false;
};

bool writeVerifiedCalibration(CalibrationBlobStorage &storage, const char *key,
                              const TemperatureCalibrationRecord &record);
SelectedCalibrationRecord selectNewestCalibration(CalibrationBlobStorage &storage,
                                                   const char *keyA, const char *keyB);
