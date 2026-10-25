---
id: post-process-depth-linearisation-wrong
title: The post-process chain linearises depth with the OpenGL formula and frozen clip planes — distances are wrong at every range
status: in-progress
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

**More evidence** (macOS-PA, 2026-10-25): SSAO drew a staircase and diagonals on sky-lab's 9 km cubes (150 mm lens);
disabling the AmbientOcclusion slot removed it. Two more consumers were wrong without the OpenGL formula:
- SSR and SSGI use the Vulkan inverse, but with the frozen planes;
- `SelectionOutline` reads the real planes, but through the OpenGL formula.

## Owner decision (2026-10-25)

**Option B, a matrix reconstruction everywhere**, through ONE per-frame chain UBO. `PostProcessor` writes it
(`FrameContext::view`) and `Effects/Shared/ViewReconstructionGLSL.hpp` holds the helpers: the projection, its inverse,
the camera-relative inverse view-projection and the camera position. Every scalar copy of the clip planes disappears
from the chain. Why not the real planes + the Vulkan formula: a scalar formula is wrong for the engine's orthographic
camera, and the copy is what went out of sync.

## What remains

- [x] The UBO and the helpers.
- [x] **SSAO** ported (2026-10-25). Measured at labyrinth's pose (SSGI lane, the AO factor = AO on / AO off per
      pixel):
  - the floor-wall corner band now covers its metric width: profile 0.80 / 0.91 / 0.98 against one row at 0.92
    before;
  - sky-lab's 9 km cubes lost their diagonals.
  - **Defect it revealed, fixed with it**: the unlit path wrote aoResponse 15 and a placeholder normal (0, 0, 1)
    (`SceneRendering.cpp`). The emissive lamp panels lost ~36 % to SSAO once the distances were true. Unlit
    surfaces now write aoResponse 0, and the SSAO pass outputs 1 there, so the blur cannot spread it.
  - **Look decision (owner, 2026-10-25): the radius stays 0.5 m** (`AmbientOcclusion/ScreenSpace/Radius`): it now
    means 0.5 m. The AO used to cover ~0.23 m near the camera; corners are visibly stronger. The rule for the next
    effects: a retuned value changes its setting default too (`SettingKeys.hpp`); the owner's `settings.json` keeps
    a persisted value.
  - Still there and NOT this defect: a uniform band on the 9 km cubes' side faces. A 0.5 m radius projects to under
    a pixel there and depth precision rules; a screen-space radius floor is the usual lever.
- [ ] DepthOfField, MotionBlur, TAA, SelectionOutline, AtmosphericFog, VolumetricScattering, VolumetricLight, SSR,
      SSGI, RTGI (`skyDistance = farPlane`): each ported and measured before / after; a value tuned against the wrong
      distances is reported, and its setting default follows.
- [ ] Then remove `nearPlane`, `farPlane` and `tanHalfFovY` from `PostProcessor::PushConstants`, from the mirrors
      (`CombinePass`, `Saphir/Generator/PostProcessing.cpp`) and `setClipPlanes()`.

## References

- `src/Graphics/PostProcessor.{hpp,cpp}`, `src/Graphics/IndirectPostProcessEffect.hpp` (`FrameContext`), the effects
  above; `emeraude-base/src/Math/Matrix.hpp` (`perspectiveProjection()`), `ViewMatrices2DUBO.cpp` (the near plane);
  `dynamic-sky-aerial-perspective` (where it was met).
