---
id: crash-wayland-display-connection-lost-exit-96-d18a3c
title: "Wayland — the compositor drops the connection in production (exit code 96)"
status: open
priority: high
scope: Window/Platform
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Wayland — the compositor drops the connection in production (exit code 96)

> Dropped by the downstream application's crash-report tool: a cause never seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line, names of the application redacted); write outside it.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 13 — per version: 8.0.4 ×13 |
| Reports | 15 (first 2026-11-04, last 2026-11-10) |
| Versions | 8.0.4 ×15 |
| Platforms | linux ×15 |
| OS | CachyOS ×6, Bazzite ×3, Fedora Linux 44 (KDE Plasma Desktop Edition) ×2, Arch Linux ×1, Manjaro Linux ×1, Ubuntu 26.04.1 LTS ×1, … (1 more) |
| GPU (selected) | AMD Radeon RX 6900 XT (RADV NAVI21) (driver 26.2.6) ×2, AMD Radeon RX 7900 XTX (RADV NAVI31) (driver 26.2.6) ×2, AMD Radeon R9 200 Series (RADV HAWAII) (driver 26.2.5) ×1, AMD Radeon RX 6950 XT (RADV NAVI21) (driver 26.2.6) ×1, Intel(R) Graphics (ADL GT2) (driver 26.2.5) ×1, Intel(R) Graphics (ADL GT2) (driver 26.2.6) ×1, … (7 more) |
| ANGLE backend | vulkan ×15 |
| Died at milestone | Stopping ×15 |
| Single-instance lock | Acquired ×15 |

- **Cause (normalized):** `exit code 96 after Stopping`
- **Where:** no source location
- **Signature:** `d18a3c547ae0add6`

Last error before the death (else the last trace), by number of reports:

- ×11 `Core: The connection to the display server is lost (on Wayland a compositor protocol error, printed on stderr by libwayland) : the engine stops no`
- ×4 `VulkanQueue: Unable to present an image : VK_ERROR_SURFACE_LOST_KHR !`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) NetAPIClientService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) SystemInfoService primary service terminated gracefully! [main]
[SettingsService][Success] (vendors/emeraude-engine/src/Settings.cpp `EmEn::Settings::onTerminate`) Settings file saved to "<path>" !
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) SettingsService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) FileSystemService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) UserInfoService primary service terminated gracefully! [main]
[PrimaryServices][Success] (vendors/emeraude-engine/src/PrimaryServices.cpp `EmEn::PrimaryServices::Impl::terminate`) ArgumentsService primary service terminated gracefully! [main]
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::~Core`) *** Core level terminated ***
```

Archives:

- 15 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine (window, GLFW Wayland backend).
- **Production evidence (a downstream application, 8.0.4)**: 13 Linux machines, mostly AMD RADV on KDE / CachyOS / Bazzite / Fedora (so not only NVIDIA explicit sync), ANGLE vulkan. The connection is lost during ordinary idle use (no load burst in the journal), then the engine stops cleanly with exit code 96. Before the engine named exit 96, these runs were counted as silent deaths on Linux.
- **Missing evidence**: libwayland prints the protocol error (interface, object, code) on stderr, which the journal does not capture: without it the faulty request cannot be named.
- **Next step**: route libwayland's log into the tracer (`wl_log_set_handler_client`) and read `wl_display_get_protocol_error()` when the loss is detected; then attribute with `wayland-surface-lost-protocol-error`.

- **2026-11-12, the next step done (engine)**: libwayland's log goes to the tracer and the loss traces its protocol
  error (interface, object, code) or its errno (`docs/caution-points.md` § The force-quit dialog waits 10 s). Not
  exercised locally (no nested compositor): the first reports of a release that ships it will name the faulty request.

## What remains

- Read the named protocol errors in the reports of the first release that ships the diagnostic; attribute them with
  `wayland-surface-lost-protocol-error`.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
