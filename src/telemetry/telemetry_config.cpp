#include "telemetry_config.h"

namespace telemetry {

TelemetryConfig::TelemetryConfig()
    : publishIntervalMs_(DEFAULT_PUBLISH_INTERVAL_MS)
    , retryIntervalMs_(DEFAULT_RETRY_INTERVAL_MS)
    , maxRetries_(DEFAULT_MAX_RETRIES)
    , enabled_(DEFAULT_ENABLED)
    , mqttEnabled_(DEFAULT_MQTT_ENABLED)
    , serialEnabled_(DEFAULT_SERIAL_ENABLED) {
    strncpy(topicPrefix_, DEFAULT_TOPIC_PREFIX, sizeof(topicPrefix_));
    topicPrefix_[sizeof(topicPrefix_) - 1] = '\0';
}

uint32_t TelemetryConfig::getPublishIntervalMs() const {
    return publishIntervalMs_;
}

uint32_t TelemetryConfig::getRetryIntervalMs() const {
    return retryIntervalMs_;
}

uint32_t TelemetryConfig::getMaxRetries() const {
    return maxRetries_;
}

bool TelemetryConfig::isEnabled() const {
    return enabled_;
}

bool TelemetryConfig::isMqttEnabled() const {
    return mqttEnabled_;
}

bool TelemetryConfig::isSerialEnabled() const {
    return serialEnabled_;
}

const char* TelemetryConfig::getTopicPrefix() const {
    return topicPrefix_;
}

void TelemetryConfig::setPublishIntervalMs(uint32_t interval) {
    publishIntervalMs_ = interval;
}

void TelemetryConfig::setRetryIntervalMs(uint32_t interval) {
    retryIntervalMs_ = interval;
}

void TelemetryConfig::setMaxRetries(uint32_t retries) {
    maxRetries_ = retries;
}

void TelemetryConfig::setEnabled(bool enabled) {
    enabled_ = enabled;
}

void TelemetryConfig::setMqttEnabled(bool enabled) {
    mqttEnabled_ = enabled;
}

void TelemetryConfig::setSerialEnabled(bool enabled) {
    serialEnabled_ = enabled;
}

void TelemetryConfig::setTopicPrefix(const char* prefix) {
    strncpy(topicPrefix_, prefix, sizeof(topicPrefix_) - 1);
    topicPrefix_[sizeof(topicPrefix_) - 1] = '\0';
}

} // namespace telemetry
