---
id: crash-wayland-cursor-image-segfault-62c5eb
title: "Wayland — segfault in libwayland-client while GLFW sets the cursor image"
status: open
priority: high
scope: dependencies/glfw/src/wl_window.c
opened: 2026-11-10
tags: [crash, crash-report-auto]
---

# Wayland — segfault in libwayland-client while GLFW sets the cursor image

> Dropped by the downstream application's crash-report tool: a cause seen in its production crash reports.
> The block between the `crash-reports` markers is rewritten at every run of that tool (found by its signature line,
> names of the application redacted); write outside it. Deleting the item closes the cause: a new report of it from a
> newer version is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 2 — per version: 8.0.4 ×2 |
| Reports | 9 (first 2026-11-04, last 2026-11-17) |
| Versions | 8.0.4 ×9 |
| Platforms | linux ×9 |
| OS | CachyOS ×7, Manjaro Linux ×2 |
| GPU (selected) | NVIDIA GeForce RTX 3060 Ti (driver 620.70.06) ×7, AMD Radeon RX 6600 (RADV NAVI23) (driver 26.2.4) ×2 |
| ANGLE backend | vulkan ×9 |
| Died at milestone | ApplicationStarted ×9 |
| Single-instance lock | Acquired ×9 |

- **Cause (normalized):** `SIGSEGV / SEGV_MAPERR in the browser process: libwayland-client.so.0`
- **Where:** `libwayland-client.so.0` (no symbols: a driver or a system library)
- **Signature:** `62c5eb87db0bf510`

Last error before the death (else the last trace), by number of reports:

- ×9 `browser process, then the run died at ApplicationStarted`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[VulkanTransferManagerService][Info] (vendors/emeraude-engine/src/Vulkan/TransferManager.cpp `EmEn::Vulkan::TransferManager::releaseIdleStagingBuffers`) <n> staging buffer released (<n> MiB), <n> buffer and <n> image transfer operations kept.
[WindowService][Info] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::monitorConfigurationChanged`) New monitor removed from the configuration ! (×2)
[WindowService][Error] (vendors/emeraude-engine/src/Window.cpp `EmEn::Window::getDesktopSize`) Unable to query the primary monitor ! (×5)
```

Native stacks of the latest report (minidumps, crashing thread; `?` = guessed by stack scanning):

```
SIGSEGV / SEGV_MAPERR in the browser process
 0   libwayland-client.so.0 + 0x0000000000005704
 1 ? libEmeraude.so.1!setCursorImage [dependencies/glfw/src/wl_window.c:1283]
 2 ? libEmeraude.so.1!handleEvents [dependencies/glfw/src/wl_window.c:1473]
 3 ? libEmeraude.so.1!_glfwWaitEventsTimeoutWayland [dependencies/glfw/src/wl_window.c:2951]
 4 ? libEmeraude.so.1!EmEn::Input::Manager::waitSystemEvents(double) const [src/Input/Manager.cpp:898]
 5 ? libEmeraude.so.1!EmEn::Core::run() [src/Core.cpp:716]
 6 ? <a frame of the downstream application>
 7 ? libc.so.6 + 0x0000000000027d0d
 8 ? ? + 0x00007fff5432915e
```

Archives:

- 9 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-11-10)

- **Owner**: the engine (its GLFW, Wayland backend). 1 Linux machine (AMD RX 6600, RADV), 2 reports in 8.0.4,
  during use.
- **Stack**: `Core::run()` → `Input::Manager::waitSystemEvents()` → `_glfwWaitEventsTimeoutWayland` → `handleEvents`
  → `setCursorImage` (`wl_window.c`) → SIGSEGV in `libwayland-client.so.0`. Likely a cursor surface or buffer used
  after the connection or the cursor theme went away; related to `crash-wayland-display-connection-lost-exit-96-d18a3c`
  (the same desktops lose their Wayland connection).
- Found on 2026-11-10 once the crash-report tool named the causes of crashes in a library without symbols.

## Reading (2026-11-12) — a candidate, not proven

GLFW's Wayland backend (`dependencies/glfw/src/wl_window.c`, fork `f4360311` = 3.5.1 + the fork's one patch):
`incrementCursorImage()` (the animation timer, dispatched from `handleEvents`) advances `cursor->wl.currentImage`
modulo the image count of the 1x cursor `cursor->wl.cursor`; `setCursorImage()` then, on a HiDPI window
(`window->wl.bufferScale > 1` and a `cursorHiDPI` loaded), indexes `cursorHiDPI->images[currentImage]` — the 2x
cursor's array with an index bounded by the 1x count. If the theme gives the 2x size fewer frames, the read is past the
array: `wl_cursor_image_get_buffer()` (libwayland-cursor) receives a garbage image and calls `wl_shm_pool_create_buffer()`
on a garbage proxy — a SIGSEGV inside libwayland-client, the stack of the report.

- Not proven: the machine's theme and scale are unknown; the only theme installed on Linux main (Adwaita) has the same
  frame counts at every size (2 animated cursors, `xcur-frames` count). The same index flaw seems to be upstream.
- The fix would be one bound in `setCursorImage()` (`currentImage % wlCursor->image_count`): a SECOND patch on the
  fork (`docs/glfw-fork.md` keeps exactly one) and an upstream report — the owner's decision.
- Related public reports: a crash in `wl_cursor_image_get_buffer` reached through GLFW's cursor path (kitty, Fedora
  retrace 288203); animated cursors crashing Qt on Wayland (QTBUG-78652, KDE 463709).

## What remains

- **Owner decision 2026-11-12: the bound as a second fork patch + an upstream report** (`docs/glfw-fork.md` § 1b):
  fork commit `27737a13` on `em/customization`, pinned by the engine; upstream issue
  [glfw/glfw#2929](https://github.com/glfw/glfw/issues/2929).
- Then watch the reports of the first release that ships it; delete this item when the signature is gone.
- Reproduce: a HiDPI Wayland session with a theme whose animated cursor has fewer frames at 2x; else a compositor
  restart or a display connection killed while an animated cursor shows.

## References

- The downstream application's crash-report documentation — what an archive holds, the minidumps.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
