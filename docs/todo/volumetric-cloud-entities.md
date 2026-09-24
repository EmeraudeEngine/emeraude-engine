---
id: volumetric-cloud-entities
title: Volumetric clouds as hand-placed scene entities the camera can fly through
status: in-progress
priority: unranked
scope: Graphics, Scenes/Component
opened: 2026-09-24
tags: [participating-medium, shaders, procedural, voxels]
---

# Volumetric clouds as hand-placed scene entities the camera can fly through

> [!IMPORTANT]
> **Owner decisions (2026-09-24), do not re-litigate:**
> - **Fly-through clouds**, not a sky layer seen from below: the VOXEL family (Nubis³), not the
>   weather-map shell of Horizon Zero Dawn / Nubis 2017.
> - **A cloud is an ENTITY placed by hand**: a component on a node. Position, orientation and
>   scale come from the node and the effect follows them every frame.
> - **The shape comes from a PROCEDURAL generator** in the engine (seed + size + style → voxel
>   grid), the way `TreeStock` grows trees. No external tool. The runtime format (a voxel grid in
>   a 3D texture) does not depend on its source, so a VDB import can come later without touching
>   the renderer.
> - **A few HERO clouds** (32 drawn at most), not a cloud field: every ray tests the box of every
>   drawn cloud. No world-space voxel clipmap.
> - **Crossing a cloud is a realistic WHITE-OUT**: a dense, opaque interior.
> - **Scaling a cloud KEEPS ITS LOOK**: the extinction follows the cloud's height.
> - **The pass is a post-process effect in its own slot** (`EffectSlot::Clouds`, before `Fog`).
> - **It is automatic**: placing a cloud is enough; the stack files the pass when the scene holds one.
> - **The clouds' shadows on the world are STAGE 2**, measured on their own. Within it (2026-09-24):
>   the shadow term is **carried by the light** (matrix + bindless slot in the directional light's
>   block), and the **lit materials come first** — `VolumetricScattering`, cloud-to-cloud and the
>   ray-traced lanes later.
> - **Stylised placement is wanted** in `forest`: clouds low, touching the canopy, "4-5× bigger"
>   than the first version. Not realistic — a "cute", fairy-tale look.

## Why

The owner wants to fly among clouds in `forest`, some of them just above the trees.

## Stage 1 — DELIVERED 2026-09-24 (not committed)

The whole of it is described in `src/Graphics/AGENTS.md` § VolumetricClouds and
`src/Scenes/AGENTS.md` § Available Components; the traps met are in `docs/caution-points.md`
(a 3D image uploaded one slice; a late-filed post-process occupant was never created).

- `Graphics::CloudShapeResource` (dome + budding puffs, R8G8 density + skip distance, full mips),
  `Scenes::Component::CloudVolume`, `Scenes::CloudSet`, `AbstractEntity::CloudVolumeCreated/Destroyed`,
  `PostProcessStack::syncSceneEffects()`, `IndirectPostProcessEffect::requiresCloudVolumes()`,
  `FrameContext::clouds`, the `VolumetricClouds` effect, `Toolkit::generateCloud()`, the 3D bucket
  of `BindlessTextureSet`, the `Core/Graphics/PostProcessing/Clouds/*` keys; emeraude-base
  `Algorithms::WorleyNoise` (+ tests).
- `forest`: seven hero clouds, option 1 = 0 = none.
- Measured (RTX 3070 Ti, 2880×1620, validation ON, 0 VUID): 1.8 ms at the `forest` opening view,
  5.3 ms with the camera inside a cloud (13.3 ms before the optical-depth-adaptive step).
- Verified at runtime: the editor (Shift+F3) selects a cloud and draws its gizmo; the cloud stays
  traversable.

## Stage 2 lot 1 — the lit materials — DELIVERED 2026-09-24 (not committed)

Described in `src/Graphics/AGENTS.md` § *The clouds' shadow on the world*, `src/Saphir/AGENTS.md`
§ *The volumetric clouds' shadow is a BRANCH*, `src/Scenes/AGENTS.md` (CloudVolume paragraph).

- `Graphics::CloudShadowMap` (Beer shadow map, Hillaire 2016: front depth / mean extinction / total
  optical depth, RGBA16F, 1024² over 1024 m, 48 union-march steps at shape mip 1), owned by
  `Scenes::CloudSet`, recorded before the scene pass; `DirectionalLight::updateCloudShadow()` (frame
  snapped to the texel grid) called for the main sun by `Scene::updateCloudShadows()`; the lookup is a
  uniform branch in every PBR directional variant (no new pass type — signalled to the owner);
  `Effects/Shared/CloudVolumeGLSL.hpp` = the ONE cloud description both passes read;
  `Clouds/ShadowsEnabled|ShadowResolution|ShadowCoverage`.
- Measured on `forest` (3070 Ti, 2880×1620, validation ON, 0 VUID): map pass 0.20 ms; the lookup below
  the scene pass's run-to-run noise; the ground in a cloud shadow at 0.31-0.35 of its sunlit
  luminance, 1.004 outside (A/B across two runs, pinned sunny-16, scene-referred).

## What remains

1. **Stage 2, the other receivers**: `VolumetricScattering` (the shafts ignore the clouds' shadow),
   and — to decide — the DDGI probes and RTGI's sun term at its bounce hits (neither reads the map, so
   the indirect light under a cloud is still the sunlit one).
2. **Placing from a JSON scene and from the console** (only C++ `Toolkit::generateCloud()` and the
   editor gizmo exist): a JSON component key and a `SceneManagerService` command.
3. **Cost**: the pass is full resolution. Half resolution + a depth-aware upsample + a temporal
   accumulation is the known lever (the Horizon Zero Dawn reprojection) — measure the need first
   (5.3 ms inside a cloud is the worst case measured).
4. **Calibration of the lighting against a physical target**, not by eye: a thick cloud's plane
   albedo (two-stream, conservative scattering, `R ≈ (1−g)τ / (2 + (1−g)τ)` ≈ 0.69 at τ = 30,
   g = 0.85) says the sunlit top should read ~`0.69 · E⊥ / π`; the 4-octave approximation gives less,
   and the isotropic ambient term re-emits the full sky radiance (a reflectance of 1). Measure at a
   pinned exposure, then decide the octave count and the ambient occlusion.
5. **A cloud does not shadow another one** (its sun optical depth is marched inside itself only) —
   the Beer shadow map now exists and is the natural input: the view march's light term could read
   it for the part of the sun path OUTSIDE the current cloud.
   Overlapping clouds ARE marched as one medium since 2026-09-24 (the per-cloud march left a straight
   seam where two box entries met).
6. **The ray-traced lanes do not see the clouds** (not in the TLAS): RTR reflections and RTGI rays go
   through them. The known path is AABB procedural geometry with an intersection shader.
7. **Owner questions, open**: pin `forest`'s exposure at sunny-16 (its auto exposure washes the sky
   and the clouds out); fold `WorleyNoise` and `VoronoiNoise` into one cellular noise (emeraude-base
   `src/AGENTS.md`). (`forest`'s sun IS the painted one since 2026-09-24: the manifest was re-measured
   and the demo's own sun removed.)

## ⚠️ Traps

- ⚠️⚠️ **A miniature cloud needs a scaled optical depth**: a real cumulus (~0.05 m⁻¹) is barely
  visible over 15 m. The look is `Look::opticalThickness` (vertical τ), never a density in 1/m.
- ⚠️⚠️ **Judge the clouds at a PINNED exposure**: an auto-exposed `forest` frame puts them in the
  tone mapper's shoulder (247/255 top and base alike).
- ⚠️ **A silent pass cannot be debugged**: read the census line (`Clouds drawn: N of M (...)`); no
  line means `execute()` never ran.
- ⚠️⚠️ **A low-sun cloud shadow is a long band with straight, parallel sides** — the geometry (a
  29° sun stretches it ×2 along its horizontal direction), not a clipped map. Measure the direction of
  an edge before suspecting the map (`src/Graphics/AGENTS.md` § *The clouds' shadow on the world*).
- ⚠️ **The shadow is judged by a DIFFERENTIAL capture** across two runs (`Clouds/ShadowsEnabled` is
  read once), same pose, pinned exposure — the trees and their CSM shadows hide it on a single frame.
- ⚠️ **A log redirected to a file is block-buffered**: `stdbuf -oL -eL` or the traces arrive late
  (projet-alpha `docs/runtime-session.md` § 2).

## References

- Schneider, *Nubis³: Methods (and Madness) to Model and Render Immersive Real-Time Voxel-Based
  Clouds*, SIGGRAPH 2023, Advances in Real-Time Rendering.
- Schneider & Vos, *The Real-time Volumetric Cloudscapes of Horizon Zero Dawn*, SIGGRAPH 2015,
  Advances in Real-Time Rendering (billowy Worley, the erosion remap, the reprojection).
- Bouthors & Neyret, *Modeling Clouds Shape*, Eurographics 2004 (short paper) — the hierarchical growth.
- Hillaire, *Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite*, SIGGRAPH 2016,
  Physically Based Shading course (energy-conserving integration).
- Wrenninge, Kulla & Lundqvist, *Oz: The Great and Volumetric*, SIGGRAPH 2013 Talks
  (multiple-scattering octaves).
- Worley, *A Cellular Texture Basis Function*, SIGGRAPH 1996.
- Quilez, *Smooth minimum* (2013) and *Ellipsoid SDF* (2019), iquilezles.org.
- Jendersie & d'Eon, *An Approximate Mie Scattering Function for Fog and Cloud Rendering*,
  SIGGRAPH 2023 Talks (a candidate phase function for the calibration).
- Museth, *NanoVDB*, SIGGRAPH 2021 Talks (the voxel format, if an import is ever added).
