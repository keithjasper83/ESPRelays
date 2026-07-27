#include "telemetry_transport.h"

namespace telemetry {

TelemetryTransport::TelemetryTransport()
    : connected_(false) {
}

bool TelemetryTransport::initialize() {
    return false;
}

void TelemetryTransport::shutdown() {
}

bool TelemetryTransport::publish(const TelemetrySnapshot& snapshot) {
    return false;
}

bool TelemetryTransport::flushBuffer(TelemetryBuffer& buffer) {
    return false;
}

bool TelemetryTransport::isConnected() const {
    return connected_;
}

String TelemetryTransport::getLastError() const {
    return lastError_;
}

void TelemetryTransport::setConnectionCallback(std::function<void(bool)> callback) {
    connectionCallback_ = callback;
}

bool TelemetryTransport::attemptPublish(const TelemetrySnapshot& snapshot, uint32_t retries) {
    return false;
}

void TelemetryTransport::attemptReconnect() {
}

} // namespace telemetry
