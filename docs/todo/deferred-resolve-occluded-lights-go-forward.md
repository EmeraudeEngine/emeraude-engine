---
id: deferred-resolve-occluded-lights-go-forward
title: Deferred resolve — occluded lamps beyond 128 still get forward passes
status: open
priority: unranked
scope: Graphics/DeferredLightResolve, Scenes/Scene.rendering.cpp
opened: 2026-10-06
tags: [lighting, performance]
---

# Deferred resolve — occluded lamps beyond 128 still get forward passes

## Why

The resolve keeps the 128 visible lamps closest to the camera and skips the lamps whose reach misses the frustum
(2026-10-06). A frustum is not an occlusion test: in `labyrinth` (548 ceiling lamps, 40 × 40 cells), 498 lamps are
inside the frustum behind the walls at the spawn, so 370 are left to the forward passes, which re-draw every wall
region they touch. Measured (RTX 3070 Ti, 2880×1620, RT lane, validation ON): `ScenePass` 18.0 ms with the resolve,
of which `DeferredLights` 1.6 ms; 31.5 ms forward-only.

## What remains

Options to present to the owner (none chosen):

- Raise `MaxLights` with tiled or clustered light culling, so the per-pixel loop stays cheap (J. Andersson,
  "DirectX 11 Rendering in Battlefield 3", GDC 2011; O. Olsson, M. Billeter, U. Assarsson, "Clustered Deferred and
  Forward Shading", HPG 2012). The deferred resolve is exact for any number of lights; only its loop cost grows.
- Drop the occluded lamps before the forward path: test each lamp's sphere against a Hi-Z of the previous frame
  (item `hi-z-occlusion-culling`).
- Leave the forward overflow and only measure the gain of each option at the `labyrinth` spawn.

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md` § The frame's selection.
- Item `mrt-single-pass-deferred` (the resolve's design decisions).
