---
id: skinned-instances-pay-their-acceleration-structure-every-frame
title: A skinned instance pays its ray-tracing structure every frame, far away and out of view
status: open
priority: unranked
scope: src/Graphics (RayTracing, SkinnedGeometryProcessor, AccelerationStructureBuilder), src/Scenes (the TLAS selection)
opened: 2026-11-21
tags: [ray-tracing, performance, vram, skinning]
---

# A skinned instance pays its ray-tracing structure every frame, far away and out of view

## Why

Measured on 2026-11-21 with projet-alpha's `city` at side 128 (RTX 3070 Ti 8 GB, ray-traced lane, window-less
1280 × 720, no validation layer, the GPU profiler on, night): sixteen skinned characters of 116 341 vertices and
150 000 triangles stand idle in the streets, the nearest 496 m from the camera, NONE in view. Eleven of them lie
within `Core/Graphics/RayTracing/TLASDistance` (1000 m).

| At the spawn | 16 characters | none | Difference |
|---|---|---|---|
| Frame rate (median of 24 samples) | 61.5 | 84.0 | −22.5 |
| `Frame` (GPU, average) | 14.92 ms | 12.40 ms | +2.52 ms |
| `TLASBuild` | 2.62 ms | 0.18 ms | **+2.44 ms** |
| `ScenePass` | 6.17 ms | 5.53 ms | +0.65 ms |
| Device memory allocated | 2934 MiB | 2645 MiB | **+289 MiB** |

The memory, by group: `AS_STORAGE` buffers +99.6 MiB (11 buffers), `STORAGE|DEVICE_ADDRESS|AS_INPUT` buffers
+107.4 MiB (11 buffers: one skinned copy of the vertices per instance), the textures +64 MiB, the shared vertex and
index buffers +11.5 MiB. That is about **0.22 ms a frame and 19 MiB per character in range**, whatever it does and
wherever it stands. With the camera 1300 m from the city's centre (no character within the TLAS distance) the two
sides run at the same rate (98 and 97 frames a second): the cost follows the TLAS distance, not the view.

Not a defect: the structures are right. A cost to decide about — the target card has 8 GB, and a crowd of skinned
characters is out of reach at this price.

## What remains

Investigate first (none of this is measured yet):

- Whether an instance whose pose did not change between two frames is refitted anyway (a character holding a pose).
- Whether a skinned instance needs the scene's whole TLAS distance: a character 800 m away adds nothing a reflection
  or the global illumination shows. A distance of its own for skinned instances, or a selection by projected size.
- Whether a far skinned instance can enter the TLAS with a REST-pose structure shared by every instance of the mesh
  (no skinned copy, no refit) until it comes close.

Then propose the mechanism to the owner with its numbers (the same A/B: `city` side 128, the spawn, 16 against 0).

## ⚠️ Traps

- The cost does not show in a pose taken far from the characters, nor with a small scene: measure at a pose INSIDE
  the TLAS distance of many of them.
- `TLASBuild` is the zone to read; the frame rate alone hides it behind the scene pass.

## References

- `docs/subsystems/scenes/14-ray-tracing-architecture.md` (the session, the `SkinnedGeometryProcessor`, refit against rebuild).
- projet-alpha `docs/subsystems/builtin/27-city-the-tile-built-city.md` § The hunt — measured.
