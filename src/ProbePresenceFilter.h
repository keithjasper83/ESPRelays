#pragma once

#include <stdint.h>

class ProbePresenceFilter
{
public:
    ProbePresenceFilter(uint8_t presentSamples, uint8_t absentSamples, int minRaw, int maxRaw);

    bool update(int raw, bool monitoringEnabled);
    bool present() const { return present_; }
    int stableRaw() const { return present_ ? stableRaw_ : -1; }
    void reset();

private:
    uint8_t presentSamples_;
    uint8_t absentSamples_;
    uint8_t inRangeCount_ = 0;
    uint8_t outOfRangeCount_ = 0;
    int minRaw_;
    int maxRaw_;
    int stableRaw_ = -1;
    bool present_ = false;
};
