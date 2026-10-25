---
id: post-process-depth-linearisation-wrong
title: The post-process chain linearises depth with the OpenGL formula and frozen clip planes — distances are wrong at every range
status: open
priority: high
scope: Graphics/PostProcessor (setClipPlanes, FrameContext constants), Graphics/Effects (AtmosphericFog, VolumetricScattering, VolumetricLight, DepthOfField, MotionBlur, SSAO, TAA, SSR, SSGI, RTGI), Graphics/SelectionOutline
opened: 2026-10-25
tags: [post-processing, depth, defect]
---

# The post-process chain linearises depth with the OpenGL formula and frozen clip planes

## Why

Found while mapping the effects the aerial perspective will touch (2026-10-25), checked by reading:

1. **The clip planes are frozen.** `PostProcessor::setClipPlanes()` (`PostProcessor.hpp`) has NO caller:
   `m_nearPlane{0.1F}` / `m_farPlane{1000.0F}` feed every `FrameContext::constants.nearPlane / farPlane`
   (`PostProcessor.cpp`), while the real projection uses far = `Core/Graphics/ViewDistance` (10 km by default) and
   near ≈ 0.089 m (`computeNearPlane()`).
2. **The formula is OpenGL's.** The effects linearise with `2·n·f / (f + n − z·(f − n))`, the inverse of a [−1, 1] depth;
   the engine's projection (`Matrix::perspectiveProjection()`, Vulkan [0, 1], `LESS_OR_EQUAL`, clear 1.0) inverts as
   `n·f / (f − z·(f − n))`.

Both together, for the default camera (near 0.089, far 10 000):

| True distance | 1 m | 10 m | 50 m | 100 m | 500 m | 1 km | 2 km | 5 km | 9 km |
|---|---|---|---|---|---|---|---|---|---|
| What the effects read | 2.24 m | 22.0 m | 101 m | 185 m | 542 m | 714 m | 849 m | 957 m | 995 m |

Twice too far up close, and saturating under 1 km. Consumers:
- the OpenGL-form linearisation: `AtmosphericFog`, `VolumetricScattering`, `DepthOfField`, `MotionBlur`, `SSAO`, `TAA`,
  `SelectionOutline`;
- the frozen constants (other uses, to classify one by one): `VolumetricLight`, `SSR`, `SSGI`, `RTGI` (its sky ray
  distance is `farPlane`).
The effects that reconstruct positions through a matrix (`invRelativeViewProj`: RTAO, RTR, RTGI's trace, the contact
shadows, `VolumetricClouds`) are not affected by the formula. Nothing in the docs mentions `setClipPlanes`.

What it changes is not measured yet: SSAO's radius and DoF's focus are in metres (both off by ~2× in the first metres),
the fog's density integrates the wrong length, TAA's depth rejection compares wrong distances.

## What remains

- [ ] Choose the fix (owner): set the real near / far every frame (the camera can change them) AND switch every
      linearisation to the Vulkan [0, 1] inverse — or remove the scalar linearisation and reconstruct through the
      inverse projection matrix everywhere, the way the RT effects and the clouds already do (one shared GLSL helper).
- [ ] Measure each effect before / after (SSAO and DoF first: their parameters are in metres), and re-tune any setting
      that was tuned against the wrong distances.

## References

- `src/Graphics/PostProcessor.{hpp,cpp}`, `src/Graphics/IndirectPostProcessEffect.hpp` (`FrameContext`), the effects
  above; `emeraude-base/src/Math/Matrix.hpp` (`perspectiveProjection()`), `ViewMatrices2DUBO.cpp` (the near plane);
  `dynamic-sky-aerial-perspective` (where it was met).
