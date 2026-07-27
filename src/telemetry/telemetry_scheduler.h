#ifndef TELEMETRY_SCHEDULER_H
#define TELEMETRY_SCHEDULER_H

#include <Arduino.h>

namespace telemetry {

class TelemetryConfig;

/**
 * @brief Determines when telemetry should run.
 * 
 * Owns timing only.
 * No MQTT.
 * No JSON.
 * No sampling.
 */
class TelemetryScheduler {
public:
    TelemetryScheduler();
    
    // Initialize scheduler with config
    void initialize(const telemetry::TelemetryConfig& config);
    
    // Check if telemetry should be scheduled now
    bool shouldSchedule() const;
    
    // Schedule next telemetry run
    void scheduleNext();
    
    // Get time until next schedule
    uint32_t getTimeUntilNext() const;
    
    // Set custom schedule time
    void setScheduleTime(uint32_t timestamp);
    
    // Get current schedule time
    uint32_t getScheduleTime() const;
    
private:
    uint32_t nextScheduleTime_;
    uint32_t lastScheduleTime_;
    uint32_t publishInterval_;
};

} // namespace telemetry

#endif // TELEMETRY_SCHEDULER_H
