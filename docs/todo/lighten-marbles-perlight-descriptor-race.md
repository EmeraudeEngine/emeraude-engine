---
id: lighten-marbles-perlight-descriptor-race
title: The terrain is drawn once before its PerLight descriptor set exists (lighten-marbles)
status: open
priority: high
scope: Graphics (RenderableInstance, the sealed pipeline layouts), Scenes (terrain creation, LightSet)
opened: 2026-10-02
tags: [rendering, race, descriptor-sets]
---

# The terrain is drawn once before its PerLight descriptor set exists (lighten-marbles)

## Why

Seen by the Windows peer on 2026-10-02 (RTX 3060 Laptop, `--disable-cef`, validation layers off), once in three
launches of `lighten-marbles`, during the physics P3 validation: four errors right after the terrain was created and
before the 'Moon' cubemap loaded:

```
[RenderableInstance] Descriptor set contract violation: the sealed pipeline layout declares the 'PerLight' set,
but the renderable instance cannot provide it. The draw call is skipped.
```

on `TerrainSceneFloorDetailWindow` and `TerrainSceneFloor`, render target View (1280x720). The draw is skipped (no
crash, no VUID reported), and the next launches were clean. It looks like a load-time race: the terrain's renderable
instance is drawn before the scene's lights / environment give it its per-light set. Not physics.

## What remains

- [ ] Reproduce: repeated launches of `lighten-marbles` (and other terrain demos), counting the message; with the
  validation layers ON.
- [ ] Find which side is late (the renderable instance's per-light resources, or the light set's) and make the draw
  wait for it, or make the set available before the first draw. Decide with the owner if the fix changes a contract.

## References

- The Windows report in `docs/physics-overhaul.md` § 1b (P3 accepted on Windows).
- `src/Graphics/RenderableInstance/Abstract.cpp:1239` (the "Descriptor set contract violation" message).
