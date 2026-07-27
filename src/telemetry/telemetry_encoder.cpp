#include "telemetry_encoder.h"

namespace telemetry {

TelemetryEncoder::TelemetryEncoder() {
}

String TelemetryEncoder::encodeToJson(const TelemetrySnapshot& snapshot) const {
    return String();
}

String TelemetryEncoder::encodeCompact(const TelemetrySnapshot& snapshot) const {
    return String();
}

String TelemetryEncoder::encodeBinary(const TelemetrySnapshot& snapshot) const {
    return String();
}

size_t TelemetryEncoder::estimatePayloadSize(const TelemetrySnapshot& snapshot) const {
    return 0;
}

String TelemetryEncoder::escapeJsonString(const String& str) const {
    return String();
}

} // namespace telemetry
