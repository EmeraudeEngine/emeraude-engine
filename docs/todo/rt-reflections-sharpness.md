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

Steps 1-2 (reproduce at a fixed pose, sweep the knobs) are DONE on the `post-processor-effect-debug`
bench of projet-alpha (2026-08-30; protocol and full table in projet-alpha `src/Builtin/AGENTS.md`
§ "post-processor-effect-debug"). Kernel σ of the reflected band, in pixels at 1620 rows, camera 4 m
from six white metal panels, emitter 6 m behind them (near) / 16 m (far):

| roughness | 0.1 | 0.2 | 0.3 | 0.4 | 0.5 | 0.6 |
|-----------|-----|-----|-----|-----|-----|-----|
| v1, near (defaults) | 0 | 8 | 27 | 52 | 70* | 71* |
| v1, far | 0 | 5 | 20 | 42 | 51* | 50* |
| v1, `GlossyCone/Enabled = false` | 0 | 0 | 2 | 3 | 5.5 | 7 |
| v1, `PixelDoubling = false` | 2 | 12 | 29 | 52 | 71* | 72* |
| GGX expectation (α = r², footprint at the hit re-projected), near | 6 | 23 | 53 | 93 | 145 | 209 |
| GGX expectation, far | 8 | 31 | 70 | 124 | 193 | 277 |

`*` = limited by the 1.4 m panel height (energy loss 8-10 %, centroid drift), not by the effect.

What the table says:

- **The glossy cone is 100 % of the blur.** With the cone off the reflection is mirror-sharp at
  every roughness; the shared bilateral denoiser adds ≤ 7 px at r = 0.6 (`blurRadius 12 × r²/0.49`).
- **`PixelDoubling` is not a sharpness knob**: the cone is a fixed FRACTION of the trace height and
  the pyramid base is trace/2, so full-res trace = same screen-space blur for more milliseconds.
- **The blur ignores the hit distance by construction** — a uniform screen-space cone
  (`2 · HitFraction 0.15 · traceHeight · r²`). Physics wants the FAR band 1.33× blurrier
  (`dHit / (dCamera + dHit)`: 0.8 vs 0.6); v1 gives it 25 % SHARPER (grid alignment of the coarse mip
  the band falls into — the kernel is shift-variant, i.e. "blocky", another face of the same defect).
- **Versus GGX, v1 under-blurs 2-3× in this geometry** (hit 6-16 m behind a panel seen from 4 m) and
  OVER-blurs any hit closer than ≈ 0.27 · dCamera — contact reflections (a column base on the
  floor) which physically stay sharp. Both Sponza symptoms — bright smeared "pâtés" near contacts,
  soft distant reflections — are that one missing term.

3. **Engine fix: a per-pixel hit-distance-driven cone.** The trace writes `hitT`; the resolve sizes
   the cone from `tan(1.29 · α) · hitT · dCamera / (dCamera + hitT)` projected to pixels (with
   `α = r²`, the GGX half-max in reflected-direction space), then picks the pyramid LOD from that.
   Acceptance on the bench: σ within ±25 % of the GGX row at r = 0.2-0.4, far/near ratio ≈ 1.3,
   energy conserved (≤ 3 % at r ≤ 0.3 as today). Lengthen the panels (2.4 m) or move the camera
   back before reading r ≥ 0.5.
4. Then decide with the owner whether the pyramid resolution (base = trace/2) is enough for a
   distance-driven cone, or whether a small-radius stochastic filter must replace the coarsest mips.

## ⚠️ Traps

- ⚠️⚠️ `RayTracing/Reflection/Enabled` was REMOVED (Aug 2026); ray tracing is a global
  switch now. To compare against no reflections at all, leave the `Reflections` slot EMPTY
  in a test stack — do not expect a key to do it.
- ⚠️ Disabling ray tracing does not remove the reflections: the demos fall back to SSR,
  which shimmers (half-res, 2-texel blur, rays leaving the screen). The owner saw it and
  read it as a new defect. Compare RTR against RTR, never against the fallback.
- ⚠️ `cp "$(ls -t captures | head -1)"` serves the PREVIOUS capture when the screenshot
  fails — two bit-identical images read as "no effect". Check the file is newer than the run.

- ⚠️ The bench camera was a free player on day one — a chat message typed into the focused window
  moved it and one capture had to be redone. It is a FIXED camera now (`enableFixedCamera`, no
  actor, no listener); the run script still checks `Act.getOrientation()` before every capture.
- ⚠️ Read the profiles on `capture(on) − capture(off)` (emitter replaced by a matte plate, option
  2 = 1) after inverting the tone curve (Narkowicz ACES fit, gamma 2.2): the panel also reflects
  the floor/ceiling gradient, and the ACES curve compresses the sharp band's peak by 30 % relative
  to a spread hill — raw LDR profiles gave a non-monotonic FWHM.

## References

- `dependencies/emeraude-engine/src/Graphics/Effects/Framebuffer/RTR.cpp` — the glossy cone
  lookup (`GlossyCone` push constants) and the half-res trace target.
- projet-alpha `src/Builtin/PostProcessorEffectDebug.cpp` — the bench (`--load-demo
  post-processor-effect-debug --demo-options 0,<distance>,<emitterOff>`).
- `dependencies/emeraude-engine/src/SettingKeys.hpp` § "Ray Tracing > Reflection".
- `dependencies/emeraude-engine/src/Graphics/AGENTS.md` § RTR, § EffectSlot (the order part
  of the report, fixed).
