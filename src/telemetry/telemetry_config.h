#ifndef TELEMETRY_CONFIG_H
#define TELEMETRY_CONFIG_H

#include <Arduino.h>

namespace telemetry {

/**
 * @brief Configuration values for the telemetry subsystem.
 * 
 * This module owns configurable values only:
 * - Publish intervals
 * - Enable flags
 * - Topic prefixes
 * 
 * No runtime logic.
 */
class TelemetryConfig {
public:
    // Publish intervals (in milliseconds)
    static const uint32_t DEFAULT_PUBLISH_INTERVAL_MS = 60000;
    static const uint32_t DEFAULT_RETRY_INTERVAL_MS = 5000;
    static const uint32_t DEFAULT_MAX_RETRIES = 3;
    
    // Enable flags
    static const bool DEFAULT_ENABLED = true;
    static const bool DEFAULT_MQTT_ENABLED = true;
    static const bool DEFAULT_SERIAL_ENABLED = true;
    
    // Topic prefix
    static constexpr const char* DEFAULT_TOPIC_PREFIX = "esprelay";
    
    TelemetryConfig();
    
    // Getters
    uint32_t getPublishIntervalMs() const;
    uint32_t getRetryIntervalMs() const;
    uint32_t getMaxRetries() const;
    bool isEnabled() const;
    bool isMqttEnabled() const;
    bool isSerialEnabled() const;
    const char* getTopicPrefix() const;
    
    // Setters
    void setPublishIntervalMs(uint32_t interval);
    void setRetryIntervalMs(uint32_t interval);
    void setMaxRetries(uint32_t retries);
    void setEnabled(bool enabled);
    void setMqttEnabled(bool enabled);
    void setSerialEnabled(bool enabled);
    void setTopicPrefix(const char* prefix);
    
private:
    uint32_t publishIntervalMs_;
    uint32_t retryIntervalMs_;
    uint32_t maxRetries_;
    bool enabled_;
    bool mqttEnabled_;
    bool serialEnabled_;
    char topicPrefix_[32];
};

} // namespace telemetry

#endif // TELEMETRY_CONFIG_H
