/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#pragma once

#include <Arduino.h>
#include <time.h>

#include "AppConfig.h"

class ScheduleManager
{
public:
    struct EventData
    {
        bool enabled = true;
        bool recurring = true;
        uint8_t hour = 0;
        uint8_t minute = 0;
        uint8_t dowMask = 0x7F; // bit0=Monday .. bit6=Sunday
        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day = 0;
        String command;
    };

    using DispatchHandler = bool (*)(const String &command);

    void begin();
    void maintain(bool timeValid, DispatchHandler dispatchHandler);

    bool addEvent(const EventData &data, uint8_t &createdId, String &error);
    bool updateEvent(uint8_t id, const EventData &data, String &error);
    bool removeEvent(uint8_t id);
    bool ensureRecurringEvent(const EventData &data, uint8_t &eventId, bool &created, String &error);

    String eventsJson() const;
    uint8_t count() const;
    uint8_t capacity() const;

private:
    struct EventRecord
    {
        bool used = false;
        uint8_t id = 0;
        EventData data;
        uint32_t lastRunMinuteStamp = 0;
    };

    void loadSettings();
    void persistSettings();
    bool validate(const EventData &data, String &error) const;
    int findSlotById(uint8_t id) const;
    int firstFreeSlot() const;
    bool shouldRun(const EventRecord &event, const tm &localTime, uint32_t minuteStamp) const;
    uint32_t minuteStampFromTm(const tm &localTime) const;
    String serialize(const EventRecord &event) const;
    bool deserialize(const String &line, EventRecord &event) const;

    EventRecord events[SCHEDULE_MAX_EVENTS] = {};
    uint8_t nextId = 1;
    bool settingsLoaded = false;
};
