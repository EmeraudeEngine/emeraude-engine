---
id: jungle-ruins-fence-timeout-abort
title: jungle-ruins' first full-vegetation frame exceeds the 60 s fence timeout — imposters, then re-measure
status: open
priority: high
scope: Graphics/Renderer, scene loaders (USD), vegetation rendering
opened: 2026-08-09
tags: [usd, measured, release-only, performance]
---

# jungle-ruins' first full-vegetation frame exceeds the 60 s fence timeout

## Why

`--load-demo jungle-ruins --demo-options 2` (and option 5: full scene and all vegetation, 8.63 M instances) loads,
and then the first frame's fence does not signal within 60 s. Until 2026-10-11 this ended in a SIGABRT (134) from
`beginFrame()`'s `std::abort()`. Since df1b8158 the same timeout is treated as a device loss: a clean stop with exit
code 98 (`renderer-fail-fast-on-device-loss`). The workload itself is the defect: the frame is too heavy.

Measured, kept as the history of the diagnosis:
- **Not rebuild pollution** (an early false conclusion), and **not** the engine in general: `default` held 22 min
  with 0 `VK_TIMEOUT`.
- **Release only.** Debug at ~4.5 FPS held 9 min under gdb.
- **The 2026-10-04 device loss is explained and fixed.** `NVRM: Xid 109 … CTX SWITCH TIMEOUT` was VRAM
  oversubscription: 28.9 GB on 8 GB, 23 GB of it duplicated forest geometry. With the geometry shared (7327a4b0d) it
  is 5.57 GB, with no device loss. The original timeout came back after that fix.
- **The frame is the cause.** 613 806 QueenForest and 2.4 M RiverForest instances of 750k-940k-vertex trees.

## Done

- Geometry shared by content: 7327a4b0d (the `geometry-content-dedup` item, still in progress for the other loaders).
- Automatic geometry LOD chains for meshes of 20k triangles or more, plus a draw distance (default 1000) and a shadow
  distance per instance cell (`InstanceClusterOptions`): 89ba38fac. Its own message records that the GPU still sits at
  100 % even with a 60 m cut.
- The `std::abort()` is gone: df1b8158.

## What remains

- Draw the far vegetation as octahedral imposters (graphics 15e; its refinements are the `imposter-*`
  items), then re-measure the first frame of options 2 and 5 in RELEASE, with validation on.
- If the first frame still exceeds the timeout, find which submission is too long: the acceleration-structure
  builds were the device-fault checkpoints' first suspect (`AS-build:end` on several queues).

## References

- `docs/scene-loaders-usd.md`, `geometry-content-dedup.md`, `renderer-fail-fast-on-device-loss.md`.
- Same temporal signature: `compressed-gltf-sigill-at-idle.md`, `wayland-surface-lost-protocol-error.md`.
