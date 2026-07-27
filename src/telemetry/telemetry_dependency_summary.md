# Telemetry Module Dependency Summary

## Module Responsibilities

### Core Modules

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_config** | Configuration values only (intervals, flags, prefixes) | None |
| **telemetry_snapshot** | Plain data structures for telemetry data | None |
| **telemetry_topics** | Topic structure definitions | None |

### Data Management

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_buffer** | Queueing for pending telemetry | telemetry_snapshot |
| **telemetry_stats** | Counters only (packets sent, dropped, retries) | None |

### Diagnostics

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_diagnostics** | Diagnostic information generation | telemetry_snapshot, telemetry_buffer |

### Scheduling & Sampling

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_scheduler** | Timing only (when to run) | telemetry_config |
| **telemetry_sampler** | Collect data from ESPRelays | None |

### Processing & Transport

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_encoder** | JSON generation | telemetry_snapshot |
| **telemetry_transport** | MQTT publish, retry, reconnect | telemetry_buffer, telemetry_stats |

### Orchestration

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **telemetry_manager** | Subsystem orchestration | All other modules |

## Dependency Flow

```
telemetry_manager
    ↓
telemetry_scheduler → telemetry_config
    ↓
telemetry_sampler
    ↓
telemetry_snapshot
    ↓
telemetry_encoder
    ↓
telemetry_transport → telemetry_buffer → telemetry_stats
    ↓
telemetry_diagnostics → telemetry_snapshot, telemetry_buffer
```

## Key Design Principles

1. **Single Responsibility**: Each module has exactly one responsibility
2. **No Circular Dependencies**: Dependencies flow in one direction only
3. **Small Files**: All .cpp files < 300 lines, all .h files < 200 lines
4. **No Duplication**: Each piece of logic lives in exactly one module
5. **Public API Stability**: Manager exposes only stable, small interfaces
6. **No Global State**: All state is encapsulated within modules

## Compilation Notes

- All modules can compile independently
- Include guards prevent multiple inclusion
- Namespace `telemetry` prevents symbol conflicts
- Empty implementations allow incremental development

## Next Steps

When implementing telemetry logic:
1. Start with `telemetry_config` - it has no dependencies
2. Implement `telemetry_snapshot` - plain data structures
3. Add `telemetry_buffer` and `telemetry_stats` - simple data management
4. Implement `telemetry_scheduler` - timing logic
5. Add `telemetry_sampler` - data collection
6. Implement `telemetry_encoder` - JSON generation
7. Add `telemetry_transport` - MQTT publishing
8. Finally, wire everything together in `telemetry_manager`
