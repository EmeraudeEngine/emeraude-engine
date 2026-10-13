---
id: igpu-presentation-under-nvidia-wayland
title: An Intel iGPU presenting to a Wayland session driven by the NVIDIA GPU dies instead of failing cleanly
status: open
priority: high
scope: Vulkan/SwapChain, Window (GLFW Wayland), Core startup
opened: 2026-10-11
tags: [vulkan, wayland, intel, defect]
---

# An Intel iGPU presenting to a Wayland session driven by the NVIDIA GPU dies instead of failing cleanly

## Why

Linux, GNOME Wayland on the RTX 3070 Ti, `ForceGPU = "Intel(R) Graphics (RPL-S)"` (2026-10-10/11): the engine logs
"'Intel(R) Graphics (RPL-S)' support presentation!", then the compositor refuses the Intel buffers
(`[destroyed object]: error 7: failed to import supplied dmabufs: Could not bind the given EGLImage to a
CoglTexture2D`), the surface queries return `VK_ERROR_SURFACE_LOST_KHR`, and the process died: first an abort from a
throwing `.at()` in `SwapChain::chooseSurfaceFormat()` (fixed 2026-10-11), then a SIGSEGV. `--window-less` on the same
device works (labyrinth, sponza: 0 VUID).

## Owner decision (2026-10-11)

A device that cannot present to the surface is set aside and the next one by score is taken, with a warning; a FORCED device (ForceGPU) stops cleanly with an explicit Fatal instead (an explicit request is never worked around).

## What remains

- Implement the owner decision above (2026-10-11): at startup, an unpresentable device is set aside with a warning and
  the next one by score is taken; a FORCED device stops cleanly with an explicit Fatal.
- Find the SIGSEGV after the Wayland protocol error (GLFW's connection is dead), and make the failure path exit cleanly.

## Observed 2026-10-11: the failed instance does not stop

`AutoSelectMode = PowerSaving` on Linux (RTX 3070 Ti + Intel RPL-S, NVIDIA Wayland) selects the iGPU; the swap-chain then
fails (`VK_ERROR_SURFACE_LOST_KHR` → "Unable to refresh the swap-chain!" Fatal) while the scene still loads, and the
instance ignores `Core.shutdown()` (the console accepts the connection, nothing happens) and SIGTERM; it needed SIGKILL.
The main thread was in `wl_display_dispatch_queue_pending()` (gdb, Release). Same root as this item; the unanswered
shutdown is a second defect, NOT fixed here: the clean stop on a dead Wayland display is step (4) of
`wayland-surface-lost-protocol-error`. Re-check this case once that step and the fallback above both exist.

## References

- Item `wayland-surface-lost-protocol-error`: a DIFFERENT cause (a buffer-less commit, not an unpresentable device),
  but the same unanswered stop after a dead Wayland display — its point (4) (detect the dead display connection and
  stop cleanly) is the shared remedy for the second defect above.
