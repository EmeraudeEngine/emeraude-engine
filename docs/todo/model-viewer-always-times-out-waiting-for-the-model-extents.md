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

- **The peers' round** (macOS, Windows): Release and Debug with 0 warning; `Core.openFiles` on `Box`,
  `MetalRoughSpheresNoTextures`, `Fox` and `Sponza`: the reply time, no fallback warning in the journal, the viewer
  camera's position against the Linux values of scenes doc 12; exit 0, 0 VUID.
- Record the verdicts in scenes doc 12 and the ledger, then delete this file.

## ⚠️ Traps

- The camera positions of input doc 03 (the orbit controller's proof and the peers' round 17) all start from the
  FALLBACK framing of that day: they prove the controller, and they are no longer the positions of a `Box` opened now.
- `TextureTransformTest` still falls back: another cause, item `modelviewer-extents-fallback-framing`.
- The warning goes to the journal, not to the standard output: count it there.

## References

- `src/Scenes/Viewers/ModelViewer.cpp` (the wait), `Scene::settleLoadedContent()`, scenes doc 12, lifecycle map S21.
