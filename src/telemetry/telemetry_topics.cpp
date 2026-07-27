#include "telemetry_topics.h"
#include <Arduino.h>

namespace telemetry {

const char* TelemetryTopics::TELEMETRY = "telemetry";
const char* TelemetryTopics::STATUS = "status";
const char* TelemetryTopics::METRICS = "metrics";
const char* TelemetryTopics::RELAYS = "relays";

const char* TelemetryTopics::SYSTEM_STATUS = "system/status";
const char* TelemetryTopics::CPU_METRICS = "metrics/cpu";
const char* TelemetryTopics::MEMORY_METRICS = "metrics/memory";
const char* TelemetryTopics::NETWORK_STATUS = "network/status";
const char* TelemetryTopics::RELAY_1 = "relays/1";
const char* TelemetryTopics::RELAY_2 = "relays/2";
const char* TelemetryTopics::RELAY_3 = "relays/3";
const char* TelemetryTopics::RELAY_4 = "relays/4";

const char* TelemetryTopics::ALL_METRICS = "telemetry/+";
const char* TelemetryTopics::ALL_RELAYS = "relays/+";

const char* TelemetryTopics::DIAGNOSTICS = "telemetry/diagnostics";
const char* TelemetryTopics::ERROR_LOG = "telemetry/errors";

TelemetryTopics::TelemetryTopics() {
}

String TelemetryTopics::buildTopic(const char* prefix, const char* category, const char* subcategory) {
    String topic;
    topic.reserve(MAX_TOPIC_LENGTH);
    
    if (prefix) {
        topic += prefix;
        topic += "/";
    }
    
    topic += category;
    
    if (subcategory) {
        topic += "/";
        topic += subcategory;
    }
    
    return topic;
}

} // namespace telemetry
