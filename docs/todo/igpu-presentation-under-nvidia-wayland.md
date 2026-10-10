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

## What remains

- Decide what a lost / unpresentable surface at startup means: refuse the device and fall back to the next one by
  score, or stop with a clear Fatal — owner decision.
- Find the SIGSEGV after the Wayland protocol error (GLFW's connection is dead), and make the failure path exit cleanly.
