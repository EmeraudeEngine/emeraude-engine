---
id: deferred-resolve-projection-never-run-on-amd
title: The deferred resolve's light projection (2026-11-22) was never run on AMD (RADV)
status: open
priority: unranked
scope: src/Graphics/DeferredLightResolve.cpp (the bindless table as a second set), the AMD peer
opened: 2026-11-23
tags: [amd, radv, deferred-lights, projection, validation]
---

# The deferred resolve's light projection was never run on AMD (RADV)

## Why

The lots of 2026-11-22 were verified on Linux (RTX 3070 Ti), Windows (RTX 3500 Ada) and macOS (Apple M6, MoltenVK).
The AMD peer (`LinuxB-PA`, a RADV RENOIR iGPU — the only AMD path) was not connected that day. What it has never run:

- **the deferred resolve sampling a projection** (engine `2e8067e2`): the resolve's pipeline layout gained a SECOND
  descriptor set, the bindless table, a parallel buffer at binding 6 and `GL_EXT_nonuniform_qualifier` in its fragment
  shader — the part most likely to behave differently on another driver;
- the vehicle's hold at a standstill (`ef3e5a2b`) and the public lighting mark (`01b3984d`): CPU code, no driver
  path, listed for completeness;
- the traced projection (`060cb003`) does not apply: that machine runs the screen-space lane only.

Until then every one of them is "untested on AMD".

## What remains

When the AMD peer is connected, one request (peer-validation rules: heads to pull, a scratch build directory, "do not
commit or push"):

- Release build, 0 warning.
- projet-alpha's `city` by night, a driver in the car (two spots projecting a pattern), the validation layer proven
  loaded: 0 `VUID-`, 0 "Unable to compile shader", `Core.RendererService.getDeferredLightStatistics()` counting two
  more eligible lights once the driver is in (Windows read 453 → 455).
- The picture against the forward passes in ONE launch (`Core.RendererService.setDeferredPunctualLights(1 | 0 | 1)`),
  a pinned exposure, three captures a state: the difference between the two states inside the difference between two
  captures of one state, nothing on the road the beams light nor along their cut-off.
- The cost with the GPU profiler in a settings copy, the validation layer off: `ScenePass` and the frame with the
  lights off and on.
- Record the verdict in graphics doc 38 § *A light that projects a texture*, then delete this file.

## ⚠️ Traps

- That iGPU's bindless table may be cut to a smaller budget than the desktop's (macOS runs 956 2D slots): read the
  log's line before concluding from a refused pipeline.
- No build and no other instance during the pixel comparison.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § *A light that projects a texture* (the three OS'
  verdicts and their protocol).
