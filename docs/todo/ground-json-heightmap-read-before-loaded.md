---
id: ground-json-heightmap-read-before-loaded
title: A JSON ground reads its heightmap's pixels before the image is loaded
status: open
priority: unranked
scope: Graphics/Renderable/TerrainResource, Graphics/Renderable/BasicGroundResource
opened: 2026-09-22
tags: [terrain, resources, loading]
---

# A JSON ground reads its heightmap's pixels before the image is loaded

## Why

`TerrainResource::load(const Json::Value &)` and `BasicGroundResource::load(const Json::Value &)`
ask for each `HeightMap` image with `images->getResource(name, true)` — an ASYNCHRONOUS request — and
call `applyDisplacementMapping(imageResource->data(), ...)` on the next line. An image not already in
memory hands back an empty pixmap, the displacement is skipped with a `Pixmap is not usable` line, and
the ground comes out FLAT with no error. It is the trap of `docs/caution-points.md` § "Inside load(),
a dependency is NOT loaded", in the two JSON ground paths. Found while giving the `terrain` demo its
heightmap (2026-09-22), which avoids it by asking synchronously (`getResource(name, false)`) outside
any `load()`.

## What remains

- [ ] Decide the shape: a synchronous request from inside a loading thread, or the image declared as a
  DEPENDENCY and the displacement applied in `onDependenciesLoaded()` (the engine's contract), which
  then builds the CDLOD geometry there instead of in `load()`.
- [ ] Reproduce first with a JSON terrain whose heightmap is not preloaded: the symptom is a flat ground.

## References

- `src/Graphics/Renderable/TerrainResource.cpp` — the `JKHeightMap` loop of `load(const Json::Value &)`.
- `src/Graphics/Renderable/BasicGroundResource.cpp` — the same loop.
