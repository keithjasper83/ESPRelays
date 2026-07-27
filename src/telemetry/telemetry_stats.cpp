#include "telemetry_stats.h"

namespace telemetry {

TelemetryStats::TelemetryStats()
    : packetsSent_(0)
    , packetsReceived_(0)
    , packetsDropped_(0)
    , totalRetries_(0)
    , lastPublishTime_(0)
    , lastReceiveTime_(0)
    , connectionFailures_(0)
    , publishFailures_(0) {
}

uint32_t TelemetryStats::getPacketsSent() const {
    return packetsSent_;
}

uint32_t TelemetryStats::getPacketsReceived() const {
    return packetsReceived_;
}

uint32_t TelemetryStats::getPacketsDropped() const {
    return packetsDropped_;
}

uint32_t TelemetryStats::getTotalRetries() const {
    return totalRetries_;
}

uint32_t TelemetryStats::getLastPublishTime() const {
    return lastPublishTime_;
}

uint32_t TelemetryStats::getLastReceiveTime() const {
    return lastReceiveTime_;
}

uint32_t TelemetryStats::getConnectionFailures() const {
    return connectionFailures_;
}

uint32_t TelemetryStats::getPublishFailures() const {
    return publishFailures_;
}

void TelemetryStats::reset() {
    packetsSent_ = 0;
    packetsReceived_ = 0;
    packetsDropped_ = 0;
    totalRetries_ = 0;
    lastPublishTime_ = 0;
    lastReceiveTime_ = 0;
    connectionFailures_ = 0;
    publishFailures_ = 0;
}

void TelemetryStats::resetPacketCounters() {
    packetsSent_ = 0;
    packetsReceived_ = 0;
    packetsDropped_ = 0;
    totalRetries_ = 0;
}

void TelemetryStats::resetErrorCounters() {
    connectionFailures_ = 0;
    publishFailures_ = 0;
}

void TelemetryStats::incrementPacketsSent() {
    packetsSent_++;
}

void TelemetryStats::incrementPacketsReceived() {
    packetsReceived_++;
}

void TelemetryStats::incrementPacketsDropped() {
    packetsDropped_++;
}

void TelemetryStats::incrementTotalRetries() {
    totalRetries_++;
}

void TelemetryStats::incrementConnectionFailures() {
    connectionFailures_++;
}

void TelemetryStats::incrementPublishFailures() {
    publishFailures_++;
}

} // namespace telemetry
