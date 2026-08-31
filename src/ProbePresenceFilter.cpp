#include "ProbePresenceFilter.h"

ProbePresenceFilter::ProbePresenceFilter(uint8_t presentSamples, uint8_t absentSamples,
                                         int minRaw, int maxRaw)
    : presentSamples_(presentSamples), absentSamples_(absentSamples),
      minRaw_(minRaw), maxRaw_(maxRaw)
{
}

bool ProbePresenceFilter::update(int raw, bool monitoringEnabled)
{
    if (!monitoringEnabled)
    {
        reset();
        return false;
    }

    const bool inRange = raw > minRaw_ && raw < maxRaw_;
    if (inRange)
    {
        inRangeCount_ = inRangeCount_ < presentSamples_ ? inRangeCount_ + 1 : inRangeCount_;
        outOfRangeCount_ = 0;
        if (!present_ && inRangeCount_ >= presentSamples_)
        {
            present_ = true;
        }
        if (present_)
        {
            stableRaw_ = raw;
        }
    }
    else
    {
        outOfRangeCount_ = outOfRangeCount_ < absentSamples_ ? outOfRangeCount_ + 1 : outOfRangeCount_;
        inRangeCount_ = 0;
        if (present_ && outOfRangeCount_ >= absentSamples_)
        {
            present_ = false;
            stableRaw_ = -1;
        }
    }

    return present_;
}

void ProbePresenceFilter::reset()
{
    inRangeCount_ = 0;
    outOfRangeCount_ = 0;
    stableRaw_ = -1;
    present_ = false;
}
