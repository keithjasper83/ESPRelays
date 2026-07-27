#ifndef TELEMETRY_TRANSPORT_H
#define TELEMETRY_TRANSPORT_H

#include "telemetry_snapshot.h"
#include "telemetry_buffer.h"
#include <Arduino.h>

namespace telemetry {

/**
 * @brief Publishes telemetry data.
 * 
 * Handles:
 * - MQTT publish
 * - Retry logic
 * - Reconnect support
 * - Flushing queued packets
 * 
 * No sampling.
 * No JSON creation.
 */
class TelemetryTransport {
public:
    TelemetryTransport();
    
    // Initialize transport (MQTT, Serial, etc.)
    bool initialize();
    
    // Shutdown transport
    void shutdown();
    
    // Publish a telemetry snapshot
    bool publish(const TelemetrySnapshot& snapshot);
    
    // Publish from buffer (flush queued packets)
    bool flushBuffer(TelemetryBuffer& buffer);
    
    // Check if transport is connected
    bool isConnected() const;
    
    // Get last error
    String getLastError() const;
    
    // Set callback for connection status changes
    void setConnectionCallback(std::function<void(bool)> callback);
    
private:
    // Retry logic
    bool attemptPublish(const TelemetrySnapshot& snapshot, uint32_t retries);
    
    // Reconnect logic
    void attemptReconnect();
    
    bool connected_;
    String lastError_;
    std::function<void(bool)> connectionCallback_;
};

} // namespace telemetry

#endif // TELEMETRY_TRANSPORT_H
