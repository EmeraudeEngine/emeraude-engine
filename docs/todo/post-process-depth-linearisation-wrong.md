---
id: post-process-depth-linearisation-wrong
title: The post-process chain linearises depth with frozen clip planes — distances are wrong, by a factor that depends on the lens
status: in-progress
priority: high
scope: Graphics/PostProcessor (setClipPlanes, FrameContext constants), Graphics/Effects (AtmosphericFog, VolumetricScattering, VolumetricLight, DepthOfField, MotionBlur, SSAO, TAA, SSR, SSGI, RTGI), Graphics/SelectionOutline
opened: 2026-10-25
tags: [post-processing, depth, defect]
---

# The post-process chain linearises depth with frozen clip planes

## Why

Found while mapping the effects the aerial perspective will touch (2026-10-25), checked by reading:

1. **The clip planes are frozen.** `PostProcessor::setClipPlanes()` (`PostProcessor.hpp`) has NO caller:
   `m_nearPlane{0.1F}` / `m_farPlane{1000.0F}` feed every `FrameContext::constants.nearPlane / farPlane`
   (`PostProcessor.cpp`), while the real projection uses far = `Core/Graphics/ViewDistance` (10 km by default) and
   near ≈ 0.089 m (`computeNearPlane()`).
2. ⚠️ **Not the formula** (CORRECTED 2026-10-25, after a measurement contradicted it). The effects write the OpenGL
   inverse `2·n·f / (f + n − z′·(f − n))` on `z′ = depth × 2 − 1`, which simplifies EXACTLY to the Vulkan inverse
   `n·f / (f − depth·(f − n))`. The opening version of this item called the formula wrong, and its table (1 m read as
   2.24 m) was computed that way: that was false. The whole error is the frozen planes.

The real near plane depends on the FIELD OF VIEW: `computeNearPlane()` = nearest object distance (0.1 m) /
√(1 + tan²(fov/2)·(aspect² + 1)). The frozen 0.1 matches only a long lens. The effects read, at 16:9:

| Lens (vertical FOV) | real near | 1 m | 2.6 m | 10 m | 1 km |
|---|---|---|---|---|---|
| 13 mm (85°, the labyrinth's player) | 0.047 m | 2.11 m | 5.5 m | 20.7 m | 680 m |
| 24 mm (53°) | 0.070 m | 1.43 m | 3.7 m | 14 m | 590 m |
| 85 mm (16°) | 0.096 m | 1.04 m | 2.7 m | 10.4 m | 510 m |

The far plane (1 000 frozen, 10 000 real) bends everything beyond a few hundred metres. A scalar formula is also wrong
for the orthographic camera, whatever its planes.

Consumers:
- the frozen planes in the linearisation: `AtmosphericFog`, `VolumetricScattering`, `DepthOfField`, `MotionBlur`,
  `SSAO`, `TAA`, `SSR`, `SSGI`;
- other uses of the frozen constants: `VolumetricLight`, `RTGI` (its sky ray distance is `farPlane`);
- `SelectionOutline` reads the REAL planes (`mainViewMatrices`) and is correct in perspective, wrong only in
  orthographic.

The effects that reconstruct through a matrix (`invRelativeViewProj`: RTAO, RTR, RTGI's trace, the contact shadows,
`VolumetricClouds`, `AerialPerspective`) are not affected. Nothing in the docs mentions `setClipPlanes`.

**Proof by the depth of field** (labyrinth's lamp panel, 2.6 m from the eye, manual focus sweep at f/1.4, the panel
edges' sharpness, 99th percentile of the gradient):
- 24 mm: the old DoF is sharpest on a plateau 3.0–4.4 m (it believed the panel at ~3.7 m, the ×1.43 above); the
  ported DoF peaks at 2.6 m (74.7 at 1.6 m, 128.7 at 2.6 m, 85 at 4.4 m).
- 85 mm: both peak at 2.6–2.7 m (the frozen near is right there).

**More evidence** (macOS-PA, 2026-10-25): SSAO drew a staircase and diagonals on sky-lab's 9 km cubes (150 mm lens);
disabling the AmbientOcclusion slot removed it.

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
- [x] **Every chain consumer ported** (2026-10-25, part 2): DepthOfField, MotionBlur, TAA, AtmosphericFog,
      VolumetricScattering, SSR, SSGI (the horizon search maps its screen direction through the matrix too),
      VolumetricLight (two dead fields), RTGI (its sky rays reach the REAL far plane). `PostProcessor::PushConstants` is
      16 bytes; `setClipPlanes()`, `nearPlane`, `farPlane`, `tanHalfFovY` are gone, with the two GLSL mirrors.
      Measured before / after with both binaries, 0 VUID: graphics doc 13-12 § 41 (fog contributions halved with a
      wide lens, SSGI +20 % in labyrinth, TAA's gate identical, RTGI +3 % at most). No setting default changed: the
      values in metres were physical choices and are now honoured.
- [ ] Linux gate, then the peers (macOS-PA, Windows-PA) on the same scenes; close the item after their reports.
- Not this item: `SelectionOutline` is not a chain effect and is right in perspective; its scalar formula is wrong
  only for an orthographic main camera (to rank if the editor ever needs it).

## References

- `src/Graphics/PostProcessor.{hpp,cpp}`, `src/Graphics/IndirectPostProcessEffect.hpp` (`FrameContext`), the effects
  above; `emeraude-base/src/Math/Matrix.hpp` (`perspectiveProjection()`), `ViewMatrices2DUBO.cpp` (the near plane);
  `dynamic-sky-aerial-perspective` (where it was met).
