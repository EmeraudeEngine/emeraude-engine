---
id: pom-visual-quality-broken
title: Parallax occlusion mapping looks broken on textured surfaces
status: open
priority: unranked
scope: Saphir/Generator/SceneRendering, Graphics/Material/StandardResource
opened: 2026-09-08
blocked-by: [pom-setting-outside-program-cache-key]
tags: [shaders, quality, owner-report]
---

# Parallax occlusion mapping looks broken on textured surfaces

## Why

**Owner report, 2026-09-08**, on `light-and-shadow-debug` with
`Core/Graphics/Texture/POMIterations = 16`: *"le parallax sur texture devra être réglé aussi […]
parce qu'il est un peu éclaté"* — the parallax on textures needs fixing, it looks blown out.

The setting had been sitting at the engine default **0** (POM entirely disabled, no POM code
emitted at all), so this is the first look at the feature in a while rather than a regression.

## ⚠️ Blocked, and why the blocker is not optional

`POMIterations` does **not** enter the program cache key
([`pom-setting-outside-program-cache-key.md`](pom-setting-outside-program-cache-key.md)), and both
shader caches are on by default with the SPIR-V one persisting on disk. Until that is fixed, a
launch with a changed POM count can be served the shader built for the previous count, so **no
before/after on this feature means anything**. Fix the key first, or run the whole investigation
with both shader caches set to false.

## What remains

- [ ] Reproduce with a named surface and a pinned pose, once the blocker allows a valid A/B.
- [ ] Read the GENERATED GLSL for the POM march (both shader caches false, or nothing is dumped)
      before touching the shader — the engine's own rule for this class of work.
- [ ] Check the tangent frame the march runs in: the surfaces to suspect first are those whose
      tangents are generated rather than supplied.

## ⚠️ Traps

- ⚠️ Do NOT read the black sphere of that scene as a POM symptom. It is a **separate, unattributed**
  observation (a geodesic sphere at quality 4 carrying `Walls/Bricks001` renders near-black, mean
  32/255, and it was already dark before POM was ever enabled). Whether the two share a cause is
  unknown; measuring them together is how a wrong attribution gets made.
- ⚠️ `Core/Graphics/Texture/POMIterations` is clamped to [4, 64] or the special 0
  (`Generator/Abstract.hpp:setPOMIterations()`): an authored 1 or 2 is not what runs.
