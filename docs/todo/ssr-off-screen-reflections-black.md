---
id: ssr-off-screen-reflections-black
title: The screen-space lane reflects off-screen content as black (post-processor-effect-debug panels)
status: open
priority: high
scope: Graphics/PostProcess (SSREffect ray-miss fallback), environment cubemap binding
opened: 2026-10-11
tags: [rendering, ssr, defect]
---

# The screen-space lane reflects off-screen content as black (post-processor-effect-debug panels)

## Why

`post-processor-effect-debug`, six reflective panels facing the camera (they mirror the room BEHIND it):

| Run (2026-10-10/11) | Lane / reflection occupant | Frame luma | Panels |
|---|---|---|---|
| Linux RTX 3070 Ti, `LightingLane = Auto` | RayTracing / RTREffect | 131.4 | the roughness gradient |
| Linux RTX 3070 Ti, `LightingLane = ScreenSpace` | ScreenSpace / SSREffect | 74.7 | black |
| Windows Intel UHD (no RT) | ScreenSpace / SSREffect | 74.8 (panels 6.1) | black |

Every SSR ray misses (the reflected room is off-screen), and the ray-miss environment fallback (caution-points:
"SSR's ray-miss environment fallback (lot 4) reads the bindless prefiltered slot") yields black, although the log
says the scene uses `+DefaultTextureCubemap` and the bindless environment slot is initialised. Every machine
without ray tracing (the Apple GPUs, an Intel iGPU) and every run on the low settings profile (lane `ScreenSpace`)
renders this. Possibly the same cause: `labyrinth`'s ceiling is black on Intel (RTGI absent).

## What remains

- Find why the miss path returns black: the prefiltered slot content for `+DefaultTextureCubemap`, the roughness LOD,
  or a weight that zeroes the fallback.
- Re-measure the three rows: the ScreenSpace panels must show the environment, not black.
