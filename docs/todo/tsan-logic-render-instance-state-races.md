---
id: tsan-logic-render-instance-state-races
title: The states shared by the logic and rendering threads all have a protocol — the last lot (a configuration written on a live object, whether an entity draws anything) awaits the peers
status: in-progress
priority: high
scope: Graphics/RenderableInstance::Abstract (when its configuration is written), Scenes/AbstractEntity (isPublishedRenderable()), Scenes/Manager.console.cpp (enableTBN), Graphics/Renderer (its four switches)
opened: 2026-11-15
tags: [tsan, concurrency, defect]
---

# The states shared by the logic and rendering threads all have a protocol — the last lot (a configuration written on a live object, whether an entity draws anything) awaits the peers

## Why

Both threads hold the active scene's access SHARED, so nothing orders a plain write on one side against a read on the
other. Every such state found — by ThreadSanitizer, then by reading — has ONE protocol since 2026-11-16: the table,
the rule for choosing a protocol, the proofs and the numbers are scenes doc 12 § *Plain state shared by the logic and
rendering threads*. The peers passed all of it but the last lot (rounds 10, 13 and 14).

The last lot (2026-11-16, owner decisions, proven on Linux — the last three rows of the table and the proof that
closes the section):
- whether an entity draws anything is read by a frame in ITS render state (`AbstractEntity::isPublishedRenderable()`,
  the `None` kind of the published extent), never on the live flag `updateEntityProperties()` resets and sets again;
- an instance's configuration is written before its component is linked to the entity (the builder's `setup()`) or
  under the scene's exclusive access — `enableTBN` takes it, `Toolkit::generateHLODProxy()` configures in the setup;
- the renderer's four switches written by another thread (the tangent-space display, the grab pass, the shadow maps
  and the render-to-textures) are atomics.

## What remains

- **The peers' round on that lot** (macOS, Windows): Release and Debug with 0 warning; the render statistics of the
  reference scenes unchanged (`animation-debug` 22 / 61198 | 21 / 61186, `light-and-shadow-debug` with projections
  7 / 673958 | 30 / 4033408, `terrain` as their round 14); `enableTBN(1)` then `enableTBN(0)` on `geometry-loader`
  (Windows: the vectors drawn then gone; macOS: refused, the device has no geometry stage — MoltenVK); exit 0, 0 VUID.
- Record the verdicts in scenes doc 12 and the ledger, then delete this file.

## ⚠️ Traps

- ThreadSanitizer misses these windows of a few microseconds (scenes doc 12, the last proof): a clean run on a peer
  proves nothing about them; what the peers verify is the build on their compiler and the unchanged behaviour.
- What the MAIN thread still does to a running scene through the scene service's other console commands is another
  item: `scene-console-commands-write-the-scene-outside-the-exclusive-access`.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads*; `docs/component-builder-pattern.md`
  (the setup note); `docs/renderable-instance-system.md` (when the configuration is written).
