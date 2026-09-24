---
id: sky-manifests-star-direction-y-down
title: The 28 sky manifests declare their celestial bodies BELOW the horizon (Y-down legacy)
status: open
priority: unranked
scope: Scenes/Sky, data (projet-alpha.data/data-stores/Backgrounds)
opened: 2026-09-13
blocked-by: [sky-review-28-manifests]
tags: [ibl, sky, y-up, data]
---

# The 28 sky manifests declare their celestial bodies BELOW the horizon

## Why

`Background` manifests describe a star by a `Direction` that points TOWARD the body in the world
frame (UP = +Y — `src/Graphics/AGENTS.md` § Background photometric contract, and
`Scene::applyBackgroundLightingNow()` places the light entity at `direction × 1000` and shines it
along `-normalize(position)`). The 28 manifests authored in July 2026 (Y-DOWN world) carry a
NEGATIVE Y on every sun and moon (`AutumnFieldPureSky` `[0.705, -0.486, 0.517]`, `Clouds`
`[-0.331, -0.587, 0.739]`, `Moon` `[0.161, -0.168, 0.972]`, …) and were never converted at the
Aug 2026 flip. Consequence, measured on `basic-scenery --demo-options 1` (Clouds, 50 klx sun,
2048 shadow map): the sun shines UPWARD, no ground shadow anywhere, the scenes live on the IBL
ambient alone. `AxisDebug` (authored after the flip, `[0.5, 0.5, 0.5]`) and `Kloppenheim05`
(measured by `tools/sky-manifest.py`) are correct.

## Done so far

- **`AutumnFieldPureSky` — re-measured 2026-09-24 (owner decision, for `forest`)** with
  `tools/sky-manifest.py --sun-illuminance 83000 --temperature 5500`: sun `(0.705, 0.4856, 0.517)`,
  29.05° elevation (the old Y was the only wrong sign), Luminance 31 800 → **15 380** nits,
  AmbientIlluminance 17 000 → **8 020** lx, AverageColor grey → `(0.386, 0.634, 1.0)`. The picture's
  own sun/sky ratio on the ground is 5.0; the old manifest put it at 2.4. `forest` lost its hand-made
  sun the same day and takes this star with its cascades: it had TWO suns (173 klx, one shining up
  from below the horizon) and an IBL mask over empty sky. 27 manifests remain.

## What remains

- [ ] Owner decision: negate Y in the 28 `Direction` vectors (a mechanical data fix in
      `projet-alpha.data/data-stores/Backgrounds/*.json`), or re-measure each sun with
      `tools/sky-manifest.py` for the HDR skies (3 files) and by hand for the LDR ones.
- [ ] Verify one sky-driven demo per family afterwards at a PINNED exposure: ground shadows
      present, sun on the side the skybox shows it.

## ⚠️ Traps

- The manifests are under the owner gate of `sky-review-28-manifests` — do not edit them in a
  side job.
- A star whose `Illuminance` is high hides the defect: the IBL ambient (which is oriented right)
  lights the scene plausibly, only the SHADOWS are missing. Look for shadows, not brightness.
- `InTexture` bodies are masked out of the IBL bake at the MANIFEST direction: a wrong direction
  masks the wrong patch of sky (a hole where there is no sun) and leaves the real sun in.

## References

- `src/Graphics/AGENTS.md` § Background photometric contract.
- `src/Scenes/AGENTS.md` § Sky → LightSet bridge (the OPEN note).
- `tools/sky-manifest.py`.
