---
id: wayland-surface-lost-protocol-error
title: Wayland — a compositor protocol error kills the surface mid-load
status: open
priority: high
scope: Platform/Window
opened: 2026-08-04
tags: [wayland, glfw, intermittent, measured]
---

# Wayland — a compositor protocol error kills the surface mid-load

## Why

Measured **1 run in 8** (2026-08-04): `wp_linux_drm_syncobj_surface_v1#94: error 3: Release or
Acquire point set but no buffer attached`, then `VK_ERROR_SURFACE_LOST_KHR` at present, then a
clean `User exit code: 0` shutdown (2561 frames rendered). No device loss, no GPU fault — the
symptom reads as "the demo closed by itself".

TIMING IS THE LEAD: the error fires inside the **RT skinned-geometry creation burst** (Fox +
Paladin BLAS mirrors, ~4 MB of mirror buffers allocated back to back) — the "heavy load starves
the Wayland dialogue" family.

## ⚠️ Attribution — corrected 2026-08-22, do not re-derive

**THE ENGINE DOES NOT SPEAK EXPLICIT SYNC AT ALL.** Measured: of every object in the process's
startup link map (engine, GLFW inside it, libdecor and its plugins, CEF), **not one references
`wp_linux_drm_syncobj`**; the only binary that does is `libnvidia-glcore.so.610.57.04`, the NVIDIA
Vulkan driver, dlopen'd at runtime. The acquire/release points therefore come from the **driver's
WSI**, inside `vkQueuePresentKHR`. Auditing our present path for a sync point we never set would
burn a session looking for absent code.

The protocol rule (`linux-drm-syncobj-v1`): a client must set the acquire and release points **if
and only if a non-null buffer is attached in the SAME surface commit**. So the violation is an
EXTRA `wl_surface_commit()` carrying the driver's pending sync-point state but no buffer — i.e. it
comes from whoever else commits that surface. That is **GLFW**: its Wayland backend
(3.4-108-g4263be2a at the time of that measurement — the pin has since moved, see below) calls `wl_surface_commit` at 11 sites and drives libdecor through 74 call
sites (resize, fractional scale, opaque region, decoration updates), which is exactly why the
failure is intermittent.

Known upstream family, same protocol, same shape: godotengine/godot#93669,
kovidgoyal/kitty#7767, blender#135039, wezterm#6699.

Since 2026-08-22 both reporting sites (`Queue::present()`, `SwapChain::acquireNextImage()`) print
that reading next to the error through `vkResultDiagnosticHint()`.

## Observed 2026-10-16 — a shutdown that did not happen after the loss

A test instance of projet-alpha (`default`, Linux, RTX 3070 Ti, Wayland, engine 4fec0c77 + the uncommitted P-W
Resources annotation lot), launched beside the owner's own instance, got `VK_ERROR_SURFACE_LOST_KHR` at about the
moment the owner closed theirs. The frames went on (staging releases traced), the remote console ACCEPTED the
connection of `Core.shutdown()` but the command was NEVER executed ("Executing command" absent) — the process was
still alive 120 s later and killed by the run's timeout (exit 137). Before (2026-08), a loss was followed by a clean
`exit 0`. Not reproduced: 3 lone runs and a two-instance run with the first closed all ended `exit 0`, no loss.
Not attributed: the main-thread console poll stopped, cause unknown — the P-W lock changes of that day are not
excluded. ⚠️ Next time: `gdb -p <pid> -batch -ex "thread apply all bt 15"` on the hung process BEFORE killing it
(the log alone cannot tell a deadlock from a blocked swap-chain recreation).

## What remains — in order

**New data, 2026-10-12 (Linux, engine 9054be48 + uncommitted lot 2, Release, no validation layer):** reproduced at the
main menu (no demo) with a FRESH config directory (`--config-directory=<new empty dir>`, so every setting at its
default), `wp_linux_drm_syncobj_surface_v1#94: error 3: Release or Acquire point set but no buffer attached`, then
`VK_ERROR_SURFACE_LOST_KHR`. ⚠️ **After it the process HUNG instead of stopping**: the main thread spun at 100 % CPU in
`libdecor-gtk.so` → `wl_display_prepare_read_queue()` (three gdb samples, the same loop), a second thread at 100 % in
`system_clock::now()` under libwayland-client, the remote console accepted the connection but never executed
`Core.shutdown()`, SIGTERM was ignored (the termination catcher's flag is read by the stuck main loop): only SIGKILL
ended it. So a dead Wayland display also takes the main loop with it — the clean stop on a lost surface does not run.
That clean stop is step (4) below; it is owned HERE and shared with `igpu-presentation-under-nvidia-wayland` (same
unanswered stop after a dead display, different cause).

**Frequency, 2026-10-12 (Linux, the 37-demo validation sweep, Release, validation layer ON):** 2 of 37 demos lost the
surface with the same `wp_linux_drm_syncobj_surface_v1#100: error 3` (`paths` 2 s after its load, `world-lobby` during
its load); the same build passed both demos 2 / 2 right after, and the previous sweep (lot 2) passed all 37. Neither
process answered the console afterwards (the call timed out) and both had to be killed: the hang above, again.


- [ ] **(1) Capture the protocol trace on the owner's session** (only a real compositor will do —
  X11/Xvfb has no libdecor and no explicit sync):
  `python3 tools/wayland-protocol-trace.py --capture -- ./projet-alpha --load-demo game-logic --disable-cef`
  keeps only the failing run, then `--analyse <log>` resolves the offending object to its
  `wl_surface`, prints the requests of the rejected commit and says whether it carried an attach —
  which names the committer outright.
  ⚠️ `WAYLAND_DEBUG` slows the client enough to hide a tight race: a clean sweep is a RESULT, not
  a failed attempt.
- [ ] **(2) A/B the libdecor plugin** — `LIBDECOR_PLUGIN_DIR=<dir with only libdecor-cairo.so>`.
  The GTK3 plugin only started loading on 2026-08-22 (the symbol-interposition fix removed the
  `png_free` conflict that had been making libdecor refuse it), so the plugin in use CHANGED that
  day. ⚠️ The defect itself predates it (measured 2026-08-04 with the cairo plugin) — but nothing
  says the FREQUENCY is unchanged, and that A/B is what settles it.
- [ ] **(3) Only then** consider a GLFW-side fix/update; a vendored-submodule bug is fixed
  upstream, never worked around here.
  ⚠️ **The GLFW pin MOVED on 2026-08-31**, independently of this item: 3.4-108-g4263be2a →
  **3.5.1** + the custom patch (`f4360311`, see [`../glfw-fork.md`](../glfw-fork.md)). Two upstream
  commits in that delta touch code, and one of them is in this item's own code path:
  `b7ef2919` *"Wayland: Fix rounding of fractional scale sizes"* makes `resizeFramebuffer()` round
  the fractional-scale framebuffer size (`(w * num + 60) / 120`) instead of truncating it
  (upstream #2713 / #2810). **This is NOT a fix for this defect** — the protocol violation is an
  extra buffer-less commit, not a wrong size — but it does mean **every measurement above was taken
  on a different GLFW**, and the 1-in-8 frequency has to be re-established on the new pin before it
  is compared to anything. Step (1) is still the next action.
- [ ] **(4) Stop cleanly on a dead display connection** (independent of (1)-(3)): make the window service detect it
  (`wl_display_get_error()` through GLFW, or a GLFW error callback) and stop the application instead of entering
  libdecor's loop; then `Core.shutdown()` and SIGTERM must work after a lost surface. Also closes the second defect of
  `igpu-presentation-under-nvidia-wayland` (re-check it there once done).
