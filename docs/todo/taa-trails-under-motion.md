---
id: taa-trails-under-motion
title: The TAA leaves trails behind moving foliage and animated characters
status: open
priority: unranked
scope: Graphics/Effects/Resolve/TAA, Graphics/RenderableInstance (skinning history), Scenes/Component/Visual
opened: 2026-09-26
tags: [taa, temporal, ghosting, velocity, skinning, foliage, owner-report]
---

# The TAA leaves trails behind moving foliage and animated characters

## Why

Owner, 2026-09-26, after switching the TAA back on under Linux: *"il fait pleins de bavures quand les
éléments bougent"* — on trees and foliage, on characters, and *"quand je tourne la caméra, en gros c'est
souvent la silhouette des choses qui fait des traînées"*.

Measured the same day (Linux, RTX 3070 Ti, 2880×1620, validation layers on, 0 VUID; captures with
`temporalCapture`, exposure and focus pinned, motion blur neutralised by a 1/8000 s shutter at the same EV
unless stated):

- **Camera turns: the first suspect is the camera's MOTION BLUR, not the TAA.** It is on by default since
  2026-09-13 (owner decision, `MotionBlur/Enabled`). A camera step with it: 25-30 % of the pixels near
  silhouettes above 8/255 for one frame, against 5-7 % without — so on every frame of a continuous turn.
- **A pure camera rotation (1°, 5°) leaves no ghost.** The history follows the camera (stale weight
  w ≈ 0.014); the residual is symmetric (trailing/leading 0.59) and decays at 0.95 per frame, the parked
  blend rate: the history is SOFTENED by the reprojection's resampling and re-sharpens over ~10 frames.
  A blur after a move, not a trail.
- **A sideways step (parallax) leaves a TAA-only residual**: `basic-scenery --demo-options 1`, 1 m: 5 % of
  the edge band above 8/255 right after the step, 2.3 % eight frames later, against the floor from the
  next frame with the TAA off. Same with the lighting lane off (`None`): not a GI history.
- **Animated characters** (`animation-debug`, camera parked): a dotted COMB of old sword-tip outlines,
  ~100 px long, trails the Paladin's sword over the sky, over all 16 captured frames. None with the TAA
  off. Over the white ground it is hidden by the tone mapper's shoulder. The render ran at ~94 fps for a
  60 Hz logic.
- **Foliage in the wind** (`forest --demo-options 0,0,0`, camera parked): the canopy smears into brush
  strokes that bleed into the sky; with the TAA off the leaves are sharp (and aliased).

## What was tried and FAILED — do not retry blindly

**The "provenance tag" (TAG)**, from a 1D scanline model of the resolve that predicted it would fix the
parallax and character trails: write into the history alpha the depth of the surface the history colour
came from (the history's own depth when it was accepted only through the 3×3 range), instead of always
the centre depth. Implemented with a 1 % and a 5 % separation threshold and measured against the current
resolve, **same scene, same pose, same session layout**:

| step (edge band, % above 8/255 at k = 8) | current | TAG 1 % | TAG 5 % |
|---|---|---|---|
| sideways 1 m | 2.30 | 2.31 | 2.39 |
| sideways 0.2 m | 0.40 | 0.37 | 0.40 |
| forward 2 m | 3.18 | 3.12 | 3.15 |
| forward 0.5 m | 1.23 | 1.03 | 1.04 |

No measurable effect; the sword comb and the foliage smear are still there with TAG 5 %, and the relief
parked-shimmer gate was unchanged (0.28-0.29 % against 0.29-0.30 %). **Reverted, never committed.** The
model's "laundering" mechanism (an edge mixture re-tagged with the background depth) is therefore not what
the renderer shows, or not what dominates it.

## What remains

1. **Instrument before designing**: a debug view of the resolve's per-pixel decision (clip or not, which
   neighbour's velocity the dilation took, the history depth against the 3×3 range), to see WHY a comb
   tooth survives 16 frames over a sky whose 3×3 holds no sword. Every fix so far was designed from code
   reading and a model; the one that was built did nothing.
2. **The skinned pose history is archived per LOGIC tick, not per rendered frame** (owner-approved fix,
   2026-09-26, "after the TAA"): the previous pose is archived on each logic update
   (`RenderableInstance/Abstract.cpp:386-387`), staged from the logic thread
   (`Scenes/Component/Visual.cpp:57-65`) and uploaded per frame outside the triple-buffer latch
   (`Abstract.cpp:396-425`). Between two frames with no tick the full tick delta is still reported; with two
   ticks, half of it. Candidate cause of the sword comb (its teeth look stroboscopic). Cheap attribution
   test first: run `animation-debug` capped at the logic rate and see whether the comb goes.
3. **Foliage**: inside a canopy the 3×3 depth range always spans leaf to sky, so the history is almost
   never clipped there (the 2026-09-23 depth rule, by design). Colour rectification while moving (the
   "GATE" option: the clip blended back with the motion) or an FSR2-style lock are the known answers; both
   trade against the parked-camera shimmer the depth rule fixed — an owner decision, to be taken on
   measurements.
4. Found by the same analysis, not reported by the owner yet: particles report zero object motion
   (`ParticlesEmitter.hpp:86`, sprites excluded from motion history `Multiple.cpp:74`); translucent surfaces
   overwrite the velocity and depth of what is behind them (G-buffer blending off,
   `Saphir/Generator/SceneRendering.cpp:1104-1113`).

## ⚠️ Traps

- **`basic-scenery` re-draws its palms at every launch** (the scene randomizer is seeded from
  `std::random_device`, `Scenes/Scene.hpp`): an A/B across two launches compares two different forests.
  The first TAG readings "improved" parallax by 40 % and "worsened" the forward step by 70 % — both were
  palm layouts. Compare two configurations inside ONE launch, or seed the palms for the bench (a local
  `Base::Randomizer< float >{seed}`, never committed without an owner decision).
- **`sponza` cannot bench the TAA after a camera move**: with the TAA off its residual stays high 24
  frames later (the GI histories and the probe volume reconverge), and one repeat in two jumps to 30 %
  when a move crosses a probe cell. Lane `None` first, or another scene.
- **A 1D model is exact only for a vertical edge**: it ranked TAG first, and TAG did nothing. Treat a
  model's ranking as a hypothesis to measure, never as a result.
- The relief parked-shimmer gate at the spawn pose (f/8 · 1/125 s · ISO 100) now reads mean 0.96, 0.29 %
  above 8/255, max 27-29 — not the 0.01 % / max 12 of 2026-09-23 (`src/Graphics/AGENTS.md` § The TAA
  resolve), which was taken on the FAR band. Compare like with like: same crop, same pose.

## References

- `src/Graphics/Effects/Resolve/TAA.cpp` (resolve: dilation at the closest depth, the depth-range test,
  the alpha written = centre depth).
- `src/Graphics/AGENTS.md` § The TAA resolve (the 2026-09-23 depth rule).
- projet-alpha `docs/temporal-stability-measurement.md` § 1d (the camera-step bench and its traps).
