---
id: sprite-uniform-scale
title: Sprites — decide what a sprite definition's UniformScale means
status: open
priority: low
scope: Graphics
opened: 2026-09-28
tags: [graphics, data, measured]
---

# Sprites — decide what a sprite definition's UniformScale means

## Why

Since 2026-09-28 a MESH's `UniformScale` is drawn by its instance
(`RenderableInstance::Abstract::applyLocalTransformation()`, `docs/caution-points.md` § A mesh's
uniform scale). Sprites were left out on purpose: `SpriteResource` reads the same key, and the
data store's sprite definitions carry values that were NEVER drawn — `UniformScale: 64` on the
explosions, fires, fireballs and smoke, `4` on three others. Drawing them would make every
explosion 64× larger. Only the LOD radius used them (radius × 64, over-conservative); it no
longer does for sprites (`Scene.rendering.cpp` `worldRadius()`).

## What remains

- [ ] Decide the semantics: a sprite's world size (its quad side in metres?) or a unit like a mesh's.
- [ ] Measure what each sprite is drawn at today and set the definitions to match the decision.
- [ ] Then draw it (remove the sprite exception in `applyLocalTransformation()`, `Visual::meshScale()`,
  `worldRadius()`).
