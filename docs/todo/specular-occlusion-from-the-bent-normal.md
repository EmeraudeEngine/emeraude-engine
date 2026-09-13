---
id: specular-occlusion-from-the-bent-normal
title: Specular occlusion from the bent normal and V, on the raster's specular IBL leg
status: open
priority: unranked
scope: Graphics (Effects/Lighting SSGI), Saphir (LightGenerator specular IBL leg)
opened: 2026-09-13
tags: [rendering, lighting, screen-space, ibl, specular]
---

# Specular occlusion from the bent normal

## Why

The indirect-diffuse ownership only ever moves the DIFFUSE leg of the raster's IBL: the specular
legs (prefiltered reflection, Fdez-Agüera multi-scatter compensation) keep the raw irradiance,
because no post-process replaces them. A rough metal inside a closed room therefore still reflects
a full, unoccluded environment.

Since Sep 2026 the screen-space lane produces exactly what a specular occlusion needs — a bent
normal and a cosine-weighted visibility V, half-res, per pixel (`SSGI_SkyVisibility`). The standard
term is in Lagarde & de Rousiers, "Moving Frostbite to PBR" (2014), § specular occlusion.

## What remains

1. ⚠️ **The ordering problem is the whole difficulty**: the raster pass cannot read this frame's
   post-process output. Either the term is applied to the PREVIOUS frame's visibility (a reprojection,
   with everything that implies at a disocclusion), or the specular IBL leg moves out of the material
   shader and into the composite — a contract change, not a tweak.
2. Decide which, with the owner, before writing anything.
3. The visibility is currently consumed inside the SSGI trace and never leaves it; exposing it means
   the lane protocol of `docs/todo/ssao-consumes-the-sky-visibility-lane.md`.

## References

- `src/Graphics/AGENTS.md` § "Indirect-diffuse OWNERSHIP" (why the weight scales the diffuse leg only).
- `src/Saphir/LightGenerator.cpp` (the split-sum specular branch).
