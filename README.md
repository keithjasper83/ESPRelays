<!--
SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 Keith Jasper
Contact: https://github.com/keithjasper83/ESPRelays/issues
-->

# ContactorRelays

![ContactorRelays hero](docs/images/hero-relay.svg)

[![CI](https://img.shields.io/github/actions/workflow/status/keithjasper83/ESPRelays/ci.yml?branch=main&style=for-the-badge&label=CI)](https://github.com/keithjasper83/ESPRelays/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/keithjasper83/ESPRelays?style=for-the-badge&label=Release)](https://github.com/keithjasper83/ESPRelays/releases)
[![Issues](https://img.shields.io/github/issues/keithjasper83/ESPRelays?style=for-the-badge&label=Issues)](https://github.com/keithjasper83/ESPRelays/issues)
[![Stars](https://img.shields.io/github/stars/keithjasper83/ESPRelays?style=for-the-badge&label=Stars)](https://github.com/keithjasper83/ESPRelays/stargazers)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache%202.0-0B6E6E?style=for-the-badge)](LICENSE)
![Platform: ESP32-C3](https://img.shields.io/badge/Platform-ESP32--C3-0F766E?style=for-the-badge)
![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-0EA5A5?style=for-the-badge)
![OTA Enabled](https://img.shields.io/badge/OTA-Enabled-854D0E?style=for-the-badge)
![Local First](https://img.shields.io/badge/Local-First-1D4ED8?style=for-the-badge)
![Suggestions Welcome](https://img.shields.io/badge/Suggestions-Welcome-7C3AED?style=for-the-badge)

ContactorRelays turns a small ESP32 board into a serious automation edge node:
fast local control, resilient remote commands, and enough scheduling intelligence
to run real-world devices without babysitting.

If you want "set it and trust it" relay control for home labs, utility spaces,
workshops, or small installations, this project is built for exactly that.

## Visual Preview

![Web panel concept preview](docs/images/web-panel-preview.svg)

## Quick Demo Vibe

![Animated relay pulse preview](docs/images/relay-pulse-demo.svg)

## System Flow At A Glance

![ContactorRelays high-level flow](docs/images/network-flow.svg)

## Why It Exists

- Local-first HTTP control with Unified Server WebSockets for remote workflows.
- Practical automation features instead of demo-only gimmicks.
- Hardware-aware defaults for ESP32-C3 deployments.
- A clean base for extending into your own automation platform.

## Mission Statement: Your Hardware, Your Rules

We believe devices in your home should stay yours.

That means we do not hard-lock firmware into an unchangeable state by default.
Yes, security matters. A lot. But context matters too:

- This is home automation gear, not a nuclear launch console.
- The realistic threat model is usually inconvenience, not national-security meltdown.
- If someone hacks your relay and turns your TV off during Coronation Street,
  that is annoying and deeply rude, but it is not a state emergency.

So our design stance is simple:

- Prioritize secure defaults and transparent hardening options.
- Keep recovery and owner control possible.
- Avoid permanent lock-in that bricks ownership if support disappears.

If the company, cloud, or update server ever vanishes into the void, your kit
should still work in your house, on your terms, with firmware you can inspect
and replace.

Security should protect users, not trap them.

## Key Capabilities

- Web control endpoints for direct relay interaction and runtime configuration.
- OTA check/update paths for safer firmware rollout.
- Time sync and schedule execution for repeatable behavior.
- UDP discovery for quick device visibility on the network.

## Firmware builds and compatibility

PlatformIO is the build definition for every supported build:

- `pio run -e esp32-c3-devkitm-1`: device firmware (also used by CI and release).
- `pio test -e native`: host tests.
- `node --test test/web/*.test.cjs`: embedded console regression tests.
- `python3 -m unittest discover -s test/build_guard -v`: build and partition guards.

There is no separate supported ESP-IDF/CMake build. The obsolete generated stubs
and unused vendored mDNS component have been removed. ESP32 Arduino's SDK mDNS
responder still advertises `_http._tcp` and `_kj-esp._tcp` on port 80.

The 4.2.0 development firmware removes MQTT/Matterbridge and disconnected LED
controls. Direct HTTP control and Unified Server WebSocket communication remain.
See [migration and measured storage](docs/FIRMWARE_CLEANUP.md) before upgrading.

## Self-hosted CI runner bootstrap

Runner bootstrap scripts and the autoplaybook are now maintained in the public
dependency repository [keithjasper83/GitHub-Runner](https://github.com/keithjasper83/GitHub-Runner),
included here as a git submodule at `tooling/github-runner`.

Quick start examples:

```bash
tooling/github-runner/scripts/bootstrap_runner.sh register --repo keithjasper83/ESPRelays --service
```

```powershell
.\tooling\github-runner\scripts\bootstrap_runner.ps1 -Action register -Repo keithjasper83/ESPRelays -RunAsService
```

CI runner selection is controlled by repository variable `CI_RUNNER` in
`.github/workflows/ci.yml`:

- Leave unset to use GitHub-hosted `ubuntu-latest`.
- Set to a JSON array for self-hosted labels, for example:

```text
["self-hosted","linux","x64"]
```

## Ideal Use Cases

- Smart home relay control with local fallback behavior.
- Workshop and utility automation with low-latency switching.
- Prototype control nodes for larger IoT systems.
- Learning reference for production-minded ESP32 service architecture.

## Roadmap Ideas

Short-term:
- Better setup UX in the web panel with stronger validation and onboarding hints.
- Richer diagnostics endpoint for easier field troubleshooting.
- More contract tests around config and scheduling edge cases.
- Auto-off from last-off style function (design and review only, not implemented yet).

Mid-term:
- Role-based web auth options and hardened security defaults.
- Scene and rule primitives (for example "if this then switch that").
- Enhanced telemetry model for fleet-level observability.

Long-term:
- Multi-device coordination patterns.
- Optional cloud bridge modules while keeping local-first behavior.
- Deployment profiles for home, lab, and light industrial scenarios.

Engineering review backlog:
- See [docs/TODO.md](docs/TODO.md) for the current engineering backlog.

Hardware and wiring reference:
- See [docs/WIRING.md](docs/WIRING.md) for the current ESP32-C3 Super Mini pin map and block diagram.

Release history:
- See [docs/CHANGELOG.md](docs/CHANGELOG.md).

## Suggest Features And Improvements

The best ideas usually come from real installs. If you have a suggestion:

- Open an issue at https://github.com/keithjasper83/ESPRelays/issues.
- Start the title with one of: `Feature:`, `Idea:`, `Roadmap:`.
- Include your use case, what hurts today, and what success looks like.
- If possible, add hardware details and expected command flow.

Fast template you can paste into an issue:

```md
## Problem
What is hard today?

## Proposal
What should change?

## Value
Why this matters in real usage.

## Environment
Board, firmware version, network setup, and Unified Server connectivity.
```

## Contact

- Suggestions and bugs: https://github.com/keithjasper83/ESPRelays/issues
- Trademark/branding permissions: see [docs/TRADEMARKS.md](docs/TRADEMARKS.md)

## ESP Home Unified Server

Firmware 3.1.0 listens for the local Unified Server beacon on
`239.255.42.99:42424` and opens one outbound WebSocket for registration,
heartbeats, state, idempotent relay commands, and acknowledgements. `GET
/unified/hello?server=http://192.168.0.50:8111` provides the server's reverse
probe path. The native HTTP interface remains available independently for local control.

Wi-Fi sleep and ESP light/deep sleep are disabled. The CPU runs at 80 MHz while
the relay, button, discovery, WebSocket, HTTP, and OTA loops remain responsive.
Temperature calibration is stored as a versioned CRC32-protected NVS record.
Two alternating checksummed slots protect the last confirmed calibration from
an interrupted write. Unified Server receives immutable copies of every
calibration state and can explicitly restore a complete record to replacement
hardware using the same logical hostname.
The first 3.x boot migrates the existing `temp_probe` values exactly and keeps
the legacy keys mirrored for safe downgrade. OTA does not erase NVS, and
calibration values change only after an explicit capture, reset, or trim action.

## License

This project is licensed under Apache License 2.0.
See [LICENSE](LICENSE) and [NOTICE](NOTICE) for required attribution and
redistribution notices.

## Attribution

If you redistribute this project or derivatives, you must retain:
- License text (Apache-2.0)
- Copyright notices
- NOTICE attributions

## Branding

Code is open under Apache-2.0, but project branding (names/logos) is not
licensed for unrestricted use.
See [docs/TRADEMARKS.md](docs/TRADEMARKS.md).
