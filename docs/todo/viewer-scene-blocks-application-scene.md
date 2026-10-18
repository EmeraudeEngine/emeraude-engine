---
id: viewer-scene-blocks-application-scene
title: An active viewer scene blocks the application's next scene
status: open
priority: high
scope: src/Core.cpp (openModelViewer, openImageViewer, clearStageForViewerScene), src/Scenes/Manager.cpp (enableScene)
opened: 2026-10-18
tags: [defect, viewers, scenes, open-files]
---

# An active viewer scene blocks the application's next scene

## Why

A file opened from the main menu (`Core.openFiles()`, a dropped file) shows in a Core-owned viewer scene
(`+ModelViewer`, `+ImageViewer`). The viewer stays the ACTIVE scene, and the application's next scene is then refused:
`Stage.loadDemo(...)` fails with `[Warning][Manager] The scene '+ModelViewer' is still active. Disable it before !`
(`Scenes::Manager::enableScene()`). Found by the macOS peer on 2026-10-18 while validating the WAD split: an FBX opened
through `openFiles` (+ModelViewer) and then `Stage.loadDemo` — the demo never loads.

The reverse direction is handled on purpose: `Core::clearStageForViewerScene()` never disturbs a regular active scene
("A scene is running, the file was ignored." — a user notification only). But the console reply to `Core.openFiles()`
still says "1 of 1 file(s) taken by the opening pipeline." (a success) when the file was ignored that way: the console
reports an opening that did not happen (seen on Linux, 2026-10-18).

## What remains

- Owner decision (2026-10-18): **Core removes its viewer scene when a scene is loaded** — a viewer is a transient,
  Core-owned scene that any other scene replaces. Co-development rule: the fix belongs in the engine, never as a
  workaround in a downstream `Stage`.
- The console reply of `Core.openFiles()` must say "ignored" when `clearStageForViewerScene()` refused the file.
- Verify: open an FBX from the main menu, then `Stage.loadDemo("animation-debug")` — the demo loads, the viewer is gone,
  0 VUID; and `Core.openFiles()` during a demo answers that the file was ignored.

## References

- `src/Core.cpp` — `Core::openFiles()`, `clearStageForViewerScene()`, `openModelViewer()`, `openImageViewer()`.
- `src/Scenes/Manager.cpp` — `Manager::enableScene()` (the refusal).
- `src/Scenes/Viewers/ModelViewer.hpp`, `ImageViewer.hpp` — the scene names.
