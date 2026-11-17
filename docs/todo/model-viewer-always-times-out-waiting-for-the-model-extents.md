---
id: model-viewer-always-times-out-waiting-for-the-model-extents
title: The model viewer waits five seconds on the main thread for extents that never come, then frames on the fallback
status: open
priority: high
scope: Scenes/Viewers/ModelViewer (and ImageViewer to check), Scenes/Component load completion
opened: 2026-11-17
tags: [defect, viewers, main-thread]
---

# The model viewer waits five seconds on the main thread for extents that never come, then frames on the fallback

## Why

Opening a model (`Core.openFiles`, a dropped file) blocks the MAIN thread for the whole `ExtentsWaitBudgetMS`
(5000 ms, `ModelViewer.hpp`) and then frames the camera on the fallback (target at the origin, radius 2) instead of
the model's box. Measured 2026-11-17 with `Models/Box/glTF/Box.gltf`, Release, window-less:

| Machine | Reply time of `Core.openFiles` |
|---|---|
| Linux (RTX 3070 Ti), engine 5b23b534 | 5.07 s, twice in the same run |
| Linux, engine f10bc200 (2026-11-16) | 5.05 s and 5.09 s |
| Windows, engines 8332013e and 5b23b534 | 5.38 s |
| macOS, engines 8332013e and 5b23b534 | 5.8 s |

The journal of the Linux run: the model's three resources are loaded 0.34 s after the command; nothing happens from
+1.9 s to +5.045 s, where `ModelViewer` writes "The imported content published no extents in time, using the fallback
framing." Every other console command answers in 0.02 s.

The wait loop (`ModelViewer.cpp`, "The loaders enqueue mesh resources on the thread pool…") polls
`getWorldRenderBoundingBox()` of the static entities and of the model's nodes every 10 ms, sleeping on the main
thread.

## What remains

- **Find why the box never becomes valid.** ⚠️ Not verified - the lead: since `aacd2dcd8` (2026-11-15) a component
  only RECORDS its resource's load and refreshes its bounding volumes in its `processLogics()`, on the logic thread
  (lifecycle map S21); the viewer's scene is still being built on the main thread, so no logic cycle runs on it and
  the extents the loop waits for cannot be published while it waits. Check whether the timeout predates that commit.
- **Choose the fix with the owner**: the framing needs the model's extents, and a main thread that sleeps until a
  budget expires is neither a wait nor a contract. Whatever is chosen, the main thread must not be held.
- Check `ImageViewer` for the same shape.
- Proof to give: the reply time of `Core.openFiles`, the warning gone, the camera framed on the model's box (a model
  that is NOT at the origin nor of radius 2: the Box hides the defect, its box is close to the fallback).

## ⚠️ Traps

- The peers' rounds on the orbit controller (input doc 03) compared camera positions that all start from this
  fallback framing: they prove the controller, not the framing.
- The warning goes to the journal, not to the standard output: count it there.

## References

- `src/Scenes/Viewers/ModelViewer.cpp` (the wait loop, `ExtentsWaitBudgetMS`), lifecycle map S21, scenes doc 12.
