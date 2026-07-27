#ifndef TELEMETRY_SAMPLER_H
#define TELEMETRY_SAMPLER_H

#include "telemetry_snapshot.h"
#include <Arduino.h>

namespace telemetry {

/**
 * @brief Collects information from the rest of ESPRelays.
 * 
 * Produces a populated TelemetrySnapshot.
 * No encoding.
 * No transport.
 */
class TelemetrySampler {
public:
    TelemetrySampler();
    
    // Sample and create a telemetry snapshot
    TelemetrySnapshot sample();
    
    // Sample specific data points
    float sampleCpuTemperature();
    float sampleMemoryUsage();
    bool sampleWifiConnected();
    uint32_t sampleSignalStrength();
    bool sampleRelayState(uint8_t relayIndex);
    
    // Set callback for data sources
    void setCpuTemperatureCallback(std::function<float()> callback);
    void setMemoryUsageCallback(std::function<float()> callback);
    void setWifiConnectedCallback(std::function<bool()> callback);
    void setSignalStrengthCallback(std::function<uint32_t()> callback);
    void setRelayStateCallback(uint8_t index, std::function<bool()> callback);
    
private:
    std::function<float()> cpuTemperatureCallback_;
    std::function<float()> memoryUsageCallback_;
    std::function<bool()> wifiConnectedCallback_;
    std::function<uint32_t()> signalStrengthCallback_;
    std::function<bool()> relayStateCallbacks_[4];
};

} // namespace telemetry

#endif // TELEMETRY_SAMPLER_H
