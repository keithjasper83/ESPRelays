/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Keith Jasper
 * Contact: https://github.com/keithjasper83/ESPRelays/issues
 */

#include "ScheduleManager.h"

#include <Preferences.h>

namespace
{
    constexpr char SCHED_PREF_NAMESPACE[] = "sched_cfg";
    constexpr char SCHED_PREF_NEXT_ID[] = "next_id";

    String slotKey(uint8_t slot)
    {
        return String("ev") + String(slot);
    }
}

void ScheduleManager::loadSettings()
{
    if (settingsLoaded)
    {
        return;
    }

    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        events[i] = {};
    }

    Preferences preferences;
    if (!preferences.begin(SCHED_PREF_NAMESPACE, false))
    {
        settingsLoaded = true;
        return;
    }

    nextId = preferences.getUChar(SCHED_PREF_NEXT_ID, 1);
    if (nextId == 0)
    {
        nextId = 1;
    }

    bool needsPersist = false;

    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        const String key = slotKey(i);
        if (!preferences.isKey(key.c_str()))
        {
            continue;
        }

        const String line = preferences.getString(key.c_str(), "");
        if (line.length() == 0)
        {
            continue;
        }

        EventRecord event;
        if (!deserialize(line, event))
        {
            needsPersist = true;
            continue;
        }

        String error;
        if (!validate(event.data, error))
        {
            needsPersist = true;
            continue;
        }

        event.used = true;
        events[i] = event;
    }

    preferences.end();
    settingsLoaded = true;

    if (needsPersist)
    {
        persistSettings();
    }
}

void ScheduleManager::persistSettings()
{
    Preferences preferences;
    if (!preferences.begin(SCHED_PREF_NAMESPACE, false))
    {
        return;
    }

    preferences.putUChar(SCHED_PREF_NEXT_ID, nextId);

    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        const String key = slotKey(i);
        if (!events[i].used)
        {
            preferences.putString(key.c_str(), "");
            continue;
        }

        preferences.putString(key.c_str(), serialize(events[i]));
    }

    preferences.end();
}

bool ScheduleManager::validate(const EventData &data, String &error) const
{
    if (data.hour > 23)
    {
        error = "Hour must be 0-23";
        return false;
    }

    if (data.minute > 59)
    {
        error = "Minute must be 0-59";
        return false;
    }

    if (data.command.length() == 0 || data.command.length() > 24)
    {
        error = "Command is required and must be <=24 chars";
        return false;
    }

    if (data.recurring)
    {
        if (data.dowMask == 0)
        {
            error = "Recurring events require a non-zero day mask";
            return false;
        }

        return true;
    }

    if (data.year < 2020 || data.year > 2099)
    {
        error = "Year must be 2020-2099";
        return false;
    }

    if (data.month < 1 || data.month > 12)
    {
        error = "Month must be 1-12";
        return false;
    }

    if (data.day < 1 || data.day > 31)
    {
        error = "Day must be 1-31";
        return false;
    }

    return true;
}

int ScheduleManager::findSlotById(uint8_t id) const
{
    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        if (events[i].used && events[i].id == id)
        {
            return i;
        }
    }

    return -1;
}

int ScheduleManager::firstFreeSlot() const
{
    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        if (!events[i].used)
        {
            return i;
        }
    }

    return -1;
}

uint32_t ScheduleManager::minuteStampFromTm(const tm &localTime) const
{
    const uint32_t year = static_cast<uint32_t>(localTime.tm_year + 1900);
    return (year * 600000UL) + (static_cast<uint32_t>(localTime.tm_yday) * 1440UL) + (static_cast<uint32_t>(localTime.tm_hour) * 60UL) + static_cast<uint32_t>(localTime.tm_min);
}

bool ScheduleManager::shouldRun(const EventRecord &event, const tm &localTime, uint32_t minuteStamp) const
{
    if (!event.used || !event.data.enabled)
    {
        return false;
    }

    if (event.data.hour != localTime.tm_hour || event.data.minute != localTime.tm_min)
    {
        return false;
    }

    if (event.lastRunMinuteStamp == minuteStamp)
    {
        return false;
    }

    if (event.data.recurring)
    {
        const uint8_t mondayFirstIndex = static_cast<uint8_t>((localTime.tm_wday + 6) % 7);
        const uint8_t bit = static_cast<uint8_t>(1U << mondayFirstIndex);
        return (event.data.dowMask & bit) != 0;
    }

    const uint16_t year = static_cast<uint16_t>(localTime.tm_year + 1900);
    const uint8_t month = static_cast<uint8_t>(localTime.tm_mon + 1);
    const uint8_t day = static_cast<uint8_t>(localTime.tm_mday);

    return year == event.data.year && month == event.data.month && day == event.data.day;
}

String ScheduleManager::serialize(const EventRecord &event) const
{
    String line;
    line.reserve(64 + event.data.command.length());
    line += String(event.id);
    line += "|";
    line += event.data.enabled ? "1" : "0";
    line += "|";
    line += event.data.recurring ? "1" : "0";
    line += "|";
    line += String(event.data.hour);
    line += "|";
    line += String(event.data.minute);
    line += "|";
    line += String(event.data.dowMask);
    line += "|";
    line += String(event.data.year);
    line += "|";
    line += String(event.data.month);
    line += "|";
    line += String(event.data.day);
    line += "|";
    line += event.data.command;
    return line;
}

bool ScheduleManager::deserialize(const String &line, EventRecord &event) const
{
    int start = 0;
    int part = 0;
    String values[10];

    while (part < 9)
    {
        const int sep = line.indexOf('|', start);
        if (sep < 0)
        {
            return false;
        }

        values[part++] = line.substring(start, sep);
        start = sep + 1;
    }

    values[9] = line.substring(start);

    event.id = static_cast<uint8_t>(values[0].toInt());
    event.data.enabled = values[1] == "1";
    event.data.recurring = values[2] == "1";
    event.data.hour = static_cast<uint8_t>(values[3].toInt());
    event.data.minute = static_cast<uint8_t>(values[4].toInt());
    event.data.dowMask = static_cast<uint8_t>(values[5].toInt());
    event.data.year = static_cast<uint16_t>(values[6].toInt());
    event.data.month = static_cast<uint8_t>(values[7].toInt());
    event.data.day = static_cast<uint8_t>(values[8].toInt());
    event.data.command = values[9];
    event.lastRunMinuteStamp = 0;

    return event.id != 0;
}

void ScheduleManager::begin()
{
    loadSettings();
}

void ScheduleManager::maintain(bool timeValid, DispatchHandler dispatchHandler)
{
    loadSettings();

    if (!timeValid || dispatchHandler == nullptr)
    {
        return;
    }

    time_t now = time(nullptr);
    struct tm localTime;
    localtime_r(&now, &localTime);

    const uint32_t stamp = minuteStampFromTm(localTime);
    bool changed = false;

    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        EventRecord &event = events[i];
        if (!shouldRun(event, localTime, stamp))
        {
            continue;
        }

        dispatchHandler(event.data.command);
        event.lastRunMinuteStamp = stamp;

        if (!event.data.recurring)
        {
            event.data.enabled = false;
            changed = true;
        }
    }

    if (changed)
    {
        persistSettings();
    }
}

bool ScheduleManager::addEvent(const EventData &data, uint8_t &createdId, String &error)
{
    loadSettings();

    if (count() >= SCHEDULE_MAX_EVENTS)
    {
        error = "Schedule capacity reached";
        return false;
    }

    if (!validate(data, error))
    {
        return false;
    }

    const int slot = firstFreeSlot();
    if (slot < 0)
    {
        error = "No free schedule slot";
        return false;
    }

    EventRecord &event = events[slot];
    event.used = true;
    event.id = nextId++;
    if (nextId == 0)
    {
        nextId = 1;
    }
    event.data = data;
    event.lastRunMinuteStamp = 0;

    persistSettings();
    createdId = event.id;
    return true;
}

bool ScheduleManager::updateEvent(uint8_t id, const EventData &data, String &error)
{
    loadSettings();

    if (!validate(data, error))
    {
        return false;
    }

    const int slot = findSlotById(id);
    if (slot < 0)
    {
        error = "Event not found";
        return false;
    }

    events[slot].data = data;
    events[slot].lastRunMinuteStamp = 0;
    persistSettings();
    return true;
}

bool ScheduleManager::removeEvent(uint8_t id)
{
    loadSettings();

    const int slot = findSlotById(id);
    if (slot < 0)
    {
        return false;
    }

    events[slot] = {};
    persistSettings();
    return true;
}

String ScheduleManager::eventsJson() const
{
    String json = "[";
    bool first = true;

    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        const EventRecord &event = events[i];
        if (!event.used)
        {
            continue;
        }

        if (!first)
        {
            json += ",";
        }
        first = false;

        json += "{";
        json += "\"id\":" + String(event.id) + ",";
        json += "\"enabled\":" + String(event.data.enabled ? "true" : "false") + ",";
        json += "\"recurring\":" + String(event.data.recurring ? "true" : "false") + ",";
        json += "\"hour\":" + String(event.data.hour) + ",";
        json += "\"minute\":" + String(event.data.minute) + ",";
        json += "\"dow_mask\":" + String(event.data.dowMask) + ",";
        json += "\"year\":" + String(event.data.year) + ",";
        json += "\"month\":" + String(event.data.month) + ",";
        json += "\"day\":" + String(event.data.day) + ",";
        json += "\"command\":\"" + event.data.command + "\"";
        json += "}";
    }

    json += "]";
    return json;
}

uint8_t ScheduleManager::count() const
{
    uint8_t used = 0;
    for (uint8_t i = 0; i < SCHEDULE_MAX_EVENTS; i++)
    {
        if (events[i].used)
        {
            used++;
        }
    }
    return used;
}

uint8_t ScheduleManager::capacity() const
{
    return SCHEDULE_MAX_EVENTS;
}
