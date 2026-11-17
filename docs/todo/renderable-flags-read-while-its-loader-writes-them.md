---
id: renderable-flags-read-while-its-loader-writes-them
title: A renderable's flag word is constructor-only - done on Linux, awaits the peers; the geometries' and materials' words are not audited
status: in-progress
priority: high
scope: Graphics/Renderable (Abstract, SpriteResource), Scenes/Toolkit; to audit: Graphics/Geometry, Graphics/Material
opened: 2026-11-16
tags: [tsan, concurrency, defect, measured]
---

# A renderable's flag word is constructor-only - done on Linux, awaits the peers

## Why

ThreadSanitizer on `sprite`: the sprite's loader wrote a bit (`IsUprightSprite`, read in its manifest) into the flag
word while a `MultipleVisuals` built on that sprite read `isSprite()` from the same word. `isSprite()` itself never
answered wrong - `IsSprite` is given to the constructor -: the defect was the write of another bit.

Done on 2026-11-17 (owner decision: the word is frozen at construction): a renderable's bits say what its type is and
are never written after the constructor; what a load or a creation function declares (the upright billboard, the
vegetation wind) is a member written before the publication and read on a ready renderable. Scenes doc 12, the row
"What a renderable IS and what its load declares".

## What remains

- **The peers' round** (macOS, Windows): Release and Debug with 0 warning; `sprite` and `tree-generator`: the same
  statistics as Linux, the flames upright, the leaves moving; exit 0, 0 VUID.
- **Audit the same word in the other resource families** (not done, no race measured): the word belongs to
  `Resources::ResourceTrait`, and the geometries (`EnableTangentSpace`, `EnableNormal`…) and the materials
  (`TextureEnabled`, `UnlitEnabled`, `IsCreated`…) write it during their load. Find who reads those bits before the
  load ends; the rule to apply is the renderable's.
- Record the verdicts in scenes doc 12 and the ledger, then delete this file (or keep it for the audit alone).

## ⚠️ Traps

- The race shows when the two threads meet: 4 runs of 6 before. One clean run proves nothing.
- `sprite` draws its 100 000 sprites at random places: two launches never give the same image - compare the
  statistics and LOOK, never a pixel A/B.
- `enableVegetationWind()` called AFTER `getOrCreateResource()` returned writes a renderable a scene may already draw
  (what `Toolkit` did): declare inside the creation function.

## References

- Scenes doc 12; `src/Graphics/Renderable/Abstract.hpp` (`RenderableFlagBits`).
