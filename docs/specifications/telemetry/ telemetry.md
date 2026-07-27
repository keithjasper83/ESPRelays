# ESPRelays Telemetry Specification

## Document Control

- Specification ID: `TELEMETRY-SPEC-001`
- Project: `ESPRelays`
- Specification path: `docs/specifications/telemetry/telemetry_spec.md`
- Implementation directory: `src/telemetry/`
- Existing legacy implementation: `src/telemetry.cpp`
- Status: Approved for incremental implementation

---

# 1. Objective

Replace the oversized legacy telemetry implementation with a modular telemetry subsystem.

The new subsystem must preserve required telemetry behaviour while splitting responsibilities into small, maintainable files.

Implementation must happen one task at a time.

The agent must not attempt to complete the entire specification in one run.

---

# 2. Mandatory Size Limits

These limits apply throughout the implementation:

- Maximum `.cpp` file size: 300 lines
- Maximum `.h` file size: 200 lines
- Maximum function size: 50 lines
- Maximum class responsibility: one cohesive responsibility
- No new general-purpose utility files
- No circular dependencies
- No duplicated telemetry logic
- No unbounded queues
- No large full-file rewrites unless explicitly required by a task

When a file approaches a limit, split the responsibility before continuing.

---

# 3. Required Directory

All new telemetry implementation files belong under:

```text
src/telemetry/
```

The legacy file remains at:

```text
src/telemetry.cpp
```

Do not delete the legacy file until a later migration task explicitly permits it.

---

# 4. Required Modules

The telemetry subsystem consists of:

```text
src/telemetry/
    telemetry_manager.h
    telemetry_manager.cpp

    telemetry_scheduler.h
    telemetry_scheduler.cpp

    telemetry_sampler.h
    telemetry_sampler.cpp

    telemetry_snapshot.h

    telemetry_encoder.h
    telemetry_encoder.cpp

    telemetry_transport.h
    telemetry_transport.cpp

    telemetry_buffer.h
    telemetry_buffer.cpp

    telemetry_topics.h
    telemetry_topics.cpp

    telemetry_config.h
    telemetry_config.cpp

    telemetry_stats.h
    telemetry_stats.cpp

    telemetry_diagnostics.h
    telemetry_diagnostics.cpp
```

---

# 5. Responsibility Map

## 5.1 TelemetryManager

Responsible for orchestration only.

It may:

- determine whether telemetry processing is due
- request a snapshot from the sampler
- pass the snapshot to the encoder
- pass encoded messages to the transport
- update telemetry statistics
- expose high-level telemetry lifecycle methods

It must not:

- construct JSON directly
- publish MQTT messages directly
- read sensors directly
- build MQTT topics directly
- own detailed retry logic
- contain large business-logic branches

---

## 5.2 TelemetryScheduler

Responsible only for timing decisions.

It may:

- track the last execution time
- determine whether a configured interval has elapsed
- reset or update timing state
- handle `millis()` wraparound safely

It must not:

- read sensors
- create payloads
- publish MQTT messages
- inspect relay state
- own telemetry statistics

---

## 5.3 TelemetrySampler

Responsible only for collecting telemetry source values.

It may collect:

- uptime
- Wi-Fi state
- signal strength
- relay state
- temperature data
- system state
- memory information
- firmware information
- error state
- other telemetry fields already required by the legacy implementation

It must return a `TelemetrySnapshot`.

It must not:

- construct JSON
- publish MQTT
- determine publish intervals
- queue messages

---

## 5.4 TelemetrySnapshot

Contains plain telemetry data structures only.

It may contain:

- primitive values
- fixed-size strings
- enums
- nested plain structures
- validity flags
- timestamps or uptime values

It must not contain:

- MQTT operations
- JSON encoding
- sensor reads
- timing decisions
- queue operations
- substantial behaviour

---

## 5.5 TelemetryEncoder

Responsible only for converting telemetry data into outbound payloads.

It may:

- create JSON payloads
- create compact diagnostic payloads
- encode values consistently
- omit invalid or disabled fields
- enforce payload size limits
- report encoding failure

It must not:

- publish MQTT
- read sensors
- decide when telemetry is due
- own reconnect logic

---

## 5.6 TelemetryTransport

Responsible only for delivery of encoded telemetry messages.

It may:

- publish through the existing MQTT client
- detect whether MQTT is connected
- retry queued messages
- flush pending messages
- update delivery statistics
- report publish success or failure

It must not:

- construct JSON
- read telemetry values
- decide sampling intervals
- own application configuration unrelated to transport

---

## 5.7 TelemetryBuffer

Responsible only for bounded storage of pending telemetry messages.

It must:

- have a fixed maximum capacity
- reject or discard messages predictably when full
- expose queue depth
- support push, peek and pop operations
- avoid dynamic unbounded growth

It must not:

- publish messages
- generate payloads
- read sensors
- own MQTT connectivity

---

## 5.8 TelemetryTopics

Responsible only for telemetry MQTT topic construction.

It may:

- store topic suffix constants
- build full topics from the configured device prefix
- validate topic lengths
- expose stable topic names

It must not:

- publish messages
- construct JSON
- sample telemetry values

---

## 5.9 TelemetryConfig

Responsible for telemetry configuration values.

Configuration may include:

- enabled state
- publish interval
- diagnostic interval
- queue capacity
- maximum payload size
- enabled telemetry groups
- MQTT topic prefix
- retry behaviour

It must not:

- publish messages
- sample values
- encode JSON
- contain orchestration logic

---

## 5.10 TelemetryStats

Responsible only for telemetry counters and timestamps.

It may track:

- snapshots collected
- payloads encoded
- encoding failures
- messages queued
- messages published
- publish failures
- messages dropped
- retries
- last successful publish
- last failed publish

It must not:

- publish messages
- create payloads
- sample sensors

---

## 5.11 TelemetryDiagnostics

Responsible only for summarising telemetry subsystem health.

It may expose:

- queue depth
- publish counters
- failure counters
- last success time
- last failure time
- scheduler state
- enabled state

It must not:

- publish MQTT messages
- read hardware sensors
- control relay behaviour

---

# 6. Dependency Rules

Preferred dependency direction:

```text
TelemetryManager
    -> TelemetryScheduler
    -> TelemetrySampler
    -> TelemetryEncoder
    -> TelemetryTransport
    -> TelemetryDiagnostics

TelemetrySampler
    -> TelemetrySnapshot
    -> TelemetryConfig

TelemetryEncoder
    -> TelemetrySnapshot
    -> TelemetryConfig

TelemetryTransport
    -> TelemetryBuffer
    -> TelemetryTopics
    -> TelemetryStats
    -> TelemetryConfig

TelemetryDiagnostics
    -> TelemetryStats
    -> TelemetryBuffer
```

Lower-level modules must not depend on `TelemetryManager`.

Circular dependencies are forbidden.

Prefer forward declarations in headers where practical.

---

# 7. Behaviour Preservation

The legacy implementation in:

```text
src/telemetry.cpp
```

is the reference for existing behaviour.

Before moving any behaviour, identify:

- public telemetry functions
- callers of those functions
- MQTT topics currently used
- payload formats currently used
- publish intervals
- configuration sources
- telemetry fields
- retry behaviour
- diagnostics
- dependencies on global variables
- dependencies on other project modules

Do not silently change externally visible behaviour.

Any intentional behaviour change must be explicitly required by a task.

---

# 8. Build and Test Rules

After every implementation task:

1. Save all modified files.
2. Run the normal project build.
3. Run relevant tests if they exist.
4. Check compiler warnings.
5. Confirm file and function size limits.
6. Stop after reporting results.

Do not continue to the next task automatically.

The expected build command should be discovered from the repository configuration, such as `platformio.ini`, project scripts or existing documentation.

Do not invent a build command without checking the repository.

---

# 9. Task Index

The dispatcher must use this table as the persistent task list.

| Task ID | Heading | Status |
|---|---|---|
| `TEL-001` | Audit the legacy telemetry implementation | COMPLETE |
| `TEL-002` | Validate the generated telemetry skeleton | TODO |
| `TEL-003` | Define telemetry configuration | TODO |
| `TEL-004` | Define telemetry snapshot data | TODO |
| `TEL-005` | Implement telemetry statistics | TODO |
| `TEL-006` | Implement the bounded telemetry buffer | TODO |
| `TEL-007` | Implement telemetry topic construction | TODO |
| `TEL-008` | Implement telemetry scheduling | TODO |
| `TEL-009` | Implement telemetry sampling | TODO |
| `TEL-010` | Implement telemetry encoding | TODO |
| `TEL-011` | Implement telemetry transport | TODO |
| `TEL-012` | Implement telemetry diagnostics | TODO |
| `TEL-013` | Implement telemetry manager orchestration | TODO |
| `TEL-014` | Integrate the modular subsystem with existing callers | TODO |
| `TEL-015` | Remove migrated logic from the legacy file | TODO |
| `TEL-016` | Final build, regression review and documentation | TODO |

Only the status values below are valid:

- `TODO`
- `IN_PROGRESS`
- `BLOCKED`
- `COMPLETE`

---

# 10. Task Definitions

## TEL-001 — Audit the Legacy Telemetry Implementation

### Objective

Produce a factual inventory of `src/telemetry.cpp` before moving any implementation.

### Actions

- Read the complete legacy telemetry file in sensible sections.
- Do not repeatedly request the same line range.
- Identify every function.
- Identify every externally visible function.
- Identify every global or static variable.
- Identify all MQTT topics.
- Identify all payload formats.
- Identify all telemetry fields.
- Identify all timing logic.
- Identify all retry and buffering logic.
- Identify all diagnostics.
- Identify all external dependencies.
- Identify all callers elsewhere in the repository.
- Map each responsibility to the new target module.

### Deliverable

Create:

```text
docs/specifications/telemetry/telemetry_legacy_audit.md
```

The audit must include:

- function inventory
- dependency inventory
- topic inventory
- payload inventory
- state inventory
- target-module mapping
- unresolved questions
- migration risks

### Restrictions

- Do not modify production code.
- Do not implement telemetry.
- Do not delete anything.
- Stop after the audit is complete.

### Completion Criteria

- Audit document exists.
- Every legacy function is accounted for.
- Every externally visible dependency is identified.
- Project source is unchanged.

---

## TEL-002 — Validate the Generated Telemetry Skeleton

### Objective

Check that the generated files form a valid, minimal architecture skeleton.

### Actions

- Inspect every file under `src/telemetry/`.
- Confirm declarations match definitions.
- Confirm include guards or `#pragma once`.
- Remove unnecessary invented APIs.
- Remove circular dependencies.
- Confirm namespace usage is consistent.
- Confirm the files compile as part of the project.
- Keep implementations empty or minimal.

### Restrictions

- Do not migrate legacy behaviour.
- Do not implement feature logic.
- Do not modify `src/telemetry.cpp`.

### Completion Criteria

- All skeleton files compile.
- No circular dependencies exist.
- No file exceeds the size limits.
- No unnecessary behaviour has been introduced.

---

## TEL-003 — Define Telemetry Configuration

### Objective

Implement the telemetry configuration data model.

### Inputs

- `telemetry_legacy_audit.md`
- existing project configuration patterns
- current telemetry intervals and enable flags

### Actions

- Define configuration fields required by existing behaviour.
- Choose safe defaults matching current behaviour.
- Add validation for invalid intervals and sizes.
- Avoid owning runtime orchestration.
- Add focused unit tests if the project supports them.

### Restrictions

- Do not publish MQTT.
- Do not sample sensors.
- Do not migrate unrelated configuration.

### Completion Criteria

- Configuration compiles.
- Defaults preserve existing behaviour.
- Invalid values are handled predictably.
- Relevant tests pass.

---

## TEL-004 — Define Telemetry Snapshot Data

### Objective

Define the plain data structure containing one telemetry sample.

### Inputs

- legacy audit telemetry field inventory
- existing sensor and relay interfaces

### Actions

- Add only fields required by existing telemetry behaviour.
- Include validity indicators where values may be unavailable.
- Prefer fixed-size or bounded storage suitable for ESP32.
- Avoid large dynamic allocations.
- Keep the structure free of transport and encoding behaviour.

### Restrictions

- No MQTT.
- No JSON.
- No direct sensor reads.
- No scheduling.

### Completion Criteria

- Snapshot model compiles.
- All legacy telemetry fields are represented or explicitly documented as excluded.
- Data structure remains cohesive and bounded.

---

## TEL-005 — Implement Telemetry Statistics

### Objective

Implement telemetry counters and timestamps.

### Actions

- Implement required counters.
- Provide focused increment and update methods.
- Provide read-only accessors or snapshots.
- Handle counter types safely.
- Keep all methods small.

### Completion Criteria

- Statistics compile.
- Counters can be updated and inspected.
- No transport or encoding logic is present.
- Tests pass where supported.

---

## TEL-006 — Implement the Bounded Telemetry Buffer

### Objective

Implement fixed-capacity storage for pending telemetry messages.

### Actions

- Determine payload representation from the encoder and transport requirements.
- Implement bounded push, peek and pop behaviour.
- Define overflow behaviour explicitly.
- Expose queue depth and capacity.
- Avoid unbounded dynamic allocation.
- Record dropped-message statistics through an appropriate interface if required.

### Restrictions

- No MQTT calls.
- No payload construction.
- No sensor access.

### Completion Criteria

- Buffer behaviour is deterministic.
- Capacity cannot grow without limit.
- Overflow behaviour is tested or demonstrably verified.
- File limits are respected.

---

## TEL-007 — Implement Telemetry Topic Construction

### Objective

Implement stable MQTT telemetry topic generation.

### Inputs

- topic inventory from the legacy audit
- existing project MQTT conventions
- configured device prefix

### Actions

- Define named topic suffixes.
- Build full topics safely.
- Preserve existing externally visible topic names.
- Enforce topic length constraints.
- Avoid duplicated string-building logic.

### Restrictions

- Do not publish MQTT.
- Do not encode telemetry payloads.

### Completion Criteria

- All existing telemetry topics can be produced.
- Topic output matches current behaviour.
- Invalid or oversized topics fail predictably.

---

## TEL-008 — Implement Telemetry Scheduling

### Objective

Implement interval-based telemetry timing.

### Actions

- Preserve existing telemetry intervals.
- Use wraparound-safe elapsed-time comparisons.
- Keep scheduling independent from transport and sampling.
- Support reset and configuration updates if required by existing behaviour.

### Restrictions

- No sensor reads.
- No MQTT.
- No JSON.

### Completion Criteria

- Scheduler determines due and not-due states correctly.
- `millis()` wraparound is handled.
- Relevant tests pass or behaviour is otherwise verified.

---

## TEL-009 — Implement Telemetry Sampling

### Objective

Move telemetry data collection into `TelemetrySampler`.

### Inputs

- legacy audit
- telemetry snapshot
- existing relay, Wi-Fi, temperature and system interfaces

### Actions

- Collect one complete snapshot.
- Preserve existing field meanings.
- Handle unavailable data without crashing.
- Avoid encoding or transport logic.
- Keep hardware-specific reads behind existing project interfaces where possible.

### Restrictions

- No JSON creation.
- No MQTT publishing.
- No queue operations.
- No scheduling decisions.

### Completion Criteria

- Snapshot population matches existing telemetry fields.
- Missing values are represented safely.
- Project builds and relevant tests pass.

---

## TEL-010 — Implement Telemetry Encoding

### Objective

Move telemetry payload generation into `TelemetryEncoder`.

### Inputs

- legacy payload inventory
- telemetry snapshot
- telemetry configuration

### Actions

- Preserve current payload field names and value meanings.
- Preserve required JSON structure.
- Enforce payload size limits.
- Return explicit success or failure.
- Avoid repeated heap-heavy string concatenation where practical.
- Keep separate payload types in separate small functions.

### Restrictions

- No MQTT publishing.
- No sensor reads.
- No scheduling.

### Completion Criteria

- Encoded payloads match required legacy behaviour.
- Oversized or invalid payloads fail predictably.
- Functions remain below 50 lines.
- Relevant tests or fixture comparisons pass.

---

## TEL-011 — Implement Telemetry Transport

### Objective

Move MQTT delivery, retry and queue flushing into `TelemetryTransport`.

### Inputs

- existing MQTT client interface
- telemetry buffer
- telemetry topics
- telemetry statistics

### Actions

- Publish encoded telemetry through the existing MQTT interface.
- Detect disconnected state.
- Queue messages when required.
- Flush queued messages when possible.
- Update statistics on success, failure, retry and drop.
- Preserve existing publish semantics.

### Restrictions

- No sensor reads.
- No JSON construction.
- No scheduling decisions.

### Completion Criteria

- Connected publish succeeds.
- Disconnected behaviour is deterministic.
- Queue flushing works.
- Statistics are updated correctly.
- No unbounded retry loop exists.

---

## TEL-012 — Implement Telemetry Diagnostics

### Objective

Expose concise telemetry subsystem diagnostics.

### Actions

- Report enabled state.
- Report queue state.
- Report publish statistics.
- Report last success and failure.
- Report scheduler state if required.
- Preserve existing diagnostics that callers depend upon.

### Restrictions

- No MQTT publishing.
- No sampling.
- No transport ownership.

### Completion Criteria

- Diagnostics compile.
- Existing required diagnostic data is represented.
- No unrelated logic is introduced.

---

## TEL-013 — Implement Telemetry Manager Orchestration

### Objective

Implement the high-level telemetry workflow.

### Expected Flow

```text
check schedule
    -> collect snapshot
    -> encode payload or payloads
    -> submit to transport
    -> update relevant state
```

### Actions

- Wire together existing modules.
- Keep methods short.
- Keep detailed logic in owned modules.
- Preserve externally visible lifecycle behaviour.
- Do not introduce hidden background loops.

### Completion Criteria

- Manager compiles.
- Manager contains orchestration rather than detailed business logic.
- Existing expected telemetry workflow can be executed.
- Project builds and tests pass.

---

## TEL-014 — Integrate the Modular Subsystem With Existing Callers

### Objective

Switch existing project callers to the new telemetry subsystem.

### Actions

- Use the caller inventory from the legacy audit.
- Update includes.
- Update initialisation.
- Update periodic calls.
- Update configuration wiring.
- Preserve public behaviour.
- Use compatibility wrappers temporarily where that reduces migration risk.

### Restrictions

- Do not delete the legacy implementation yet.
- Do not change unrelated application behaviour.
- Do not perform broad repository rewrites.

### Completion Criteria

- Existing callers use the modular subsystem.
- Project compiles.
- Firmware behaviour remains consistent.
- No duplicate telemetry execution occurs.

---

## TEL-015 — Remove Migrated Logic From the Legacy File

### Objective

Remove telemetry logic that has been successfully migrated.

### Preconditions

- Tasks `TEL-001` through `TEL-014` are complete.
- The modular subsystem is integrated.
- The project builds.
- Relevant tests pass.
- No active caller depends on the legacy implementation.

### Actions

- Remove migrated code from `src/telemetry.cpp`.
- Retain a compatibility wrapper only if genuinely required.
- Remove dead declarations.
- Remove duplicate globals.
- Update build references.
- Delete the legacy file only when no longer required.

### Restrictions

- Do not remove code based on assumption.
- Confirm references before deletion.

### Completion Criteria

- No duplicate implementation remains.
- No unresolved references remain.
- Project builds and tests pass.
- Legacy file is removed or reduced to a justified compatibility layer.

---

## TEL-016 — Final Build, Regression Review and Documentation

### Objective

Verify the completed subsystem.

### Actions

- Run a clean project build.
- Run all available tests.
- Review warnings.
- Verify file sizes.
- Verify function sizes.
- Verify dependency direction.
- Verify no circular dependencies.
- Verify legacy MQTT topics and payloads.
- Verify retry and buffer behaviour.
- Update telemetry architecture documentation.
- Update the task index statuses.

### Deliverable

Update:

```text
docs/specifications/telemetry/telemetry_dependency_summary.md
```

Include:

- final module map
- final dependencies
- configuration summary
- behaviour compatibility notes
- test results
- known limitations

### Completion Criteria

- Clean build succeeds.
- Relevant tests pass.
- Every task is marked `COMPLETE`.
- No file or function violates the limits.
- Documentation matches the final implementation.

---

# 11. Dispatcher State Rules

The task table in Section 9 is the authoritative task list.

At the start of every run:

1. Read Section 9.
2. Select the first task marked `TODO` whose prerequisites are complete.
3. Change that task to `IN_PROGRESS`.
4. Execute only that task.
5. On success, change it to `COMPLETE`.
6. On a genuine blocker, change it to `BLOCKED`.
7. Save the specification.
8. Stop.

Never mark a task complete before its completion criteria are met.

Never work on more than one task per run.

---

# 12. Agent Loop Protection

The agent must stop immediately when any of these occur:

- the same exact file range is requested three times without a file change
- the same tool call fails three times
- the same build failure remains after three attempted fixes
- the required file does not match the specification
- context compaction causes loss of the current task
- the agent cannot determine whether a task is complete
- completing the task would require starting another task

When stopping, report the blocker clearly instead of continuing blindly.

---

# 13. Required End-of-Run Report

At the end of each run output:

```text
Specification:
Current task:
Task status:
Files read:
Files created:
Files modified:
Build command:
Build result:
Tests run:
Test result:
Size-limit check:
Blockers:
Next task:
```

Do not claim success without build or test evidence where the task requires it.

Do not start the next task.