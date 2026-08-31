# MQTT and disconnected LED retirement

Implemented 2026-08-31 on `codex/mdns-manifest`, following baseline commit
`009d58659fc8f7e8a4a84612d09b4e57529d345f`. This is an unflashed 4.2.0 development
build, not a published release or a claim of hardware validation.

## Removed and retained

Removed `MqttManager`, PubSubClient, MQTT/Matterbridge command and telemetry
paths, both experimental telemetry implementations, `IndicatorLeds`, NeoPixel,
all corresponding web controls/settings/status fields/routes, and unused
`components/mdns`. There is no runtime flag or alternate profile that enables
these features again.

Retained direct HTTP relay control, buttons, Wi-Fi and hostname handling,
Unified Server registration/heartbeats/state/commands, temperature sampling and
calibration, persisted relay state/auto-off, schedules/NTP, OTA, legacy UDP
discovery, and SDK mDNS. No GPIO reassignment, partition change, ownership or
identity change was made. SDK `libmdns.a` plus `libESPmDNS.a` still contributes
29,503 linked bytes; it advertises both `_http._tcp` and `_kj-esp._tcp`.

Both supported PlatformIO environments (`esp32-c3-devkitm-1` and host-test
`native`) use this source tree. CI and release build the device environment and
run native, partition/build-contract and embedded-console tests. Generated
ESP-IDF-only CMake stubs did not define a usable Arduino build; those stubs were
removed rather than retaining a broken alternate build or fetching another
vendored mDNS component. Historical worktrees and other firmware projects were
not rewritten.

## Upgrade implications

- MQTT subscribers, commands, retained publications and Matterbridge integration
  cease after flashing. No broker messages or retained records are cleared by
  this change; the firmware no longer connects to a broker.
- Removed LED endpoints (`/led/*`, `/test/relay-led`, `/test/wifi-led`,
  `/test/leds`) return the existing 404 response. MQTT/LED settings and status
  fields, including `mqtt_client_id`, are no longer emitted. Retired `/config`
  fields are ignored; a request containing only these fields returns the current
  config without saving or restarting, as with any unknown-only config request.
- Manifest revision changes from `relay-2` to `relay-3`; protocol/schema remain
  version 1. The `indicators` group, its three LED children, and `mqtt-connected`
  disappear. Surviving keys, nesting and hardware/legacy identities are unchanged.
  Clients should replace cached capability trees on refresh, not accumulate nodes.
- Existing `mqtt_cfg` and `led_cfg` NVS namespaces remain inert on upgrade. The
  existing explicit factory-reset path still clears those historical namespaces.
  No Wi-Fi, relay, hostname, temperature, schedule or Unified settings are erased.
- Serial MQTT telemetry heartbeat/status dumps disappear with that experiment.
  Existing Unified Server WebSocket heartbeats/state reporting remain unchanged.

## Measured storage

Same ESP32-C3 target, toolchain and application partition layout. Measurements
are file lengths, excluding filesystem block rounding. Source includes `src`,
`include`, `lib` and `components`, not Git history, docs or caches.

| Item | Before (bytes) | After (bytes) | Reduction (bytes) |
|---|---:|---:|---:|
| Actual OTA application image | 1,309,648 | 1,247,472 | 62,176 |
| Source/library/component files | 1,387,546 | 280,940 | 1,106,606 |
| Production build directory | 74,686,547 | 67,358,760 | 7,327,787 |
| Project production dependency cache | 1,287,081 | 952,699 | 334,382 |
| ELF debug/build artifact (inside build directory) | 18,513,320 | 17,274,044 | 1,239,276 |

The 1,310,720-byte application slot now has **63,248 bytes free**, versus
1,072 before. ELF is not uploaded; do not add its savings to the build-directory
savings. Zero linked-byte contributions from vendored mDNS and unused alternate
telemetry mean their deletion mostly saves source storage, not device flash.
Build artifacts and source checkout savings are independent of unchanged Git
history and global toolchain caches.

## Verification

### Combined main integration (2026-08-31)

The subsequent merge also preserves `codex/relay-recovery`: reset diagnostics,
brownout-only relay OFF recovery, and verified A/B calibration persistence.
The combined ESP32-C3 image measures **1,251,600 bytes**, leaving **59,120 bytes**
in the unchanged 1,310,720-byte OTA slot. This is 58,048 bytes smaller than the
pre-cleanup image above. The native suite now contains 67 passing tests.
These are build and host-test measurements; no controller was flashed.

### Cleanup-only verification

- Clean device build plus actual binary/partition guard; no upload.
- 41 native C++ tests; six Python build/partition checks; five embedded-console
  Node tests. Command router/device-command line coverage: 90.8% (118/130).
- Regression tests verify surviving controls/settings, no retired HTML controls,
  and capability removals with unchanged identities and nesting.
- Unified Server fixtures retain `relay-2` and add actual host-produced `relay-3`;
  109 Python tests and 20 Swift tests pass, including old/new manifest decoding.
- Swift runs in the existing Linux Swift 6.0.3 image. Apple Bonjour APIs and
  physical controller behavior still require separate hardware validation.
- Independent source review found no functional regression; whitespace issues
  identified by review were corrected. No live controller writes, OTA, NVS reset,
  server restart, publish or push occurred.
