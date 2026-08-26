---
id: jungle-ruins-fence-timeout-abort
title: jungle-ruins dies on an explicit abort after a 60 s fence timeout (Release only)
status: open
priority: high
scope: Graphics/Renderer
opened: 2026-08-09
tags: [usd, crash, measured, release-only]
---

# jungle-ruins dies on an explicit abort after a 60 s fence timeout

## Why

`--load-demo jungle-ruins --demo-options 2` dies in **SIGABRT (134)** after 1-3 min, preceded by
`[VulkanFence] Unable to wait the fence : VK_TIMEOUT` + `Something wrong happens while waiting the
fence for image #N`. Reproduced twice, including once on a fully idle machine.

⚠️ An early conclusion blaming rebuild pollution (`libEmeraude.so` rewritten under the running
process) was **FALSE** — it reproduces with no pollution at all.

⚠️⚠️ The SIGABRT is an **explicit `std::abort()`** (`Graphics/Renderer.cpp:1543`) when the frame
fence does not signal within **60 s** — not memory corruption: the engine gives up on a mute GPU.
Zero Vulkan validation errors before the abort.

## What is already excluded

- **Control run, discriminating**: `--load-demo default` held **22 min, 0 VK_TIMEOUT** ⇒ the crash
  is SPECIFIC to `jungle-ruins`, neither Wayland in general nor the engine in general.
- ⚠️⚠️ **It does NOT reproduce in Debug**: 9 min under gdb, same scene, 0 timeout, where Release
  dies in 1-3 min. Debug runs at ~4.5 FPS (223 ms/frame, -O0) ⇒ leading hypothesis is a
  **cadence-dependent** failure (resource pressure or a race only a fast loop reaches), not the
  scene content.

## What remains

- [ ] **Stay in RELEASE** — Debug is the wrong instrument since it does not reproduce. Instrument
  the frame loop, or run Release under the validation layers + synchronization validation.
- [ ] Unexplored leads, in order: VRAM residency (4096² textures; `TextureCompressor` initialises
  3× in the log), then the instancing path itself.

GPU selected in the failing runs: RTX 3070 Ti (8 GB).

## References

- Same temporal signature as `compressed-gltf-sigill-at-idle.md` (different signal) and as
  `wayland-surface-lost-protocol-error.md` (the swap-chain timeout is the CONSEQUENCE there).
