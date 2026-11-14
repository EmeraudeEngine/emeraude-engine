---
id: camera-debug-model-in-its-own-target
title: A camera created with its debug model seems to render that model into its own target
status: open
priority: unranked
scope: Scenes/Toolkit (generatePerspectiveCamera, generateTexture2DRenderer: showModel), Scenes/AbstractEntity.debug (the camera's visual debug), Graphics/RenderTarget (excludeFromRendering)
opened: 2026-11-15
tags: [render-target, visual-debug, camera, toolkit]
---

# A camera created with its debug model seems to render that model into its own target

## Why

Measured 2026-11-15 (`docs/render-targets.md` § A uniform dump is first of all a picture): in `offscreen-rendering`
the security camera's target (`SecurityTexture2D`) is ONE colour, (0, 0, 255, 255), for about a quarter of each turn
of that rotating camera (period 10.4 s), and the `TV` of the scene shows it. For six weeks it was taken for a wrong
read-back (the former item `dump-render-target-first-dump-solid-blue`, Windows, 3 launches of 6).

The CANDIDATE, not proven: the camera sees its own debug model.
- The demo creates the camera with `showModel = true`: `Toolkit::generatePerspectiveCamera()` calls
  `enableVisualDebug(VisualDebugType::Camera)` on the camera's OWN entity (`AbstractEntity::getCameraVisualDebug()`:
  the geometry `Items/Camera` with the default standard material), so the eye sits inside that model.
- A saturated blue shape of about 1.7 m is at the camera's position, (15, 2, -10): at the centre of the +X face of
  the scene's environment probe, 12.5 m away, on Linux and on macOS (the `TV`, at (-0.5, 1.5, -1), is on another
  face).
- Not explained: why a quarter of the turn only, if the model turns with the camera. Either the visual debug of an
  animated STATIC entity does not follow it, or the model's own shape hides the view in that sector.

The render targets can leave an instance out (`RenderTarget::Abstract::excludeFromRendering()`); the Toolkit does
not use it for the model of the camera which feeds the target.

## What remains

- [ ] Prove it: hide the camera's debug model and repeat the series of dumps (thirty, more than a second apart):
  no blue phase is expected. No console command shows or hides an entity's visual debug today — propose one to the
  owner (name, arguments, the state it changes) rather than editing the demo for an A/B.
- [ ] Look at the model from outside at two instants a quarter of a turn apart: does it turn with the camera?
- [ ] Owner decision once proven: a camera's own debug model is left out of the target that camera renders (in the
  Toolkit, through `excludeFromRendering()`), or the demos stop asking for the model, or nothing (a debug display).

## ⚠️ Traps

- The blue is neither a wrong read nor the clear colour (black): do not reopen the transfer.
- One dump says nothing: the picture is blue a quarter of the time. Read a series.

## References

- `docs/render-targets.md` (the measurements, the rule).
- `src/Scenes/Toolkit.hpp` (`generateTexture2DRenderer()`, `generatePerspectiveCamera()`), `src/Scenes/AbstractEntity.debug.cpp`.
- projet-alpha `src/Builtin/OffscreenRendering.cpp` (the security camera, its `LocalYRotation` animation, the `TV`).
