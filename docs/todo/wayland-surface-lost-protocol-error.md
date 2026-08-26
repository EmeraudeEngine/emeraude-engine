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
(3.4-108-g4263be2a here) calls `wl_surface_commit` at 11 sites and drives libdecor through 74 call
sites (resize, fractional scale, opaque region, decoration updates), which is exactly why the
failure is intermittent.

Known upstream family, same protocol, same shape: godotengine/godot#93669,
kovidgoyal/kitty#7767, blender#135039, wezterm#6699.

Since 2026-08-22 both reporting sites (`Queue::present()`, `SwapChain::acquireNextImage()`) print
that reading next to the error through `vkResultDiagnosticHint()`.

## What remains — in order

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
