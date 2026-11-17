---
id: model-viewer-always-times-out-waiting-for-the-model-extents
title: The model viewer makes its scene act on the loaded resources before framing - done on Linux, awaits the peers
status: in-progress
priority: high
scope: Scenes/Viewers/ModelViewer, Scenes/Scene and AbstractEntity (settleLoadedContent), Scenes/Component (settleLoadedResource)
opened: 2026-11-17
tags: [defect, viewers, main-thread]
---

# The model viewer makes its scene act on the loaded resources before framing - done on Linux, awaits the peers

## Why

Since 2026-11-15 a scene component only records the end of its resource's load, and the logic thread acts on it at its
next cycle - of the ACTIVE scene. The model viewer waited for its entities' boxes while BUILDING its scene, which has
no cycle: `Core.openFiles` held the main thread for its whole 5 s budget at every opening, then framed on the fallback
(the camera's distance, near plane and dolly limits all derived from a radius of 2 m, whatever the model).

Done on 2026-11-17 (owner decisions: the builder of a scene not enabled yet plays the logic thread's role; the budget
and its fallback stay): `Scene::settleLoadedContent()`, called by the viewer once the meshes are ready. The rule and
the numbers are scenes doc 12 § *A resource's load reaches a scene object on the logic thread*, point 2.

## What remains

- **A cold texture cache** (the peers' round 18, scenes doc 12): the viewer waits for the meshes to be READY, which
  includes their materials, whose textures are compressed on the CPU at the first opening. `Sponza`: 2.49 s on
  Linux, 3.5 s on macOS, 5.3 s and the fallback on Windows; slower machines than these three are the real users'.
  Owner decision (2026-11-17): a renderable publishes its EXTENT as soon as its geometry is loaded, before its
  materials, and the components derive their bounds from it - for every scene, not the viewer alone. The protocol
  (who writes, who reads, which flag) goes to the owner before any code.
- Proof to give: `Sponza` with an empty `--cache-directory` framed with a reply far under the texture work, on the
  three OS; ThreadSanitizer on scenes that build on resources still loading.

## ⚠️ Traps

- The camera positions of input doc 03 (the orbit controller's proof and the peers' round 17) all start from the
  FALLBACK framing of that day: they prove the controller, and they are no longer the positions of a `Box` opened now.
- `TextureTransformTest` still falls back: another cause, item `modelviewer-extents-fallback-framing`.
- The warning goes to the journal, not to the standard output: count it there.

## References

- `src/Scenes/Viewers/ModelViewer.cpp` (the wait), `Scene::settleLoadedContent()`, scenes doc 12, lifecycle map S21.
