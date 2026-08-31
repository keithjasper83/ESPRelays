#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace FakeRuntime
{
    inline std::vector<std::string> events;
    inline std::map<int, int> pinModes;
    inline std::map<int, int> pinLevels;

    inline void resetObservations()
    {
        events.clear();
        pinModes.clear();
        pinLevels.clear();
    }

    inline size_t preferenceWriteCount()
    {
        return static_cast<size_t>(std::count_if(
            events.begin(), events.end(), [](const std::string &event) {
                return event.rfind("preferences.put", 0) == 0;
            }));
    }
}
