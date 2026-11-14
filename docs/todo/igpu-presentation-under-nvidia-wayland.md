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
- ~~Find the SIGSEGV after the Wayland protocol error~~ Found and fixed downstream (2026-11-14, § Observed below): the
  display-loss stop arrived while the act was being built. With projet-alpha's stage serving the stop when the build
  returns, the same run (Debug + TSan, the Intel device forced) ends with exit code 96, 0 VMA assertion, no SIGSEGV.
  What is left of the failure path is the hang when the protocol error is READ before a flush fails: step (4) of
  `wayland-surface-lost-protocol-error`.

## Observed 2026-10-11: the failed instance does not stop

`AutoSelectMode = PowerSaving` on Linux (RTX 3070 Ti + Intel RPL-S, NVIDIA Wayland) selects the iGPU; the swap-chain then
fails (`VK_ERROR_SURFACE_LOST_KHR` → "Unable to refresh the swap-chain!" Fatal) while the scene still loads, and the
instance ignores `Core.shutdown()` (the console accepts the connection, nothing happens) and SIGTERM; it needed SIGKILL.
The main thread was in `wl_display_dispatch_queue_pending()` (gdb, Release). Same root as this item; the unanswered
shutdown is a second defect, NOT fixed here: the clean stop on a dead Wayland display is step (4) of
`wayland-surface-lost-protocol-error`. Re-check this case once that step and the fallback above both exist.

## Observed 2026-11-14 (Mesa 25.0.7, mutter 48.7, NVIDIA 620.70.06): the SIGSEGV has a stack, the hang is the known loop

Two runs of projet-alpha `default` with `ForceGPU = "Intel(R) Graphics (RPL-S)"` in a settings copy; the compositor
refused the first buffer both times (gnome-shell's journal: `WL: error in client communication`).

- **Debug + TSan** — the loss reached the engine as `Connection reset by peer` (a flush failed before the error event
  was read: no `libwayland:` line), and the display-loss stop ran (exit code 96) WHILE the act was still being built on
  its worker. The act was activated after the application's stop hook and its scene destroyed after the device:
  7 VMA "allocations were not freed" assertions, a null allocator, then a SIGSEGV in `vmaDestroyBuffer` ←
  `Vulkan::Buffer::destroyWithVMA()` ← `Scene::~Scene()` ← the application's destructor. This is the SIGSEGV of "What
  remains" above; its cause was downstream (projet-alpha `docs/caution-points.md` § Core Application, fixed the same day).
- **Debug, `WAYLAND_DEBUG=1`** — the error was read: `wl_display#1.error(nil, 7, "failed to import supplied dmabufs:
  Could not bind the given EGLImage to a CoglTexture2D")`, 0.6 ms after the first `wl_surface.commit()` of Mesa's
  "mesa vk display queue". Then the instance hung: the main thread at 100 % inside libdecor-gtk 0.2.2, alternating
  `wl_display_prepare_read_queue()` and `wl_display_dispatch_queue_pending()` (four gdb samples), called by GLFW's
  `handleEvents()` whose first call is `libdecor_dispatch()` (`wl_window.c:1400`); it never returns, so the main thread
  keeps the native-surface lock and the rendering thread waits for it in `Renderer::recreateRenderingSubSystem()`
  (`retryDegradedSwapChain()`). The console accepted the connection and never executed `Core.shutdown()`. SIGTERM ended
  the process at once (exit 143): at startup the termination catcher is not installed yet while `--load-demo` builds.
  It is step (4) of `wayland-surface-lost-protocol-error`, unchanged.
- ⚠️ So on this workstation a ThreadSanitizer run cannot use the Intel device as its Mesa device: `llvmpipe` only
  (`tsan-logic-render-shared-state-races` § Traps).

## References

- Item `wayland-surface-lost-protocol-error`: a DIFFERENT cause (a buffer-less commit, not an unpresentable device),
  but the same unanswered stop after a dead Wayland display — its point (4) (detect the dead display connection and
  stop cleanly) is the shared remedy for the second defect above.
