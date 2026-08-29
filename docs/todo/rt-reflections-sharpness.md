---
id: rt-reflections-sharpness
title: Ray-traced reflections — soft bright patches on large rough surfaces
status: open
priority: medium
scope: Graphics/Effects/Framebuffer/RTR
opened: 2026-08-29
tags: [vulkan, ray-tracing, reflections, measurement]
---

# Ray-traced reflections — soft bright patches on large rough surfaces

## Why

Owner report on Sponza (2026-08-29): "la réflexion éjecte des gros pâtés lumineux partout,
disgracieux" — large soft bright patches instead of legible reflections on the big rough
walls. Part of it was a chain-order regression fixed the same day (the ambient occlusion had
stopped attenuating the reflections — see `Graphics/AGENTS.md` § EffectSlot). What remains is a
PRECISION matter, independent of the order.

Two knobs shape the softness, both already exposed as settings:

- `Core/Graphics/RayTracing/Reflection/PixelDoubling` (default `true`): the trace runs at
  HALF resolution. Full resolution was measured at **+9.8 ms @ 4K** on the RTX 3070 Ti
  (2026-07-05, RTR + RTAO both full-res) — the owner's GPU budget is deliberate.
- `Core/Graphics/RayTracing/Reflection/GlossyCone/{MaxLod 8, BlendStartTexels 2,
  BlendFullTexels 24, HitFraction 0.15}`: the v1 glossy cone is UNIFORM in screen space —
  it assumes a representative hit distance and ignores surface curvature — and reaches up to
  mip 8 of the reflection pyramid, which on a wide rough wall is a very large blur.

## What remains

1. Reproduce at a FIXED pose (read it back with `Act.getOrientation()` into the capture
   log) and quantify: a local-contrast metric on the reflective region, not a frame mean.
2. Sweep the knobs one at a time, same pose, same procedure: `PixelDoubling` off, then
   `GlossyCone/MaxLod` 8 → 6 → 4, then `BlendFullTexels`. Frame time from the GPU
   profiler scopes for each.
3. Decide with the owner: a cheaper default, or a better estimator (a cone driven by the
   ACTUAL hit distance and the roughness — the v1 comment in `SettingKeys.hpp` already names
   the limitation).

## ⚠️ Traps

- ⚠️⚠️ `RayTracing/Reflection/Enabled` was REMOVED (Aug 2026); ray tracing is a global
  switch now. To compare against no reflections at all, leave the `Reflections` slot EMPTY
  in a test stack — do not expect a key to do it.
- ⚠️ Disabling ray tracing does not remove the reflections: the demos fall back to SSR,
  which shimmers (half-res, 2-texel blur, rays leaving the screen). The owner saw it and
  read it as a new defect. Compare RTR against RTR, never against the fallback.
- ⚠️ `cp "$(ls -t captures | head -1)"` serves the PREVIOUS capture when the screenshot
  fails — two bit-identical images read as "no effect". Check the file is newer than the run.

## References

- `dependencies/emeraude-engine/src/Graphics/Effects/Framebuffer/RTR.cpp` — the glossy cone
  lookup (`GlossyCone` push constants) and the half-res trace target.
- `dependencies/emeraude-engine/src/SettingKeys.hpp` § "Ray Tracing > Reflection".
- `dependencies/emeraude-engine/src/Graphics/AGENTS.md` § RTR, § EffectSlot (the order part
  of the report, fixed).
