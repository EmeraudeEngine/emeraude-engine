---
id: dynamic-sky-aerial-perspective
title: Dynamic sky — aerial perspective, fog and clouds lit by the same atmosphere as the sky
status: in-progress
priority: unranked
scope: Graphics/Effects/Atmosphere (AtmosphericFog, VolumetricClouds, VolumetricScattering), Graphics (atmosphere LUTs)
opened: 2026-10-24
tags: [sky, atmosphere, fog, clouds, design]
---

# Dynamic sky — aerial perspective, fog and clouds lit by the same atmosphere as the sky

## Why

Under a dynamic sky, a distant mountain must turn blue by day and pink at sunset, and a cloud must
be lit by the sun's REAL colour and by the sky's ambient. `AtmosphericFog`, `VolumetricClouds` and
`VolumetricScattering` take the sun as a colour × lux today (`VolumetricClouds.hpp:153-156`), with
their own models. If they keep separate models, the horizon does not match the sky drawn behind
it.

## Owner decisions (2026-10-25)

1. **Under a dynamic sky the aerial perspective (AP) owns the distance haze**; `AtmosphericFog` keeps the LOCAL medium
   only (a valley, a marsh: the scene's `ParticipatingMedium`, height fog) — never two models for one thing.
2. **RGB transmittance** (a distant mountain loses more blue than red), not Unreal's mean: the application is a
   fullscreen pass that reads the scene colour, so it costs a second 32³ volume only.
3. **A new chain slot `AerialPerspective`**, after the geometry (translucents included, at the depth they wrote — the
   fog's present rule) and BEFORE `Clouds` and `Fog`; the `Fog` slot stays free for the local medium.
4. **Lot 4a** = the volume, its application, distant test objects in sky-lab (1 to 9 km) and the horizon seam;
   **lot 4b** = the clouds lit by the atmosphere (the sun at their altitude, the sky's ambient).

References: S. Hillaire, EGSR 2020, and its reference code (`RenderSkyRayMarching.hlsl`: 32 slices of 4 km — 128 km —
in a squared distribution, 2 × (slice + 1) samples per slice, in-scattering RGB + opacity, a fade over the first half
slice); Unreal's `SkyAtmosphere`. Distances are reconstructed through the inverse view-projection (the clouds' way),
never through the chain's clip planes (item `post-process-depth-linearisation-wrong`).

## What remains

- **Lot 4a is DONE (2026-10-25)**: the volume, its application (`Effects/Atmosphere/AerialPerspective`, slot
  `AerialPerspective`), sky-lab's distant targets (option 7), the horizon seam. The knowledge is in graphics doc 42
  § "The aerial perspective". T is measured within ~1 % of a CPU integration; the seam holds by construction (one
  integrator).
- [ ] **Lot 4b, the clouds**: their sun term reads the transmittance toward the sun at the cloud's altitude, and their
      ambient term reads the sky (its irradiance, baked amortised since 2026-10-25: graphics doc 42 § "The
      environment"). The clouds composite AFTER the aerial perspective (slot order), so a far cloud is not hazed yet:
      apply the volume at the cloud's depth too (its own distance, read in the cloud pass).
- Peers GREEN (2026-10-25): macOS M6 (MoltenVK: the 3D storage images work) and Windows (RTX 3500 Ada + Intel iGPU).
  T within 0.006 / 0.008 of Linux; 0 VUID. Cost:
  - Intel iGPU at 720p: the volume avg 0.63 ms (max 4.6), the pass 0.36 ms, ~3.5 % of its frame;
  - M6 at 1440p: 0.057 + 0.090 ms;
  - RTX 3500 Ada: 0.030 + 0.029 ms.
- [ ] **The volume's cost on an iGPU** (non-defect, owner's ranking): every froxel re-marches from the camera
      (Hillaire's reference: Σ 2(s + 1) = 1 056 samples per column). A cumulative march per column, each slice
      continuing the previous one, would take ~64 samples per column. Measure it before / after on the Intel iGPU.

## References

- the physical sky, lot 1 (graphics doc 42, `42-physical-sky-atmosphere.md`, § "The environment").
- `src/Graphics/Effects/Atmosphere/`, `docs/post-processing-pipeline.md`.
