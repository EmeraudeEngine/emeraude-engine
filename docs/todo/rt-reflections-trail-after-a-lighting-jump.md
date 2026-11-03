---
id: rt-reflections-trail-after-a-lighting-jump
title: The ray-traced reflections keep a visible trail for ~1.4 s after a lighting jump (Windows RTX)
status: open
priority: medium
scope: Graphics/Effects/Lighting (RTR and its GIDenoiser), the temporal-discontinuity contract
opened: 2026-11-04
tags: [temporal, reflections, ray-tracing]
---

# The ray-traced reflections keep a visible trail for ~1.4 s after a lighting jump (Windows RTX)

## Why

Windows-PA, 2026-11-04, engine eed27ab5, RTX 3500 Ada, RayTracing lane, **TAA off**, `city` at the quay, the
in-capture sunset jump (`temporalCapture(60, 5, "…setTimeOfDay(CitySun0, CitySunClock, 0.95)")`). After the jump,
the sea box (0, 560, 400, 720 at 1280×720) reads 2.6-3.0 on the first night frame (marked), 1.1-1.5 three frames
later, ~0.2 at frame #31 and 0.06-0.10 at #59, in 4 of 4 jumps. It shows as a diagonal line of bright specks on the
water at the lower left. The land converges in about 1 s, as expected.

TAA is off, so the trail is in the rendered input itself. The likely source is the ray-traced reflections' own
temporal history (RTR, denoised by `GIDenoiser` in reflection mode) converging after the step. Linux main's RT lane
(RTX 3070 Ti, 1920×1080) reads 0.0 in the same box: not reproduced there, so the cause is not established.

## What remains

- Reproduce on Windows with `temporalCapture(N, triggerFrame, command)`. Split by switching the reflections off
  (`PostProcess.disable(Reflections)`) and back to RTR, and by `select(Reflections, SSREffect)`: does the trail follow
  RTR?
- If it is RTR's history: read how RTR's denoiser consumes `FrameContext::temporalDiscontinuity` (graphics doc 13-12
  § 43: the anti-lag's pooled variant on marked frames) and why its accumulation keeps the day's reflections.
  "Later consumers (VolumetricLight, RTR accumulation, the clouds) read the same field" is § 43's own note.
- Find why Linux shows none (resolution, the reflection's own settings, the denoiser's parameters).
- The fix is the owner's decision.

## References

- Graphics doc 13-12 § 43 (the open observation, the contract's consumers), § 16 (the GI denoiser's anti-lag).
