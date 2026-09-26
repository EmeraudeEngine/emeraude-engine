---
id: rt-sprite-trace-cost
title: One alpha-tested sprite in the TLAS costs ~35 ms of ray-traced passes
status: open
priority: unranked
scope: Graphics/RayTracing, Scenes/SceneMetaData
opened: 2026-09-26
tags: [ray-tracing, sprite, performance, measured]
---

# One alpha-tested sprite in the TLAS costs ~35 ms of ray-traced passes

## Why

Measured on `game-logic` (RTX 3070 Ti, 2880×1620, RT lane, validation ON, GPU profiler), WITHOUT
pressing X, the camera untouched at the spawn: the frame costs **52-55 ms while `Actor::Fire` burns
and 18 ms once it has died** (its lifetime is 60 s). The whole gap is in the ray-traced passes:

| Pass | Fire alive (t ≈ 25-55 s) | Fire dead (t ≈ 85 s) |
|---|---|---|
| Frame | 52-55 ms | 18 ms |
| `RTREffect/trace` | 27-29 ms | 3.1 ms |
| `RTGIEffect/trace` | 11.9-12.7 ms | 3.1 ms |

The fire is ONE sprite (`fire001`, scaled by its 4 m damage radius, cylindrical billboard on its TLAS
instance) plus one shadow-less point light. The retired explosion sprite that stayed in the TLAS
before the `Node::destroyChild()` fix (engine `docs/caution-points.md` § *a node removed by
`destroyChild()` stayed DRAWN and TRACED forever*) cost ~110 ms the same way, at 60 m. A game with a
few fires or explosions on screen is not playable in the RT lane.

## What remains

- **Attribute before fixing** — nothing below is measured yet:
  - the alpha test itself: the instance is `FORCE_NO_OPAQUE` (`SceneMetaData.cpp`, alpha-tested /
    blended sub-materials), so every ray crossing the quad's bounds produces a candidate resolved in
    the shader (`rtCandidateIsSolid()`), reflection AND shadow rays AND GI rays;
  - the animated texture: a per-frame bindless slot refresh (`AnimatedTexture2D`, one 2D view per
    layer) — check whether the candidate test samples a large texture per candidate;
  - the point light at the hit: RTR evaluates every RT light per reflection hit — check with the
    fire's light alone (sprite hidden) versus the sprite alone.
- Then decide with the owner what a sprite owes the traced passes (e.g. excluded from shadow/GI rays,
  a cheaper candidate test, or a TLAS mask per ray type). ⚠️ An unlit emissive sprite is a LIGHT
  source for RTGI: excluding it from GI rays changes the image, not only the cost.

## ⚠️ Traps

- The fire DIES at 60 s: any A/B on `game-logic` must be taken on the same side of that instant, or
  the fire's disappearance reads as the effect of whatever was toggled.
- A barrel rolling into the fire explodes on its own (seen 2026-09-26): read the log / screenshot
  before trusting a "before X" baseline.
- Enable `Core/Graphics/GPUProfiler/Enabled` (read at launch) — `getStatus()` FPS alone cannot
  attribute a pass.

## References

- Engine `docs/caution-points.md` § *a node removed by `destroyChild()` stayed DRAWN and TRACED forever*.
- `Scenes/SceneMetaData.cpp` — TLAS instance flags, sprite billboard transform.
- projet-alpha `src/Actor/Fire.cpp`, `src/Actor/Explosion.cpp`.
