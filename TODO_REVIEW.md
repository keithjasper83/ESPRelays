<!--
SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 Keith Jasper
Contact: https://github.com/keithjasper83/ESPRelays/issues
-->

# Engineering To-Do Review Backlog

This is an intentionally nitpick-heavy self-review list to track quality, reliability,
and security hardening work before external review pressure forces it.

## Priority Guide

- P0: Security or data integrity risk.
- P1: Reliability and correctness risk.
- P2: Maintainability and observability uplift.

## Security And Access Control

- [ ] P0: Add authentication and optional role separation to web control endpoints in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P0: Add CSRF protection for mutating HTTP actions in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P0: Add rate limiting and basic abuse throttling for write endpoints in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P0: Add command allowlist validation for schedule-dispatched commands in [src/ScheduleManager.cpp](src/ScheduleManager.cpp) and dispatch path in [src/main.cpp](src/main.cpp).
- [ ] P1: Add optional MQTT auth/TLS configuration flow and secure defaults in [src/MqttManager.cpp](src/MqttManager.cpp).
- [ ] P1: Add OTA source integrity checks and stricter release verification path in [src/OtaUpdateManager.cpp](src/OtaUpdateManager.cpp).

## Correctness And Behavior

- [ ] P1: Fix wifi_password_set semantics to check whether password is actually set (currently tied to SSID length) in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P1: Escape schedule command strings in JSON output to avoid malformed JSON when commands contain quotes or control chars in [src/ScheduleManager.cpp](src/ScheduleManager.cpp).
- [ ] P1: Add full calendar date validation for one-time events (month/day combinations, leap year) in [src/ScheduleManager.cpp](src/ScheduleManager.cpp) and parser in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P1: Add explicit hostname rule checks (label boundaries and leading/trailing hyphen constraints) in [src/main.cpp](src/main.cpp).
- [ ] P1: Add deterministic behavior spec for retries/backoff timing under Wi-Fi and MQTT outages in [src/WiFiManager.cpp](src/WiFiManager.cpp) and [src/MqttManager.cpp](src/MqttManager.cpp).
- [ ] P2: Add replay-safe schedule execution notes around clock jumps/NTP adjustments in [src/ScheduleManager.cpp](src/ScheduleManager.cpp) and [src/TimeSyncManager.cpp](src/TimeSyncManager.cpp).

## Memory And Performance

- [ ] P1: Audit heavy String concatenation paths that build large JSON/HTML blobs for heap fragmentation risk in [src/WebControlServer.cpp](src/WebControlServer.cpp), [src/MqttManager.cpp](src/MqttManager.cpp), and [src/ScheduleManager.cpp](src/ScheduleManager.cpp).
- [ ] P1: Add upper bounds for HTTP body parsing and command payload sizes in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P1: Set explicit MQTT packet buffer size for worst-case telemetry payload and test it under realistic growth in [src/MqttManager.cpp](src/MqttManager.cpp).
- [ ] P2: Remove blocking delay usage in maintenance/control paths and replace with non-blocking state transitions in [src/WiFiManager.cpp](src/WiFiManager.cpp) and [src/main.cpp](src/main.cpp).
- [ ] P2: Add periodic heap telemetry (free, largest block, low watermark) to diagnostics endpoint in [src/WebControlServer.cpp](src/WebControlServer.cpp).

## Observability And Diagnostics

- [ ] P1: Add structured event codes for connectivity, schedule dispatch, and OTA attempts in [src/main.cpp](src/main.cpp), [src/WiFiManager.cpp](src/WiFiManager.cpp), [src/MqttManager.cpp](src/MqttManager.cpp), and [src/OtaUpdateManager.cpp](src/OtaUpdateManager.cpp).
- [ ] P2: Add a dedicated health endpoint with dependency-level status and retry counters in [src/WebControlServer.cpp](src/WebControlServer.cpp).
- [ ] P2: Add ring-buffered recent event log to support post-failure forensic review in [src/main.cpp](src/main.cpp).

## Test Coverage Gaps

- [ ] P1: Add tests for malformed JSON and escaping edge cases in schedule and config responses under [test](test).
- [ ] P1: Add tests for retry storms and reconnect stability under poor network simulation in [test](test).
- [ ] P1: Add tests for timezone and DST boundary behavior in scheduler execution in [test](test).
- [ ] P2: Add tests for max-capacity schedule CRUD and persistence corruption recovery in [test](test).
- [ ] P2: Add snapshot contract tests for web response schemas to detect accidental API drift in [test/test_http_contracts/test_main.cpp](test/test_http_contracts/test_main.cpp).

## Dependency And Lifecycle

- [ ] P1: Re-evaluate MQTT client dependency strategy because upstream PubSubClient README currently states unmaintained status; decide pin/replace strategy in [platformio.ini](platformio.ini).
- [ ] P2: Add periodic dependency review cadence and changelog note format in [README.md](README.md).

## Product Features To Track (Not Implemented Yet)

- [ ] P1: Auto-off from last-off style function. Keep as planned roadmap feature only; do not implement until behavior spec and safety rules are approved.

## Suggested Internet-Backed References For Review Standards

- ESP-IDF heap instrumentation and fragmentation diagnostics:
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/heap_debug.html
- PubSubClient limitations and maintenance status:
  https://github.com/knolleary/pubsubclient
