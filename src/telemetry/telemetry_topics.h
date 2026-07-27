#ifndef TELEMETRY_TOPICS_H
#define TELEMETRY_TOPICS_H

#include <Arduino.h>

namespace telemetry {

/**
 * @brief Defines telemetry topic structure.
 * 
 * Topic format: {prefix}/{category}/{subcategory}
 */
class TelemetryTopics {
public:
    // Root topics
    static const char* TELEMETRY;
    static const char* STATUS;
    static const char* METRICS;
    static const char* RELAYS;
    
    // Combined topics
    static const char* SYSTEM_STATUS;
    static const char* CPU_METRICS;
    static const char* MEMORY_METRICS;
    static const char* NETWORK_STATUS;
    static const char* RELAY_1;
    static const char* RELAY_2;
    static const char* RELAY_3;
    static const char* RELAY_4;
    
    // Wildcard topics
    static const char* ALL_METRICS;
    static const char* ALL_RELAYS;
    
    // Diagnostic topics
    static const char* DIAGNOSTICS;
    static const char* ERROR_LOG;
    
    TelemetryTopics();
    
    // Build topic with prefix
    static String buildTopic(const char* prefix, const char* category, const char* subcategory = nullptr);
    
private:
    static const size_t MAX_TOPIC_LENGTH = 64;
};

} // namespace telemetry

#endif // TELEMETRY_TOPICS_H
