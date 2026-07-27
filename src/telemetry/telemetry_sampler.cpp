#include "telemetry_sampler.h"

namespace telemetry {

TelemetrySampler::TelemetrySampler()
    : cpuTemperatureCallback_(nullptr)
    , memoryUsageCallback_(nullptr)
    , wifiConnectedCallback_(nullptr)
    , signalStrengthCallback_(nullptr) {
    for (int i = 0; i < 4; i++) {
        relayStateCallbacks_[i] = nullptr;
    }
}

TelemetrySnapshot TelemetrySampler::sample() {
    TelemetrySnapshot snapshot;
    
    snapshot.cpuTemperature = sampleCpuTemperature();
    snapshot.memoryUsage = sampleMemoryUsage();
    snapshot.wifiConnected = sampleWifiConnected();
    snapshot.signalStrength = sampleSignalStrength();
    snapshot.relay1State = sampleRelayState(0);
    snapshot.relay2State = sampleRelayState(1);
    snapshot.relay3State = sampleRelayState(2);
    snapshot.relay4State = sampleRelayState(3);
    snapshot.snapshotTimestamp = millis();
    
    return snapshot;
}

float TelemetrySampler::sampleCpuTemperature() {
    if (cpuTemperatureCallback_) {
        return cpuTemperatureCallback_();
    }
    return 0.0f;
}

float TelemetrySampler::sampleMemoryUsage() {
    if (memoryUsageCallback_) {
        return memoryUsageCallback_();
    }
    return 0.0f;
}

bool TelemetrySampler::sampleWifiConnected() {
    if (wifiConnectedCallback_) {
        return wifiConnectedCallback_();
    }
    return false;
}

uint32_t TelemetrySampler::sampleSignalStrength() {
    if (signalStrengthCallback_) {
        return signalStrengthCallback_();
    }
    return 0;
}

bool TelemetrySampler::sampleRelayState(uint8_t relayIndex) {
    if (relayIndex < 4 && relayStateCallbacks_[relayIndex]) {
        return relayStateCallbacks_[relayIndex]();
    }
    return false;
}

void TelemetrySampler::setCpuTemperatureCallback(std::function<float()> callback) {
    cpuTemperatureCallback_ = callback;
}

void TelemetrySampler::setMemoryUsageCallback(std::function<float()> callback) {
    memoryUsageCallback_ = callback;
}

void TelemetrySampler::setWifiConnectedCallback(std::function<bool()> callback) {
    wifiConnectedCallback_ = callback;
}

void TelemetrySampler::setSignalStrengthCallback(std::function<uint32_t()> callback) {
    signalStrengthCallback_ = callback;
}

void TelemetrySampler::setRelayStateCallback(uint8_t index, std::function<bool()> callback) {
    if (index < 4) {
        relayStateCallbacks_[index] = callback;
    }
}

} // namespace telemetry
