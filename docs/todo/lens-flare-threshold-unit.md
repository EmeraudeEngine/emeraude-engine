---
id: lens-flare-threshold-unit
title: LensFlare threshold is 0.8 in a chain that carries nits — every lit texel is "bright"
status: open
priority: unranked
scope: Graphics/Effects/Framebuffer/LensFlare
opened: 2026-08-30
tags: [post-processing, photometry, calibration]
---

# LensFlare threshold is 0.8 in a chain that carries nits — every lit texel is "bright"

## Why

`LensFlare::Parameters::threshold{0.8F}` predates the photometric pipeline. The threshold pass reads
the chain colour in nits: a shadowed wall at 50 nits passes it, so the ghosts are displaced copies
of the WHOLE scene rather than of the light source, and the effect's strength depends on the scene's
absolute brightness instead of on the source. The Bloom moved to a threshold in nits set on the
camera (`setBloomThreshold`, player baseline 1000 nits); the flare did not.

## What remains

1. Decide the unit: nits (like the bloom) or exposure-relative (a fraction of the display white,
   which is what 0.8 meant). The former keeps the two glare effects consistent.
2. Default it from a measurement on `post-processor-effect-debug` bench 1 once it has a proper
   stimulus (a sky whose disc is at the manifest's sun direction).

## References

- `src/Graphics/Effects/Framebuffer/LensFlare.cpp` — the threshold pass.
- `src/Graphics/AGENTS.md` § "LensFlare — the source is probed in the depth buffer".
