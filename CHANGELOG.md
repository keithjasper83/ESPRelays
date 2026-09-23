# Changelog

## v4.2.4 - 2026-09-23

Add OTA command routing over ESP-NOW with relay support.

## v4.2.1 - 2026-09-23

- docs: record combined recovery firmware image size (3baa839)
- merge: preserve brownout and calibration safeguards in MQTT-free firmware (8b8f346)
- refactor(firmware): retire MQTT and disconnected LED support (9bd6391)
- feat(discovery): advertise relay capability manifests over mDNS (009d586)
- fix(firmware): finalize relay recovery safeguards (1f3bd79)
- docs(release): describe relay recovery safeguards (4956205)
- test(status): cover local recovery diagnostics (bc26782)
- feat(status): expose reset and storage diagnostics (11e63c7)
- fix(relay): force off after confirmed brownout (ebd38d7)
- test(temperature): cover calibration rollback (4f51471)
- fix(temperature): verify calibration writes (34207af)
- fix(temperature): debounce probe presence detection (7015052)
- Merge branch 'main' of https://github.com/keithjasper83/ESPRelays (914c5d8)
- feat: Update document title with mDNS host in WebControlServer (5782e19)

## v4.1.0 - 2026-08-16

- fix(calibration): reject invalid reference pairs (f7219f1)

## Unreleased

- Debounce temperature-probe presence changes, verify calibration storage writes, and expose reset diagnostics for recovery investigations.
- Force relays OFF only after a confirmed brownout; non-brownout restoration of the persisted relay state is unchanged.
- Do not embed calibration values in firmware; calibration remains device-specific persisted data.
- Reject equal, reversed, out-of-range, or non-numeric temperature calibration references before they can be reported as ready or persisted as a complete pair.
- Preserve valid single-point records during guided capture and roll back in-memory changes if NVS persistence fails.

## v4.0.0 - 2026-08-16

- Update artifact upload paths in CI workflow (c5ea68c)
- ci: set artifact retention to 10 days (20870f2)
- feat: add Unified Server control and durable calibration (bc11aa3)
- Add telemetry subsystem with configuration, data management, and diagnostics (de6266e)
- feat: Add Matterbridge MQTT integration and simplify MQTT topic structure (672ded7)
- feat: Add factory reset functionality and temperature monitoring settings (fcd747b)
- Add WS2812 LED strip control and status (1aebae5)

## v2.1.3 - 2026-07-11

- Merge branch 'main' of https://github.com/keithjasper83/ESPRelays (7b4dbf6)
- feat: Add LED active-high configuration and persistence support (fb34637)

## v2.1.2 - 2026-07-11

- Merge pull request #2 from keithjasper83/copilot/fix-offline-error (8c768d0)
- test: align command parity assertion with MQTT routing flow (57857bf)
- test: Update MQTT command handler tests for new routing logic (dea90aa)
- style: Format MQTT callback for improved readability (d39a1a7)
- feat: Enhance MQTT operations and add LED test endpoints (747515d)
- Merge branch 'main' of https://github.com/keithjasper83/ESPRelays (491f3cd)
- fix: update OTA release info URL to point to the correct asset name (f57403a)
- feat: Add temperature probe management and calibration features (6767168)
