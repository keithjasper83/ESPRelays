#include "telemetry_manager.h"

namespace telemetry {

TelemetryManager::TelemetryManager() {
}

bool TelemetryManager::initialize() {
    // Initialize all subsystems
    config_.setEnabled(true);
    
    if (!transport_.initialize()) {
        return false;
    }
    
    return true;
}

void TelemetryManager::shutdown() {
    transport_.shutdown();
}

void TelemetryManager::loop() {
    // Check if telemetry should be scheduled
    if (scheduler_.shouldSchedule()) {
        onTelemetryScheduled();
    }
}

bool TelemetryManager::forcePublish() {
    // Sample data
    currentSnapshot_ = sampler_.sample();
    
    // Encode
    String payload = encoder_.encodeToJson(currentSnapshot_);
    
    // Publish
    bool success = transport_.publish(currentSnapshot_);
    
    if (success) {
        stats_.incrementPacketsSent();
    }
    
    return success;
}

TelemetrySnapshot TelemetryManager::getSnapshot() const {
    return currentSnapshot_;
}

const TelemetryConfig& TelemetryManager::getConfig() const {
    return config_;
}

const TelemetryScheduler& TelemetryManager::getScheduler() const {
    return scheduler_;
}

const TelemetrySampler& TelemetryManager::getSampler() const {
    return sampler_;
}

const TelemetryEncoder& TelemetryManager::getEncoder() const {
    return encoder_;
}

const TelemetryTransport& TelemetryManager::getTransport() const {
    return transport_;
}

const TelemetryBuffer& TelemetryManager::getBuffer() const {
    return buffer_;
}

const TelemetryStats& TelemetryManager::getStats() const {
    return stats_;
}

const TelemetryDiagnostics& TelemetryManager::getDiagnostics() const {
    return diagnostics_;
}

void TelemetryManager::onTelemetryScheduled() {
    // Sample data
    currentSnapshot_ = sampler_.sample();
    
    // Encode
    String payload = encoder_.encodeToJson(currentSnapshot_);
    
    // Publish
    bool success = transport_.publish(currentSnapshot_);
    
    if (success) {
        stats_.incrementPacketsSent();
    }
    
    // Schedule next run
    scheduler_.scheduleNext();
}

void TelemetryManager::handlePublishResult(bool success) {
    if (success) {
        stats_.incrementPacketsSent();
    }
}

} // namespace telemetry
