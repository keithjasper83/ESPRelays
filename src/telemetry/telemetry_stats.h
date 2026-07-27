#ifndef TELEMETRY_STATS_H
#define TELEMETRY_STATS_H

#include <Arduino.h>

namespace telemetry {

class TelemetryConfig;

/**
 * @brief Owns counters for telemetry statistics.
 * 
 * Examples:
 * - Packets sent
 * - Packets dropped
 * - Retries
 * - Last publish timestamp
 * 
 * No transport logic.
 */
class TelemetryStats {
public:
    TelemetryStats();
    
    // Packet counters
    uint32_t getPacketsSent() const;
    uint32_t getPacketsReceived() const;
    uint32_t getPacketsDropped() const;
    uint32_t getTotalRetries() const;
    
    // Timing
    uint32_t getLastPublishTime() const;
    uint32_t getLastReceiveTime() const;
    
    // Error counters
    uint32_t getConnectionFailures() const;
    uint32_t getPublishFailures() const;
    
    // Reset all counters
    void reset();
    
    // Increment counters
    void incrementPacketsSent();
    void incrementPacketsReceived();
    void incrementPacketsDropped();
    void incrementTotalRetries();
    void incrementConnectionFailures();
    void incrementPublishFailures();
    
    // Reset specific counters
    void resetPacketCounters();
    void resetErrorCounters();
    
private:
    uint32_t packetsSent_;
    uint32_t packetsReceived_;
    uint32_t packetsDropped_;
    uint32_t totalRetries_;
    uint32_t lastPublishTime_;
    uint32_t lastReceiveTime_;
    uint32_t connectionFailures_;
    uint32_t publishFailures_;
};

} // namespace telemetry

#endif // TELEMETRY_STATS_H
