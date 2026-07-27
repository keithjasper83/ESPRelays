#ifndef TELEMETRY_ENCODER_H
#define TELEMETRY_ENCODER_H

#include "telemetry_snapshot.h"
#include <Arduino.h>

namespace telemetry {

/**
 * @brief Converts TelemetrySnapshot into payloads.
 * 
 * Owns JSON generation.
 * No MQTT.
 * No timing.
 */
class TelemetryEncoder {
public:
    TelemetryEncoder();
    
    // Encode snapshot to JSON string
    String encodeToJson(const TelemetrySnapshot& snapshot) const;
    
    // Encode snapshot to compact format
    String encodeCompact(const TelemetrySnapshot& snapshot) const;
    
    // Encode snapshot to binary format (placeholder)
    String encodeBinary(const TelemetrySnapshot& snapshot) const;
    
    // Get estimated payload size
    size_t estimatePayloadSize(const TelemetrySnapshot& snapshot) const;
    
private:
    // Helper to escape JSON strings
    String escapeJsonString(const String& str) const;
};

} // namespace telemetry

#endif // TELEMETRY_ENCODER_H
