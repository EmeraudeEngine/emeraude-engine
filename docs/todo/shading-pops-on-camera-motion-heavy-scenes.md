---
id: shading-pops-on-camera-motion-heavy-scenes
title: Shading pops in and out while the camera moves, on heavy scenes only (Sponza)
status: open
priority: unranked
scope: Graphics (RT lane: IrradianceProbeVolume, GI denoisers) / Scenes (shadow path)
opened: 2026-09-13
tags: [rendering, temporal, ray-tracing, shadows, sponza]
---

# Shading pops in and out while the camera moves, on heavy scenes only (Sponza)

## Why

Owner report (2026-09-13, `sponza`, after the animated-sun work): "quand la caméra bouge, il y a des
ombrages qui disparaissent et réapparaissent, on dirait qu'il y a encore des choses qui ne sont pas
synchronisées pour le rendu à temps. Ça ne se voit que sur des scènes surchargées." Parked by the owner
for a later session.

## What is known

- **Excluded**: the classic shadow map's caster culling. `ShadowMap::viewDistance()` returns the
  projection far plane (2 × coverage, 400 m on Sponza), so the `distance > viewDistance` test is
  consistent with the frustum; the shadow list is built from the PUBLISHED state (`readStateIndex`).
- **Measured, no transient ≥ 1.5 s after a teleport** (console `setPosition`, exposure pinned,
  same final pose, Sponza option 0, RTX 3070 Ti): from +3.3 s on, both lanes match their settled
  frame within the noise floor (RT lane mean |Δ| 0.63 → 0.18/255 with a slow tail; SS lane 0.39 flat,
  floor 0.3). The console screenshot latency (~1.5 s per capture) cannot see the sub-second window
  where the report lives; the first capture after a move still shows the OLD pose.
- **Two mechanisms are camera-dependent by construction** and act inside that window:
  1. `IrradianceProbeVolume` is anchored on the camera's grid cell (1.5 m spacing): every cell
     crossing scrolls the volume and RESETS one plane of probes (8 × 16 on Sponza's 16 × 8 × 16
     volume), which restart from their first raw estimate and re-converge with hysteresis 0.97
     (~1 s). RTGI (multi-bounce) and RTR (reflection hits) read the probes: indirect shading near a
     reset plane pops.
  2. The temporal denoisers of the RT lane (RTGI / RTAO / RTR history) reject history on
     disocclusion; disoccluded pixels show the raw estimate for a few frames.
- The 4.6× brightness gap between the two lanes seen during these measurements is a SEPARATE
  defect with its own item: `screen-space-lane-lacks-sky-visibility.md`.

## What remains

1. Discriminate live (the owner's eyes, 10 seconds): **KeyPad9** switches the lane. Popping gone in
   ScreenSpace ⇒ RT lane (probes or denoisers); still there ⇒ raster / shadow path (state desync class,
   see `docs/caution-points.md` § shadow flicker). Then relaunch with
   `Core/Graphics/RayTracing/IrradianceProbes/Enabled = false` to split probes from denoisers.
2. Build a sub-second instrument: a scripted continuous move (console `keyPress` injection or a
   short RushMaker recording) with per-frame mean luma and per-frame |Δ| against the converged frame —
   the existing screenshot path is too slow (`docs/temporal-stability-measurement.md` § 4 warns that a
   screenshot cannot catch a one-frame event).
3. If the probes are guilty: initialise an arriving plane from its neighbours (or a shorter
   hysteresis for the first frames of a reset probe) instead of the raw single-frame estimate.

## ⚠️ Traps

- "Camera in motion" names a CLASS of causes, not one — four independent mechanisms were separated
  on 2026-08-26 (`docs/caution-points.md`, shadow flicker). Vary speed and resolution before attributing.
- A console teleport produces exactly ONE frame of velocity; a screenshot series after it measures
  convergence, never the event itself.

## References

- `src/Graphics/IrradianceProbeVolume.cpp` (`m_resetPlanes`, cell anchoring), `src/Graphics/Effects/Lighting/RTGI.cpp`.
- `src/Scenes/Scene.rendering.cpp` `populateShadowCastingRenderList()`.
- Memory of the session: `project_sponza_as_intel_intended.md` (measurement scripts in the session scratchpad are gone with it).
