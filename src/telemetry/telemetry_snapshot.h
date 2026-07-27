#ifndef TELEMETRY_SNAPSHOT_H
#define TELEMETRY_SNAPSHOT_H

#include <Arduino.h>

namespace telemetry {

/**
 * @brief Plain data structures for telemetry data.
 * 
 * No methods except trivial constructors if required.
 */

/**
 * @brief Represents a single telemetry data point.
 */
struct TelemetryDataPoint {
    float value;
    uint32_t timestamp;
    
    TelemetryDataPoint() : value(0.0f), timestamp(0) {}
    TelemetryDataPoint(float val, uint32_t ts) : value(val), timestamp(ts) {}
};

/**
 * @brief Complete telemetry snapshot containing all data points.
 */
struct TelemetrySnapshot {
    // System metrics
    float cpuTemperature;
    float memoryUsage;
    uint32_t uptime;
    
    // Network status
    bool wifiConnected;
    uint32_t signalStrength;
    uint32_t ipAddress[4];
    
    // Relay status
    bool relay1State;
    bool relay2State;
    bool relay3State;
    bool relay4State;
    
    // Timestamp of this snapshot
    uint32_t snapshotTimestamp;
    
    TelemetrySnapshot();
};

} // namespace telemetry

#endif // TELEMETRY_SNAPSHOT_H
