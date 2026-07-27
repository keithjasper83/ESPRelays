#ifndef TELEMETRY_DIAGNOSTICS_H
#define TELEMETRY_DIAGNOSTICS_H

#include "telemetry_snapshot.h"
#include <Arduino.h>

namespace telemetry {

class TelemetryConfig;
class TelemetryStats;
class TelemetryBuffer;

/**
 * @brief Produces diagnostic information.
 * 
 * No publishing.
 * No scheduling.
 */
class TelemetryDiagnostics {
public:
    TelemetryDiagnostics();
    
    // Get system information
    String getSystemInfo() const;
    
    // Get memory diagnostics
    String getMemoryDiagnostics() const;
    
    // Get network diagnostics
    String getNetworkDiagnostics() const;
    
    // Get relay diagnostics
    String getRelayDiagnostics(const TelemetrySnapshot& snapshot) const;
    
    // Get buffer diagnostics
    String getBufferDiagnostics(const telemetry::TelemetryBuffer& buffer) const;
    
    // Get complete diagnostic report
    String getFullDiagnostics(const telemetry::TelemetrySnapshot& snapshot, 
                              const telemetry::TelemetryBuffer& buffer) const;
    
private:
    // Helper to format values
    String formatValue(const String& label, const String& value) const;
};

} // namespace telemetry

#endif // TELEMETRY_DIAGNOSTICS_H
