#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include "telemetry_snapshot.h"
#include "telemetry_scheduler.h"
#include "telemetry_sampler.h"
#include "telemetry_encoder.h"
#include "telemetry_transport.h"
#include "telemetry_buffer.h"
#include "telemetry_stats.h"
#include "telemetry_diagnostics.h"
#include "telemetry_config.h"
#include <Arduino.h>

namespace telemetry {

/**
 * @brief Owns the subsystem.
 * 
 * Responsible only for orchestration.
 * Should call:
 * - scheduler
 * - sampler
 * - encoder
 * - transport
 * - diagnostics
 * 
 * Should contain almost no business logic.
 */
class TelemetryManager {
public:
    TelemetryManager();
    
    // Initialize the telemetry subsystem
    bool initialize();
    
    // Shutdown the telemetry subsystem
    void shutdown();
    
    // Main loop - check if telemetry should run
    void loop();
    
    // Force immediate telemetry collection and publish
    bool forcePublish();
    
    // Get current telemetry snapshot
    TelemetrySnapshot getSnapshot() const;
    
    // Get subsystem components
    const TelemetryConfig& getConfig() const;
    const TelemetryScheduler& getScheduler() const;
    const TelemetrySampler& getSampler() const;
    const TelemetryEncoder& getEncoder() const;
    const TelemetryTransport& getTransport() const;
    const TelemetryBuffer& getBuffer() const;
    const TelemetryStats& getStats() const;
    const TelemetryDiagnostics& getDiagnostics() const;
    
private:
    // Orchestration
    void onTelemetryScheduled();
    void handlePublishResult(bool success);
    
    TelemetryConfig config_;
    TelemetryScheduler scheduler_;
    TelemetrySampler sampler_;
    TelemetryEncoder encoder_;
    TelemetryTransport transport_;
    TelemetryBuffer buffer_;
    TelemetryStats stats_;
    TelemetryDiagnostics diagnostics_;
    
    TelemetrySnapshot currentSnapshot_;
};

} // namespace telemetry

#endif // TELEMETRY_MANAGER_H
