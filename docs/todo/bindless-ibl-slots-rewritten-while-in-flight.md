---
id: bindless-ibl-slots-rewritten-while-in-flight
title: The reserved IBL slots of the one bindless descriptor set are rewritten while submitted frames read them
status: open
priority: high
scope: Graphics (BindlessTextureManager, the IBL readers), Scenes (BindlessTextureSet, the IBL rings)
opened: 2026-11-03
tags: [vulkan, descriptors, ibl, frames-in-flight]
---

# The reserved IBL slots are rewritten while submitted frames read them

## Why

The engine has ONE bindless descriptor set (`BindlessTextureManager`, `UPDATE_AFTER_BIND` + `PARTIALLY_BOUND`).
`syncTextureSet()` rewrites its three reserved cube slots (`EnvironmentCubemapSlot` 0, `IrradianceCubemapSlot` 1,
`PrefilteredCubemapSlot` 2) in place each frame from `Scenes::BindlessTextureSet`. A frame already SUBMITTED but not
yet executed then samples the NEW set. The IBL rings (`SkyEnvironment::RingSize`, `Scene::IBLRingSize`, 6 sets) keep
the IMAGES alive and untouched, not the DESCRIPTORS. Measured: AMD RADV (LinuxB-PA, 2026-11-03, 4 frames in flight,
GPU-bound), a time-of-day jump in `city` — the two frames submitted before the jump show the day sun and sky over a
night ambient, IBL and reflections (land 224 against 251), unmarked, in 8 of 8; at 8 FPS (nothing pending) never.
TAA keeps them (the 0.20 residue, graphics doc 13-12 § 43). No VUID fires. The comment of `clearTextureSet()` calls
the in-flight rewrite safe.

Owner decision 2026-11-03: **each IBL set gets its own permanent slots (a ring)**, written once; the frame carries
the index of the set it reads.

## What remains — the inventory (2026-11-03, before any code)

- **Readers bake the slot into their GLSL text as a constant.** None takes it from a push constant or a UBO field.
  - Raster: `Saphir/LightGenerator.cpp:974` (the ambient's IBL diffuse, slot 1);
    `Material/StandardResource.cpp` reflection (:4399, :4409, slot 2), refraction (:4472-4474, :4483, :4491, slot 0),
    transmission (:4552, :4559, slot 2), clear coat (:5334, slot 2).
  - Post-process and compute declare their own literal: `SSR.cpp:544` (slot 2), `SSGI.cpp:390` (slot 1, sky
    visibility only), `RTGI.cpp:164` (slot 0), `RTR.cpp:900/910/976` (bare `nonuniformEXT(2)` / `(1)`),
    `IrradianceProbeVolume.cpp:121` (slot 0), `VolumetricClouds.cpp:92` (slot 1).
- **Where the index can ride:**
  - the raster view block (`Generator::Abstract::declareViewUniformBlock`; `ViewMatrices2DUBO` / `3DUBO` /
    `CascadedUBO`: one more vec4);
  - each effect's own per-frame block: SSR `ResolvePushConstants`, SSGI `TracePushConstants`,
    `GIDenoiser::FrameUBOData` (RTGI), `RTR::TraceFrameUBOData`, `VolumetricClouds::FrameBlock` (`ambient.w` is
    spare), `IrradianceProbeVolume::ParametersUBO`. Several carry size `static_assert`s.
- ⚠️ **Timing:** the view block is filled from the logic state published per slot, but the set a frame reads is decided
  on the RENDER thread (`syncTextureSet()`, `adoptSkyEnvironment()`). The index must be written render-side, for the
  frame being recorded.
- ⚠️ **Capacity:** 6 sets × 3 cubes = 18 slots — more than the default reserved region (`FirstDynamicSlot` 16) and than
  the whole cube array of the minimal profile (14). Size the ring on the frames actually in flight (+ 1), and grow the
  reserved region with it (`ReservedSlotCount`, `MinimalTexturesPerDynamicArray`, `BindlessTextureSet::setCapacities()`).
- The picture skies' bake (`Scene::updateEnvironmentIBL()`, `m_IBLIrradiance` / `m_IBLPrefiltered`) and a picture
  sky's environment slot 0 (swapped in place by `setBackground()`) join the same scheme.
- Then measure: AMD's in-capture jump (TAA off) shows no frame mixing the day sun with the night IBL; R2 at 40 FPS
  shows the sea at 0.00 at frame #0 in every jump.

## ⚠️ Traps

- Other slots rewritten while frames are in flight, out of this item's scope:
  - the animated textures' per-frame 2D view (`syncTextureSet()`, `writeRawToDescriptorSet()`);
  - `clearTextureSet()`, which also returns early, without resetting the reserved slots, when the set has no 2D or
    cube entry;
  - `Renderer::refreshGrabPass()`.
  Same class; decide with the owner once this one is done.
- Read the exact Vulkan rule (`UPDATE_AFTER_BIND` against `UPDATE_UNUSED_WHILE_PENDING`, `VUID-vkUpdateDescriptorSets-None-03047`)
  before writing the item's conclusion in `docs/`.

## References

- Graphics doc 13-12 § 43 (AMD round 6), graphics doc 42 § The environment, scenes doc 16.
