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

## What remains

- [ ] Reproduce (Linux and macOS: repeated city 128 launches with the layer, `VK_KHRONOS_VALIDATION_DUPLICATE_MESSAGE_LIMIT=0`).
- [ ] Find which light pass draws with a descriptor set built for another shadow kind: a program chosen before the
  light's shadow kind is final, or a descriptor set reused across kinds while the light set is being built.
- [ ] Rule out the 3fd1d465 change (`ResourceTrait::checkDependencies()` clears the waiting list when every dependency
  is loaded): it only runs once every dependency reports `isLoaded()`.
- [ ] Fix, prove (N launches, 0 VUID).

## References

- macOS-PA's log: `v32p128.stdout.log` lines 91913-92020 (its scratchpad).
