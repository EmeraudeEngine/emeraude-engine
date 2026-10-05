---
id: shadow-map-rendered-for-disabled-light
title: A switched-off light still renders its shadow map every frame
status: open
priority: unranked
scope: Graphics/Renderer (shadow map loop), Scenes (light emitters)
opened: 2026-10-05
tags: [performance, shadows, measured]
---

# A switched-off light still renders its shadow map every frame

## Why

Measured 2026-10-05 on `basic-scenery` (RTX 3070 Ti, 1280×720 window-less, GPU profiler on): its four moving
coloured point lights (`WhiteLight`, `RedLight`, `GreenLight`, `BlueLight`, 1024 shadow cubemaps) were switched off
through `Core.SceneManagerService.PointLight.setEnabled(entity, component, false)`. Their lighting left the frame
(`ScenePass` 12.3 → ~7 ms: the forward passes of a shadowed light are gone), but their `ShadowMap/<Light>…` zones
kept being recorded: the sample count rose with every frame and each zone still took 2.7-3.8 ms — about 11 ms of
the GPU frame spent on four lights that light nothing.

`Renderer.cpp` (the shadow pass loop, `scene.forEachRenderToShadowMap(…)`) only asks a shadow map whether it is
ready for rendering, never whether the light it belongs to is enabled.

## What remains

- Skip the shadow map of a disabled light (the loop, or `forEachRenderToShadowMap()` itself), and make sure a light
  switched back on renders its map BEFORE its first lit frame (a stale map from the moment it was switched off would
  shade with the wrong occluders).
- Prove it: the same `basic-scenery` A/B, the zone's sample count frozen while the light is off, 0 VUID.

## ⚠️ Traps

- `getGPUTimings()` reports cumulative AVERAGES and there is no reset command: compare `last` values, several reads.
- The GPU profiler's `Frame` zone does NOT contain the shadow map zones: add them to get the GPU frame.

## References

- `src/Graphics/Renderer.cpp`, the shadow map loop (`forEachRenderToShadowMap`).
- `docs/subsystems/graphics/38-deferred-punctual-lights.md` (a shadowed light stays forward).
