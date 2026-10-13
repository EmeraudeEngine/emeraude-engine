---
id: indirect-diffuse-intensity-scales-the-sky
title: IndirectDiffuse/Intensity scales the SKY too, so handing the sky to an effect costs 20 % of it
status: open
priority: medium
scope: Graphics (Effects/Lighting RTGI, SSGI combines), Scenes (indirect-diffuse ownership)
opened: 2026-09-14
tags: [rendering, lighting, photometry, owner-decision]
---

# `IndirectDiffuse/Intensity` scales the sky term

## Why

Both occupants of the `IndirectDiffuse` slot end their combine with
`em_Color.rgb += gi * intensity`, where `intensity` is the concept-level
`Core/Graphics/PostProcessing/IndirectDiffuse/Intensity` (default **0.8**). Since Sep 2026 `gi`
carries the SKY as well as the bounce, in both lanes — so the knob now scales a term that used to be
the raster's, where it was never scaled.

**Measured** on `sponza`'s roof under an open sky (2026-09-14, exposure pinned, `AmbientOcclusion`,
`Reflections` and `ContactShadows` switched off, four tile crops, values linearised): the effect
lanes deliver **0.78-0.90** of the raster's own diffuse IBL leg — consistent with the 0.8 knob plus a
small bounce. Handing the indirect diffuse over therefore dims the sky by ~20 %, and
`setLightingMode("None")` brightens a scene by ~25 % for that reason alone.

## The question for the owner

Is `Intensity` an artistic scale of **everything indirect** (current behaviour, one knob, but the
ownership transfer is no longer energy-neutral), or of **the bounce only** (the sky then crosses the
transfer unscaled and a lane switch stops changing the sky level)?

⚠️ Whatever is decided applies to BOTH lanes at once, or the two stop being comparable — that is the
whole point of the concept-level knobs (owner decision, 2026-09-12).

## What remains

1. Owner decision above.
2. If the sky is exempted: split the term in both combines and re-run the roof measurement — the
   ratio must come back to 1.0 ± the bounce.
3. Re-check the Sponza gallery numbers of
   `docs/subsystems/graphics/13-12-post-processing-effects/18-the-screen-space-sky-visibility-the-gtao-horizon-search-that.md`
   afterwards, since every figure there was taken at 0.8.

## References

- The combines: `src/Graphics/Effects/Lighting/SSGI.cpp:1033-1040`, `src/Graphics/Effects/Lighting/RTGI.cpp:1065`
  (`combineContribution`, the add at `:1095`); the knob read at `RTGI.cpp:752`; the key and its 0.8 default at
  `src/SettingKeys.hpp:873-874`.
- `docs/subsystems/graphics/13-12-post-processing-effects/17-indirect-diffuse-ownership-who-computes-the-sky-the-raster-o.md`
  and `.../18-the-screen-space-sky-visibility-the-gtao-horizon-search-that.md`.
