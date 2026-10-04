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

**2026-10-04, option 5 (full scene + all vegetation, 8.63 M instances):** the load completes (entities at 317 s), then
20 s later `VK_ERROR_DEVICE_LOST` → SIGABRT. Kernel: `NVRM: Xid 109 … CTX SWITCH TIMEOUT` (the GPU stayed in one
context too long). The device-fault checkpoints show the last regions reached as `AS-build:end` on several queues and
image-layout transitions: the acceleration-structure builds of millions of instances are the first suspect (one
submission too long for the driver's watchdog). Peak RSS 78 GB. Log kept only in the session scratchpad.

## References

- Same temporal signature as `compressed-gltf-sigill-at-idle.md` (different signal) and as
  `wayland-surface-lost-protocol-error.md` (the swap-chain timeout is the CONSEQUENCE there).

**2026-10-04, later — the device loss explained and fixed; the timeout remains.** The Xid 109 was VRAM
oversubscription (28.9 GB on 8 GB, 23 GB of it duplicated forest geometry: `docs/scene-loaders-usd.md`, item
`geometry-content-dedup`). With the geometry shared: 5.57 GB, no device loss — and the original symptom of this
item comes back: the first frame's fence does not signal within 60 s (`VK_TIMEOUT` → `std::abort()`). The frame
is the cause: 613 806 QueenForest and 2.4 M RiverForest instances of 750k-940k-vertex trees, no geometry LOD, no
imposter, no draw-distance cut on instance cells. What remains here: the vegetation drawn with the engine's own
techniques (owner, 2026-10-04) — octahedral imposters (graphics 15e), automatic geometry LOD (15b), a distance cut
per cell — then re-measure the first frame.
