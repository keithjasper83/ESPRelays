#include "telemetry_diagnostics.h"
#include "telemetry_buffer.h"

namespace telemetry {

TelemetryDiagnostics::TelemetryDiagnostics() {
}

String TelemetryDiagnostics::getSystemInfo() const {
    return String();
}

String TelemetryDiagnostics::getMemoryDiagnostics() const {
    return String();
}

String TelemetryDiagnostics::getNetworkDiagnostics() const {
    return String();
}

String TelemetryDiagnostics::getRelayDiagnostics(const telemetry::TelemetrySnapshot& snapshot) const {
    return String();
}

String TelemetryDiagnostics::getBufferDiagnostics(const telemetry::TelemetryBuffer& buffer) const {
    return String();
}

String TelemetryDiagnostics::getFullDiagnostics(const telemetry::TelemetrySnapshot& snapshot,
                                                 const telemetry::TelemetryBuffer& buffer) const {
    return String();
}

String TelemetryDiagnostics::formatValue(const String& label, const String& value) const {
    return label + ": " + value;
}

} // namespace telemetry
