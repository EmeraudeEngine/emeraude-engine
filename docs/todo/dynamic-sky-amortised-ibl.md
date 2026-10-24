---
id: dynamic-sky-amortised-ibl
title: Dynamic sky — the environment cubemap and its IBL bake follow the sky, amortised over frames
status: open
priority: unranked
scope: Graphics/Compute (IBLBaker, ProbeConvolver), Graphics/Renderable (DynamicSkyResource::environmentCubemap), Scenes (Scene.lighting, BindlessTextureSet)
opened: 2026-10-24
tags: [sky, ibl, lighting, performance, design]
---

# Dynamic sky — the environment cubemap and its IBL bake follow the sky, amortised over frames

## Why

The ambient, the reflections and the indirect-diffuse fallbacks all read the scene's environment
cubemap and its three baked IBL slots (irradiance, GGX-prefiltered, BRDF LUT; see
`docs/reflection-pipeline.md` § 2.2). A picture sky bakes them ONCE (`IBLBaker::bakeEnvironment`,
blocking). A dynamic sky changes all day: a shadowed object must turn blue at noon, orange at
sunset and dark at night by itself. `SkyFollowsSun`'s scalar factor is not a substitute here,
since it only scales a fixed picture.

## What remains

- [ ] `DynamicSkyResource::environmentCubemap()`: a small cubemap (64² is likely enough for
      irradiance and the rough mips) rendered from the sky-view model, sun disc INCLUDED or masked
      (`IBLBaker::StarMask` exists for the picture skies' baked suns: the same rule applies, since
      the sun is already a directional light).
- [ ] Re-bake **amortised**, never blocking. The trigger is a sun move over a threshold (~0.5°?), a
      parameter change, or every N frames. The work is spread over frames: one face or one mip
      per frame. `Compute::ProbeConvolver` already records the IBLBaker prefilter into a frame's
      command buffer for the probes: reuse that path, do not write a second one.
- [ ] Publish the new slots without a hazard: double-buffer the IBL textures, or swap only when a
      bake is complete, so a frame never reads a half-baked chain.
- [ ] `ambientIlluminance()` and `averageColor()` of the background follow the new bake (today they
      are read once in `Scene::applyBackgroundLightingNow`).
- [ ] Measure the cost per frame and the visual lag at the threshold chosen.

## ⚠️ Traps

- ⚠️ A once-probe baked before the sky changed keeps the old light: `signalOnDemandRenderTargets()`
  re-bakes them on a background switch (`Scene.lighting.cpp`). Decide whether a sky that changes
  continuously re-signals them (cost) or not (stale probes).
- ⚠️ `hemisphereIlluminanceFactor()` is measured on the source cubemap at load
  (`SkyBoxResource.cpp:260`). For a dynamic sky it must be re-measured with each bake, or the
  ambient lux drifts away from the drawn sky.

## References

- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`), `indirect-diffuse-intensity-scales-the-sky`.
- `src/Graphics/Compute/IBLBaker.hpp`, `src/Graphics/Compute/ProbeConvolver.cpp`,
  `docs/reflection-pipeline.md`.
