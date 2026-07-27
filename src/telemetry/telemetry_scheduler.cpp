#include "telemetry_scheduler.h"
#include "telemetry_config.h"

namespace telemetry {

TelemetryScheduler::TelemetryScheduler()
    : nextScheduleTime_(0)
    , lastScheduleTime_(0)
    , publishInterval_(0) {
}

void TelemetryScheduler::initialize(const TelemetryConfig& config) {
    publishInterval_ = config.getPublishIntervalMs();
    nextScheduleTime_ = millis() + publishInterval_;
    lastScheduleTime_ = 0;
}

bool TelemetryScheduler::shouldSchedule() const {
    return millis() >= nextScheduleTime_;
}

void TelemetryScheduler::scheduleNext() {
    lastScheduleTime_ = millis();
    nextScheduleTime_ = millis() + publishInterval_;
}

uint32_t TelemetryScheduler::getTimeUntilNext() const {
    if (nextScheduleTime_ == 0) {
        return 0;
    }
    return nextScheduleTime_ - millis();
}

void TelemetryScheduler::setScheduleTime(uint32_t timestamp) {
    nextScheduleTime_ = timestamp;
}

uint32_t TelemetryScheduler::getScheduleTime() const {
    return nextScheduleTime_;
}

} // namespace telemetry
