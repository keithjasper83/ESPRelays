<!--
SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 Keith Jasper
Contact: https://github.com/keithjasper83/ESPRelays/issues
-->

# Engineering TODO

This backlog tracks hardening, correctness, and maintainability work that is not yet complete.

Priority levels:
- `P0`: security or data-integrity risk
- `P1`: reliability and correctness risk
- `P2`: maintainability and observability uplift

| ID | Priority | Area | Status | Task | Primary Files |
|---|---|---|---|---|---|
| SEC-01 | P0 | Security | Open | Add authentication and optional role separation for web control endpoints. | `src/WebControlServer.cpp` |
| SEC-02 | P0 | Security | Open | Add CSRF protection for mutating HTTP actions. | `src/WebControlServer.cpp` |
| SEC-03 | P0 | Security | Open | Add rate limiting/basic abuse throttling for write endpoints. | `src/WebControlServer.cpp` |
| SEC-04 | P0 | Security | Open | Add command allowlist validation for schedule-dispatched commands. | `src/ScheduleManager.cpp`, `src/main.cpp` |
| SEC-05 | P1 | Security | Partial | MQTT username/password is implemented; add optional MQTT TLS and secure certificate workflow. | `src/MqttManager.cpp`, `src/WebControlServer.cpp` |
| SEC-06 | P1 | Security | Open | Add OTA source integrity checks and stricter release verification path. | `src/OtaUpdateManager.cpp` |
| COR-01 | P1 | Correctness | Open | Fix `wifi_password_set` semantics so it reflects actual stored password state, not SSID presence. | `src/WebControlServer.cpp`, `src/WiFiManager.cpp` |
| COR-02 | P1 | Correctness | Open | Escape schedule command strings in JSON output to prevent malformed JSON on quotes/control chars. | `src/ScheduleManager.cpp` |
| COR-03 | P1 | Correctness | Open | Add full calendar date validation (month/day combos, leap year) for one-time schedules. | `src/ScheduleManager.cpp`, `src/WebControlServer.cpp` |
| COR-04 | P1 | Correctness | Open | Add explicit hostname rule checks (label boundaries, leading/trailing hyphen constraints). | `src/main.cpp` |
| COR-05 | P1 | Correctness | Open | Define deterministic retry/backoff behavior spec for Wi-Fi and MQTT outages. | `src/WiFiManager.cpp`, `src/MqttManager.cpp` |
| COR-06 | P2 | Correctness | Open | Add replay-safe schedule execution rules around clock jumps/NTP adjustments. | `src/ScheduleManager.cpp`, `src/TimeSyncManager.cpp` |
| MEM-01 | P1 | Memory/Perf | Open | Audit heavy `String` concatenation paths for heap-fragmentation risk. | `src/WebControlServer.cpp`, `src/MqttManager.cpp`, `src/ScheduleManager.cpp` |
| MEM-02 | P1 | Memory/Perf | Open | Add upper bounds for HTTP body parsing and command payload sizes. | `src/WebControlServer.cpp` |
| MEM-03 | P1 | Memory/Perf | Open | Set explicit MQTT packet buffer size for worst-case telemetry payload and test growth limits. | `src/MqttManager.cpp` |
| MEM-04 | P2 | Memory/Perf | Open | Remove blocking delays in maintenance/control paths; use non-blocking transitions. | `src/WiFiManager.cpp`, `src/main.cpp` |
| OBS-01 | P1 | Observability | Open | Add structured event codes for connectivity, schedule dispatch, and OTA attempts. | `src/main.cpp`, `src/WiFiManager.cpp`, `src/MqttManager.cpp`, `src/OtaUpdateManager.cpp` |
| OBS-02 | P2 | Observability | Open | Add dedicated health endpoint with dependency-level status and retry counters. | `src/WebControlServer.cpp` |
| OBS-03 | P2 | Observability | Open | Add ring-buffered recent event log for post-failure forensics. | `src/main.cpp` |
| TST-01 | P1 | Testing | Open | Add tests for malformed JSON and escaping edge cases in schedule/config responses. | `test/` |
| TST-02 | P1 | Testing | Open | Add tests for retry storms and reconnect stability under poor-network simulation. | `test/` |
| TST-03 | P1 | Testing | Open | Add tests for timezone and DST boundary behavior in schedule execution. | `test/` |
| TST-04 | P2 | Testing | Open | Add tests for max-capacity schedule CRUD and persistence corruption recovery. | `test/` |
| TST-05 | P2 | Testing | Open | Add snapshot contract tests for web response schemas to detect accidental API drift. | `test/test_http_contracts/test_main.cpp` |
| DEP-01 | P1 | Dependency | Open | Re-evaluate MQTT client dependency strategy due PubSubClient maintenance status; decide pin/replace strategy. | `platformio.ini` |
| DEP-02 | P2 | Dependency | Open | Define periodic dependency review cadence and changelog note format. | `README.md`, `docs/CHANGELOG.md` |
| PRD-01 | P1 | Product | Planned | Auto-off-from-last-off roadmap behavior; do not implement until safety behavior spec is approved. | `README.md`, future design notes |

## References

- ESP-IDF heap instrumentation and diagnostics: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/heap_debug.html
- PubSubClient project and maintenance context: https://github.com/knolleary/pubsubclient
