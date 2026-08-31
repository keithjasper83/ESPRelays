# KjUnifiedManifest

Small reusable PlatformIO library for the read-only `kj-esp-unified` v1 manifest.
Depends only on ArduinoJson for the portable registry/serializer. It introduces
no persistent storage, GPIO setup, command dispatch or device enrollment.

Populate `Manifest::document` with the root protocol, hardware, firmware and
name fields, then register recursive nodes with `Manifest::add(array, key,
type, label, access, available)`. Children can belong to any node type. Keys
are caller-owned stable identifiers: do not derive them from labels, tree paths
or mutable hostnames. Add state, bounds, units, enum values, semantic command
names and metadata through the returned ArduinoJson object.

`serialize(body, error)` validates the complete snapshot before emitting JSON:
64 KiB, 128 globally unique keys, eight capability levels, 32 total JSON
container levels, finite numbers, valid bounds, and string command/enum arrays.
Rebuild state on every request. `manifest_revision` belongs to the application;
bump it when its capability structure or supported semantics change.

After the existing application's successful `MDNS.begin`, call
`kjunified::advertise(MDNS, httpPort)`. This adds `_kj-esp._tcp` and its three
contract TXT values without taking ownership of the responder or removing
existing services. Call again whenever the application restarts its responder.
On ESP32, `factoryHardwareId()` reads the factory eFuse base/station MAC and
returns `esp32-` plus lowercase hex. Failure returns an empty string; manifest
validation fails closed rather than inventing an identity.

`serveManifest(server, snapshotGetter)` emits a fresh `application/json`
response with `Cache-Control: no-store`; an empty snapshot returns 503. Bind it
to HTTP GET only. It has no enrollment/command callback. Discovery metadata is
an unauthenticated hardware claim, not authorization to control a device.

The relay adapter in `src/RelayManifest.h` consumes a values-only snapshot from
existing managers. Indicator descriptors reflect compile-time configuration,
with fitted hardware unknown and manager-disabled outputs unavailable. This
release does not re-enable them. Native tests use the shared interoperability
fixture and export a full adapter example at `.pio/relay-manifest-output.json`.

## Build and release gate

The default build on this branch produces a **1,309,648-byte** OTA binary for
**1,310,720-byte** application slots, leaving only **1,072 bytes**. PlatformIO's
ELF flash summary reports a different size and must not be used as OTA headroom.
The `scripts/check_firmware_size.py` post-build guard reads the actual generated
partition table and rejects binaries larger than any application slot. CI also
runs its unit tests. No partition sizes or OTA behavior were changed.

This default-placeholder build fits, but the release-headroom gate remains open:
production local overrides, release metadata or a different dependency/toolchain
build can grow it beyond the slot. Do not publish or upload this artifact on the
strength of the current build check. Review adequate release margin separately;
no local credentials were copied into this worktree, and no hardware smoke test
or OTA rollout was performed.
