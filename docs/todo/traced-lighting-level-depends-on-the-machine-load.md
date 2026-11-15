---
id: traced-lighting-level-depends-on-the-machine-load
title: The ray-traced lane's picture depends on the machine's CPU load — the metered scene average moves from 107 to 118 nits
status: open
priority: high
scope: Graphics (the indirect diffuse chain — Effects/Lighting/RTGI, GIDenoiser, IrradianceProbeVolume — and the auto-exposure of Effects/Camera/ToneMapping); to isolate
opened: 2026-11-15
tags: [defect, wrong-result, frame-rate-dependence, global-illumination, measurement]
---

# The ray-traced lane's picture depends on the machine's CPU load — the metered scene average moves from 107 to 118 nits

## Why

Measured on 2026-11-15 (Linux, RTX 3070 Ti, Debug build, window-less 1280×720, validation layer loaded) on the testbed's
`global-illumination` scene, ray-traced lane (RTGI, RTR, RTAO; a static scene, a static camera):

| Machine | Metered scene average (`PostProcess.getStatus`) | Sensitivity | The captured picture (mean level / 255) |
|---|---|---|---|
| idle | 106.9 to 107.3 nits | ISO 932 to 935 | 121.6 to 122.5, within one level over 31 s (40 captures) |
| thirty busy processes | 112.0 nits 10 s after the start, 118.3 nits 30 s later | ISO 893, then 845 | 119.5, then down to 115.2 in 28 s (30 captures) |
| a build running beside it | not read | not read | a step of −2.4 levels between two captures 0.6 s apart, uniform over the image |

The scene's content does not change: the same render statistics (19 instances, 266 triangles), the same post-process
occupants. What changes is how many frames the renderer and the logic get per second. A lighting result that follows
the frame rate is a wrong result: the same scene must converge to the same radiance on a slow machine.

Owner decision, 2026-11-15: a defect, `high`.

## What remains

- Isolate which loop carries the dependence — nothing was isolated yet. Three leads, each with its own test:
  1. **The irradiance probe volume** (the RTGI's multi-bounce feedback reads it): a hysteresis or an update budget
     counted per FRAME, not per second, gives a steady state that depends on the frame rate when the loop gain is not
     one. Test: `PostProcess` with the bounce feedback off, idle against loaded.
  2. **The temporal accumulation of the GI** (`GIDenoiser`: history, anti-lag): a clamp or an anti-lag that fires on
     long frames changes the accumulated energy. Test: the anti-lag variant off, idle against loaded; the temporal
     discontinuity flag (`Renderer::TemporalDiscontinuityFrames`) counted in both.
  3. **The exposure loop** (`ToneMapping`: the metering's EMA, the pre-exposure): the metered AVERAGE itself rises
     under load (107 → 118 nits), so the cause is upstream of the exposure, unless the pre-exposure feeds back into
     the radiance it meters. Test: a pinned exposure (`Camera.setExposure()`) and the scene average read again.
- Reproduce on the screen-space lane (SSGI) and on a scene without indirect diffuse: does anything else move?
- Proof of a fix: the metered scene average and the mean level of the capture equal, idle against loaded, within the
  idle run's own spread (one level over 30 s).

## ⚠️ Traps

- The load must be deliberate and constant to measure (busy processes started and stopped by PID): a build beside the
  instance gives bursts, hence steps.
- A pixel A/B taken while a build or a gate runs is void on this lane (pipeline doc § 3d): this item is why.
- The logic thread slows down under load too (its cycle overruns are traced): a time-driven animation would also move
  the picture — `global-illumination` has none, check any other scene before using it.

## References

- `docs/post-processing-pipeline.md` § 3d — the measurement, as a trap of the method.
- `src/Graphics/IrradianceProbeVolume.*`, `src/Graphics/GIDenoiser.*`, `src/Graphics/Effects/Lighting/RTGI.*`,
  `src/Graphics/Effects/Camera/ToneMapping.*`.
