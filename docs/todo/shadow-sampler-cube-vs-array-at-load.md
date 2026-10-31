---
id: shadow-sampler-cube-vs-array-at-load
title: A cube-sampling shadow program is drawn once with a 2D-array shadow view during loading (intermittent)
status: open
priority: high
scope: Graphics shadow maps (point / CSM descriptor sets), RenderableInstance light passes, loading order
opened: 2026-10-29
tags: [vulkan, shadows, race, defect, intermittent]
---

# A cube-sampling shadow program is drawn once with a 2D-array shadow view during loading (intermittent)

## Why

macOS-PA (M6, 2026-10-29, alpha 7dd2f467, engine 3fd1d465, base e31c56b): in 1 of 3 launches of projet-alpha
`city 128,1,12,0` with the validation layer, 10 × `VUID-vkCmdDrawIndexed-viewType-07752` in ONE burst during the load:
"the combined image sampler descriptor [Set 2, Binding 1, Index 0, variable "uShadowMapSampler"] VkImageViewType is
VK_IMAGE_VIEW_TYPE_2D_ARRAY but the OpTypeImage has (Dim = Cube) and (Arrayed = 0)" — the same descriptor set each
time: a point-light (cube) shadow program bound with the CSM's 2D-array view. It came just after the
`RenderableInstance{Directional,Point,Spot}LightPass{ShadowMap,CSM,Full}` shaders loaded from the cache and before the
star-field upload and the imposter bakes. The two other launches (one with the duplicate-message limit at 0, 40 s of day
and night) gave 0 VUID; no earlier city 128 run on that machine showed it.

Linux (2026-10-29, engine 208657d1, alpha a9c5e62e): `city 32,1,12,0`, day → night at (0, 60, −700) with physics off,
20 × the same VUID with "(Dim = 2D) and (Arrayed = 0)" — a 2D shadow program (a spot or classic directional map)
bound with the CSM's 2D-array view; 1 run in 4 (the three repeats of the same sequence, 0). The family is the same: a
light pass drawn with another shadow kind's descriptor set, intermittently.

## Found (Linux, 2026-10-31) — a program generated for ANOTHER pass is served from the renderer's program cache

Reproduced: `city 32,1,12,0`, the item's sequence (day 0.55, physics off, (0, 60, −700), night 0.95, back to day), the
validation layer: 1 run in 6, then 1 in 12. Every time right after `setTimeOfDay(0.55)` (sunrise), one descriptor set,
a burst of 8-10 VUID over ~3 s.
A temporary probe in `RenderableInstance::Abstract::render()` (each program is named `"RenderableInstance" +
to_string(passType)` by its generator) caught it: **the program `RenderableInstanceSpotLightPassShadowMap` (sampler2D)
drawn for the pass `DirectionalLightPassCSM`** (the sun's 2D-array CSM descriptor), renderable `CityBuildings6`, layer 8,
the main view. The per-renderable cache (`Renderable::Abstract::findCachedProgram`, a full `ProgramCacheKey` with
`operator==`) only stores what the generator returns; the generator first asks the RENDERER's cache,
`Renderer::findCachedProgram(size_t)`, keyed by `SceneRendering::computeProgramCacheKey()` — a boost-style
`hashCombine` of the render pass handle (an address: different every run, hence intermittent), the renderable NAME, the
layer, the pass type, the flags, the material layout hash and flags — with NO equality check (the only guard compares
the material set layout, identical for two passes of one layer). A spot-shadow program and a CSM program of that layer
got the same `size_t`: the first one generated is served for both. Every pass type is generated for every lit layer at
load (`Scene.rendering.cpp` 2870-2883), so the cube (point) variant explains macOS-PA's "Dim = Cube" the same way.
Program generation runs on the render thread only (`getReadyForRender`, the shadow-casting path): no concurrent write to
`m_programs` found. Probe patch: projet-alpha session scratchpad `shadow-probe.patch` (never committed).

## What remains

- [x] Reproduce; find which light pass draws with the wrong program (above).
- [ ] Owner decision — make the renderer's program cache exact: key it by a full structure with `operator==` (as
  `Renderable::ProgramCacheKey`), or keep the `size_t` and store the full key beside the program to verify a hit, or
  drop the renderer-level cache for scene programs (the per-renderable cache already shares a program between the
  instances of a renderable). Then prove: the hash of the two colliding keys, and N runs with 0 VUID.
- [ ] Rule out the 3fd1d465 change (`ResourceTrait::checkDependencies()` clears the waiting list when every dependency
  is loaded): it only runs once every dependency reports `isLoaded()`.
- [ ] Fix, prove (N launches, 0 VUID).

## References

- macOS-PA's log: `v32p128.stdout.log` lines 91913-92020 (its scratchpad).
