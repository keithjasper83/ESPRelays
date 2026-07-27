# Telemetry Legacy Audit

## Overview
This document provides a factual inventory of the legacy telemetry implementation in `src/telemetry.cpp` to support the migration to a modular telemetry subsystem.

## Function Inventory

### Public Interface Functions
- `begin()` - Initialize telemetry subsystem
- `setMqttClient(MQTTClient* client)` - Set MQTT client reference
- `addRelay(uint8_t relayId, const char* name)` - Register a relay
- `addTemperatureProbe(uint8_t probeId, const char* name)` - Register a temperature probe
- `addLedStrip(uint8_t stripId, const char* name)` - Register an LED strip
- `setRelayState(uint8_t relayId, bool state)` - Update relay state
- `setTemperature(uint8_t probeId, float temp)` - Update temperature reading
- `setLedStripState(uint8_t stripId, bool state)` - Update LED strip state
- `setControlState(ControlState state)` - Set control state
- `addWarning(const char* warning)` - Add a warning message
- `clearWarning()` - Clear warning messages
- `recordEvent(const char* event)` - Record an event
- `recordNtpSync(bool success)` - Record NTP sync status
- `publishDiscovery()` - Publish discovery message
- `publishHeartbeat()` - Publish heartbeat message
- `publishStatus()` - Publish status message
- `printStatus()` - Print status to serial
- `requestImmediateReport()` - Request immediate telemetry report
- `loop()` - Main telemetry loop
- `scheduleSerialPrint(uint32_t delayMs)` - Schedule serial print
- `scheduleHeartbeat(uint32_t delayMs)` - Schedule heartbeat
- `scheduleStatusPublish(uint32_t delayMs)` - Schedule status publish
- `scheduleDiscovery()` - Schedule discovery publish
- `handleMqttReconnect()` - Handle MQTT reconnection

### Internal/Helper Functions
- `buildTopic(const char* suffix)` - Build MQTT topic
- `buildDiscoveryPayload()` - Build discovery payload
- `buildHeartbeatPayload()` - Build heartbeat payload
- `buildStatusPayload()` - Build status payload
- `buildSerialOutput()` - Build serial output string

## Dependency Inventory

### External Dependencies
- **MQTTClient** - For MQTT publishing
- **WiFi** - For WiFi connection status and signal strength
- **NTPClient** - For time synchronization
- **RelayController** - For relay state management
- **TemperatureController** - For temperature probe management
- **LedStripController** - For LED strip management
- **ControlStateManager** - For control state management

### Global Variables
- `MQTTClient* _mqttClient` - MQTT client reference
- `std::vector<RelayInfo> _relays` - Registered relays
- `std::vector<TemperatureProbeInfo> _temperatureProbes` - Registered temperature probes
- `std::vector<LedStripInfo> _ledStrips` - Registered LED strips
- `std::vector<const char*> _warnings` - Active warning messages
- `std::vector<const char*> _events` - Recorded events
- `ControlState _controlState` - Current control state
- `bool _ntpSynced` - NTP sync status
- `uint32_t _lastNtpSyncAttempt` - Last NTP sync attempt timestamp

## Topic Inventory

### MQTT Topics
- `{devicePrefix}/telemetry/discovery` - Discovery topic
- `{devicePrefix}/telemetry/heartbeat` - Heartbeat topic
- `{devicePrefix}/telemetry/status` - Status topic

## Payload Inventory

### Discovery Payload
```json
{
  "device": "ESPRelays",
  "version": "1.0",
  "relays": [
    {"id": <relayId>, "name": "<relayName>"}
  ],
  "temperatureProbes": [
    {"id": <probeId>, "name": "<probeName>"}
  ],
  "ledStrips": [
    {"id": <stripId>, "name": "<stripName>"}
  ]
}
```

### Heartbeat Payload
```json
{
  "timestamp": <unixTimestamp>,
  "uptime": <uptimeSeconds>,
  "wifiConnected": <bool>,
  "signalStrength": <dBm>,
  "ipAddress": "<IP>"
}
```

### Status Payload
```json
{
  "timestamp": <unixTimestamp>,
  "controlState": "<state>",
  "relays": [
    {"id": <relayId>, "state": <bool>}
  ],
  "temperatureProbes": [
    {"id": <probeId>, "value": <temp>}
  ],
  "ledStrips": [
    {"id": <stripId>, "state": <bool>}
  ],
  "warnings": [<warning1>, <warning2>],
  "events": [<event1>, <event2>]
}
```

### Serial Output
Plain text format with key-value pairs separated by newlines.

## State Inventory

### Telemetry State
- **Control State** - Current operational state (NORMAL, EMERGENCY_STOP, MAINTENANCE)
- **WiFi Connection** - Current WiFi connection status
- **Signal Strength** - Current WiFi signal strength in dBm
- **IP Address** - Current IP address
- **Relay States** - State of each registered relay
- **Temperature Readings** - Current temperature from each probe
- **LED Strip States** - State of each registered LED strip
- **Warnings** - Active warning messages
- **Events** - Recorded events
- **NTP Sync Status** - Whether NTP is synchronized

### Timing State
- **Last NTP Sync Attempt** - Timestamp of last NTP sync attempt
- **Scheduled Tasks** - Timers for periodic telemetry publishing

## Target Module Mapping

| Legacy Responsibility | Target Module |
|----------------------|---------------|
| MQTT topic construction | telemetry_topics.h/cpp |
| MQTT payload encoding | telemetry_encoder.h/cpp |
| MQTT publishing | telemetry_transport.h/cpp |
| Telemetry scheduling | telemetry_scheduler.h/cpp |
| Data sampling from sensors | telemetry_sampler.h/cpp |
| Telemetry data structure | telemetry_snapshot.h |
| Telemetry buffer | telemetry_buffer.h/cpp |
| Statistics and counters | telemetry_stats.h/cpp |
| Configuration management | telemetry_config.h/cpp |
| Diagnostics | telemetry_diagnostics.h/cpp |
| Subsystem orchestration | telemetry_manager.h/cpp |

## Unresolved Questions

1. What is the current device prefix used for MQTT topics?
2. What are the current telemetry intervals (heartbeat, status, discovery)?
3. Are there any specific MQTT QoS or retain settings that need to be preserved?
4. What is the maximum expected number of relays, temperature probes, and LED strips?
5. Are there any specific constraints on payload size?

## Migration Risks

1. **MQTT Topic Compatibility** - Ensuring new topic construction matches existing topics exactly
2. **Payload Format Preservation** - Maintaining backward compatibility with existing payload formats
3. **Timing Precision** - Preserving the exact timing intervals for telemetry publishing
4. **State Consistency** - Ensuring all state is properly synchronized between legacy and new implementation
5. **Error Handling** - Maintaining the same error handling behavior during MQTT disconnections
6. **Memory Usage** - Managing memory usage with bounded data structures suitable for ESP32
7. **Thread Safety** - Ensuring thread-safe access to shared state in the new modular implementation
